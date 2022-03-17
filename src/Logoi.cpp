#include "plugin.hpp"



struct Logoi : Module {

	// derived from https://github.com/pingdynasty/ClockDelay/blob/master/ClockDelay.cpp
	class ClockCounter {
	public:
		inline void reset() {
			pos = 0;
			off();
		}
		bool next() {
			if (++pos > value) {
				pos = 0;
				return true;
			}
			return false;
		}

		uint8_t pos = 0;
		uint8_t value = 0;

		void rise() {
			if (next())
				on();
			else
				off();
		}
		inline void fall() {
			off();
		}
		virtual bool isOff() {
			return delayOutput->getVoltage() == 0;
		}
		virtual void on() {
			delayOutput->setVoltage(10.f);
		}
		virtual void off() {
			delayOutput->setVoltage(0.f);
		}
		void setOutput(Output* delayOutput_) {
			delayOutput = delayOutput_;
		}
	private:
		Output* delayOutput;
	};


	class ClockDivider {
	public:
		inline void reset() {
			pos = 0;
			toggled = false;
			off();
		}
		bool next() {
			if (++pos > value) {
				pos = 0;
				return true;
			}
			return false;
		}

		uint8_t pos = 0;
		int8_t value = 0;
		bool toggled = false;
		inline bool isOff() {
			return dividedOutput->getVoltage() == 0;
		}
		void rise() {
			if (next()) {
				toggle();
				toggled = true;
			}
		}
		void fall() {
			if (value == -1)
				off();
		}
		void toggle() {
			bool state = (bool) dividedOutput->getVoltage();
			dividedOutput->setVoltage(10.f * !state);
		}
		void on() {
			dividedOutput->setVoltage(10.f);
		}
		void off() {
			dividedOutput->setVoltage(0.f);
		}
		void setOutput(Output* dividedOutput_) {
			dividedOutput = dividedOutput_;
		}
	private:
		Output* dividedOutput;
	};

	class ClockDelay {
	public:
		uint16_t riseMark = 0;
		uint16_t fallMark = 0;
		uint16_t value = 0; 	// number of (pseudo) clock ticks until rise should happen
		uint16_t pos = 0;
		bool running = false;
		inline void start() {
			pos = 0;
			fallMark = 0;
			running = true;
		}
		inline void stop() {
			running = false;
		}
		inline void reset() {
			stop();
			off();
		}
		inline void rise() {
			riseMark = value;
			start();
		}
		inline void fall() {
			fallMark = riseMark + pos;
		}
		inline void clock() {
			if (running) {
				if (++pos == riseMark) {
					on();
				}
				else if (pos == fallMark) {
					off();
					stop(); // one-shot
				}
			}
		}
		virtual void on() {
			delayOutput->setVoltage(10.f);
		}
		virtual void off() {
			delayOutput->setVoltage(0.f);
		}
		virtual bool isOff() {
			return delayOutput->getVoltage() == 0;
		}
		void setOutput(Output* delayOutput_) {
			delayOutput = delayOutput_;
		}
	private:
		Output* delayOutput;
	};

	class ClockSwing : public ClockDelay {
	public:
		void on() {
			combinedOutput->setVoltage(10.f);
		}
		void off() {
			combinedOutput->setVoltage(0.f);
		}
		bool isOff() {
			return combinedOutput->getVoltage() == 0;
		}
		void setOutputs(Output* delayOutput_, Output* combinedOutput_) {
			ClockDelay::setOutput(delayOutput_);
			combinedOutput = combinedOutput_;
		}
	private:
		Output* combinedOutput;
	};

	class DividingCounter : public ClockCounter {
	public:
		void setOutputs(Output* delayOutput_, Output* combinedOutput_) {
			ClockCounter::setOutput(delayOutput_);
			combinedOutput = combinedOutput_;
		}
		void on() {
			combinedOutput->setVoltage(10.f);

		}
		void off() {
			combinedOutput->setVoltage(0.f);
		}
		bool isOff() {
			return combinedOutput->getVoltage() == 0;
		}
	private:
		Output* combinedOutput;
	};
	// end of imported/modifed hardware code

	enum ParamId {
		DIVISION_PARAM,
		COUNT_OR_DELAY_PARAM,
		DIVISION_CV_PARAM,
		COUNT_OR_DELAY_CV_PARAM,
		MODE_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		DIVISION_CV_INPUT,
		COUNT_OR_DELAY_CV_INPUT,
		RESET_INPUT,
		CLOCK_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		DIVISION_OUTPUT,
		ADDITION_DELAY_OUTPUT,
		COMBINED_OUTPUT,
		CLOCK_THRU_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		COMBINED_LIGHT,
		DIVISION_LIGHT,
		COUNT_OR_DELAY_LIGHT,
		LIGHTS_LEN
	};

	ModuleTheme theme = LIGHT_THEME;


	dsp::SchmittTrigger clockDetector, resetDetector;
	dsp::BooleanTrigger fallDetector;

	dsp::PulseGenerator pulseGenerator;
	dsp::Timer delayTimer;
	bool delaying = false;


	dsp::ClockDivider updateClocksController; 	// used to update delay counters every N samples
	const int updateClocksFrequency = 64;		// number of samples to wait between updates (N)

	ClockDivider divider;	// standard clock divider, powers left hand side
	ClockCounter counter;	// clock counter, powers right hand side (when in count mode)
	ClockDelay delay; 		// clock delay, powers right hand side (when in delay mode)

	DividingCounter divcounter;		// used to combine left+right (when right in count mode)
	ClockSwing swinger;				// used to combine left+right (when right in delay mode)
	static constexpr float maxDelayTime = 1.f;

	enum OperatingMode {
		COUNT_MODE,
		DELAY_MODE,
		DISABLED_MODE
	};

	void reset() {
		divider.reset();
		counter.reset();
		divcounter.reset();
		delay.reset();
		swinger.reset();
	}

	struct DividerParam : ParamQuantity {
		std::string getDisplayValueString() override {
			if (module != nullptr) {
				if (paramId == DIVISION_PARAM) {
					int divisionForLabel = divisionFromParamUser(getValue());
					return std::to_string(divisionForLabel);
				}
				else {
					assert(false);
				}
			}
			else {
				return "";
			}
		}

		void setDisplayValueString(std::string s) override {
			float division = std::atof(s.c_str());
			if (module != nullptr) {
				if (paramId == DIVISION_PARAM) {
					ParamQuantity::setValue(paramFromDivisionUser(division));
				}
				else {
					assert(false);
				}
			}
		}
	};

	struct CountOrDelayParam : ParamQuantity {
		std::string getDisplayValueString() override {
			if (module != nullptr) {
				if (paramId == COUNT_OR_DELAY_PARAM) {
					const int mode = module->params[MODE_PARAM].getValue();
					switch (mode) {
						case DELAY_MODE: return std::to_string(delayFromParamUser(getValue())) + " ms";
						case COUNT_MODE: return std::to_string(countFromParamUser(getValue()));
						default: return "Not in use";
					}
				}
				else {
					assert(false);
				}
			}
			else {
				return "";
			}
		}

		void setDisplayValueString(std::string s) override {
			float value = std::atof(s.c_str());
			if (module != nullptr) {
				if (paramId == COUNT_OR_DELAY_PARAM) {
					const int mode = module->params[MODE_PARAM].getValue();
					switch (mode) {
						case DELAY_MODE: ParamQuantity::setValue(paramFromDelayUser(value)); break;
						case COUNT_MODE: ParamQuantity::setValue(paramFromCountUser(value)); break;
						default: ParamQuantity::setValue(value);
					}
				}
				else {
					assert(false);
				}
			}
		}
	};

	Logoi() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam<DividerParam>(DIVISION_PARAM, 0.f, 1.f, 0.f, "Divider");
		configParam<CountOrDelayParam>(COUNT_OR_DELAY_PARAM, 0.f, 1.f, 0.f, "Counter/Delay");

		configParam(DIVISION_CV_PARAM, 0.f, 1.f, 0.f, "Divider CV");
		configParam(COUNT_OR_DELAY_CV_PARAM, 0.f, 1.f, 0.f, "Counter/Delay CV");
		configSwitch(MODE_PARAM, 0.f, 2.f, 0.f, "Mode", {"Count", "Delay", "Off"});

		configInput(DIVISION_CV_INPUT, "Divider CV");
		configInput(COUNT_OR_DELAY_CV_INPUT, "Counter/Delay CV");
		configInput(RESET_INPUT, "Reset");
		configInput(CLOCK_INPUT, "Clock");

		configOutput(DIVISION_OUTPUT, "Divider");
		configOutput(ADDITION_DELAY_OUTPUT, "Counter/Delay");
		configOutput(COMBINED_OUTPUT, "Combined Output");
		configOutput(CLOCK_THRU_OUTPUT, "Clock thru");

		// individual processors
		divider.setOutput(&outputs[DIVISION_OUTPUT]); 			// left side, clock divider
		counter.setOutput(&outputs[ADDITION_DELAY_OUTPUT]);		// right side, mode COUNT (switch down)
		delay.setOutput(&outputs[ADDITION_DELAY_OUTPUT]);		// right side, mode DELAY (switch middle)

		// combined processors: COUNT mode
		divcounter.setOutputs(&outputs[ADDITION_DELAY_OUTPUT], &outputs[COMBINED_OUTPUT]);
		// combined processors: DELAY mode
		swinger.setOutputs(&outputs[ADDITION_DELAY_OUTPUT], &outputs[COMBINED_OUTPUT]);

		reset();

		updateClocksController.setDivision(updateClocksFrequency);
	}

	// given VCV param in range 0 - 1, convert to the expected division (for the algorithm)
	static int8_t divisionFromParamInternal(float paramValue) {
		return (paramValue < 1. / 64.f) ? -1 : (int)(paramValue * 31.f);
	}
	// given VCV param in range 0 - 1, convert to the expected division (in a form meaningful for the user)
	static int8_t divisionFromParamUser(float paramValue) {
		const int8_t divisionInternal = divisionFromParamInternal(paramValue);
		return divisionInternal == -1 ? 1 : (2 * (1 + divisionInternal));
	}
	// given a clock division from the user, convert to the param value (0 - 1)
	static float paramFromDivisionUser(int division) {
		int divisionInternal = (division == 1) ? -1 : (division / 2 - 1);
		return rescale(clamp(divisionInternal, -1, 31), -1.f, 31.f, 0.f, 1.f);
	}

	// given VCV param in range 0 - 1, convert to the expected count (for the algorithm)
	static int8_t countFromParamInternal(float paramValue) {
		return std::round(31 * paramValue);
	}
	// given VCV param in range 0 - 1, convert to the expected count (for the algorithm)
	static int8_t countFromParamUser(float paramValue) {
		return 1 + countFromParamInternal(paramValue);
	}

	// given VCV param in range 0 - 1, convert to the expected count (for right hand side)
	static float paramFromCountUser(float count) {
		count = clamp(count, 1.f, 32.f);
		return clamp((count - 1) / 31.f, 0.f, 1.f);
	}

	static float delayFromParamUser(float paramValue) {
		return maxDelayTime * 1000.f * paramValue;
	}

	static float paramFromDelayUser(float delay) {
		return delay / (maxDelayTime * 1000.f);
	}


	// given VCV param in range 0 - 1, convert to the expected delay (for the algorithm)
	uint16_t delayFromParamInternal(float paramValue) {
		// max Rack sample rate is 768kHz - with updateClocksFrequency == 64, which means we ping the clocks,
		// delay.clock() and swinger.clock() every 64 samples, the largest reasonable value
		// of maxClockTicks is 12000. Tick counter of type uint16_t [0, +65535] shouldn't overflow.
		const float maxClockTicks = (maxDelayTime / APP->engine->getSampleTime()) / updateClocksFrequency;

		return 1 + std::round(maxClockTicks * paramValue);
	}

	void processBypass(const ProcessArgs& args) override {
		clockDetector.process(inputs[CLOCK_INPUT].getVoltage());
		outputs[DIVISION_OUTPUT].setVoltage(clockDetector.isHigh() * 10.f);
		outputs[ADDITION_DELAY_OUTPUT].setVoltage(clockDetector.isHigh() * 10.f);
		outputs[COMBINED_OUTPUT].setVoltage(clockDetector.isHigh() * 10.f);
		outputs[CLOCK_THRU_OUTPUT].setVoltage(clockDetector.isHigh() * 10.f);
	}

	void process(const ProcessArgs& args) override {

		// process LHS knobs
		{
			// input CV in range -10V to +10V
			const float scaledDivisionCV = clamp(params[DIVISION_CV_PARAM].getValue() * inputs[DIVISION_CV_INPUT].getVoltage(), -10.f, +10.f);
			// CV sums with knob, where +10V is equivalent to full clockwise knob turn
			const float divisionWithCV = clamp(params[DIVISION_PARAM].getValue() + scaledDivisionCV / 10.f, 0.f, 1.f);
			divider.value = divisionFromParamInternal(divisionWithCV);
		}
		// process RHS knobs
		{
			// input CV in range -10V to +10V
			const float scaledCountDelayCV = clamp(params[COUNT_OR_DELAY_CV_PARAM].getValue() * inputs[COUNT_OR_DELAY_CV_INPUT].getVoltage(), -10.f, +10.f);
			// CV sums with knob, where +10V is equivalent to full clockwise knob turn
			const float countDelayWithCV = clamp(params[COUNT_OR_DELAY_PARAM].getValue() + scaledCountDelayCV / 10.f, 0.f, 1.f);
			// mode RHS modes infer params from the same source(s)
			divcounter.value = counter.value = countFromParamInternal(countDelayWithCV);
			delay.value = swinger.value = delayFromParamInternal(countDelayWithCV);
		}

		if (resetDetector.process(inputs[RESET_INPUT].getVoltage())) {
			reset();
		}

		// do every N ticks (set by updateClocksFrequency)
		if (updateClocksController.process()) {
			delay.clock();
			swinger.clock();
		}

		const int mode = (int) params[MODE_PARAM].getValue();
		switch (mode) {
			case DELAY_MODE: getParamQuantity(COUNT_OR_DELAY_PARAM)->name = "Delay"; break;
			case COUNT_MODE: getParamQuantity(COUNT_OR_DELAY_PARAM)->name = "Count"; break;
			case DISABLED_MODE: getParamQuantity(COUNT_OR_DELAY_PARAM)->name = "Off"; break;
		}

		// Schmitt trigger on incoming clock
		const bool rising = clockDetector.process(inputs[CLOCK_INPUT].getVoltage());
		// returns true when previous clock state was high and next is low
		const bool falling = fallDetector.process(!clockDetector.isHigh());
		// and forward the clock to the thru output
		outputs[CLOCK_THRU_OUTPUT].setVoltage(clockDetector.isHigh() * 10.f);

		if (rising) {
			divider.rise();
			switch (mode) {
				case DELAY_MODE: {
					delay.rise();
					if (divider.toggled) {
						swinger.rise();
					}
					else {
						outputs[COMBINED_OUTPUT].setVoltage(10.f);
						// COMBINED_OUTPUT_PORT &= ~_BV(COMBINED_OUTPUT_PIN); // pass through clock
						// CLOCKDELAY_LEDS_PORT |= _BV(CLOCKDELAY_LED_1_PIN);
					}
					break;
				}
				case COUNT_MODE: {
					counter.rise();
					if (divider.toggled) {
						divcounter.rise();
						if (!divcounter.isOff())
							divider.toggled = false;
					}
					break;
				}
			}
		}
		else if (falling) {
			switch (mode) {
				case DELAY_MODE: {
					delay.fall();
					if (divider.toggled) {
						swinger.fall();
						divider.toggled = false;
					}
					else {
						outputs[COMBINED_OUTPUT].setVoltage(0.f);
						// COMBINED_OUTPUT_PORT |= _BV(COMBINED_OUTPUT_PIN); // pass through clock
						// CLOCKDELAY_LEDS_PORT &= ~_BV(CLOCKDELAY_LED_1_PIN);
					}
					break;
				}
				case COUNT_MODE: {
					counter.fall();
					divcounter.fall();
					break;
				}
			}
			divider.fall();
		}

		if (mode == DISABLED_MODE) {
			outputs[DIVISION_OUTPUT].setVoltage(0.f);
			outputs[ADDITION_DELAY_OUTPUT].setVoltage(0.f);
			outputs[COMBINED_OUTPUT].setVoltage(0.f);
		}

		// do lights (just mirror output voltages)
		{
			lights[DIVISION_LIGHT].setBrightnessSmooth((bool) outputs[DIVISION_OUTPUT].getVoltage(), args.sampleTime);
			lights[COMBINED_LIGHT].setBrightnessSmooth((bool) outputs[COMBINED_OUTPUT].getVoltage(), args.sampleTime);
			lights[COUNT_OR_DELAY_LIGHT].setBrightnessSmooth((bool) outputs[ADDITION_DELAY_OUTPUT].getVoltage(), args.sampleTime);
		}
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* themeJ = json_object_get(rootJ, "theme");
		if (themeJ) {
			theme = (ModuleTheme) json_integer_value(themeJ);
		}
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "theme", json_integer(theme));

		return rootJ;
	}

};


struct LogoiWidget : ModuleWidget {
	LogoiWidget(Logoi* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/Logoi.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RebelTechPot>(mm2px(Vec(12.533, 26.112)), module, Logoi::DIVISION_PARAM));
		addParam(createParamCentered<RebelTechPot>(mm2px(Vec(37.933, 26.112)), module, Logoi::COUNT_OR_DELAY_PARAM));
		addParam(createParamCentered<RebelTechPot>(mm2px(Vec(12.533, 45.162)), module, Logoi::DIVISION_CV_PARAM));
		addParam(createParamCentered<RebelTechPot>(mm2px(Vec(37.933, 45.162)), module, Logoi::COUNT_OR_DELAY_CV_PARAM));
		addParam(createParamCentered<BefacoSwitch>(mm2px(Vec(25.225, 70.48)), module, Logoi::MODE_PARAM));

		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(12.525, 83.18)), module, Logoi::DIVISION_CV_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(37.925, 83.18)), module, Logoi::COUNT_OR_DELAY_CV_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(25.225, 95.88)), module, Logoi::RESET_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(12.525, 108.58)), module, Logoi::CLOCK_INPUT));


		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(12.525, 95.88)), module, Logoi::DIVISION_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(37.925, 95.88)), module, Logoi::ADDITION_DELAY_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(25.225, 108.58)), module, Logoi::COMBINED_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(37.925, 108.58)), module, Logoi::CLOCK_THRU_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(25.225, 57.78)), module, Logoi::COMBINED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(12.525, 70.48)), module, Logoi::DIVISION_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(37.975, 70.625)), module, Logoi::COUNT_OR_DELAY_LIGHT));
	}
};


Model* modelLogoi = createModel<Logoi, LogoiWidget>("Logoi");