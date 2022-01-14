#include "plugin.hpp"

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

	uint8_t pos;
	uint8_t value;

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

	uint8_t pos;
	int8_t value;
	bool toggled;
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
	uint16_t riseMark;
	uint16_t fallMark;
	uint16_t value; 	// number of (pseudo) clock ticks until rise should happen
	uint16_t pos;
	bool running;
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

struct Logoi : Module {
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

	Logoi() {

		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(DIVISION_PARAM, 0.f, 1.f, 0.f, "Divider");
		configParam(COUNT_OR_DELAY_PARAM, 0.f, 1.f, 0.f, "Counter/Delay");

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

	// given VCV param in range 0 - 1, convert to the expected division (for left hand side)
	int8_t divisionFromParam(float paramValue) {

		constexpr int  ADC_OVERSAMPLING = 4;
		constexpr int  ADC_VALUE_RANGE = (1024 * ADC_OVERSAMPLING);

		// the raw (oversampled) value that ATMEGA would return
		int v = std::round(paramValue * 4095);

		return (v < (ADC_VALUE_RANGE / 32 / 2)) ? -1 : v >> 7;;
	}

	// given VCV param in range 0 - 1, convert to the expected count (for right hand side)
	int8_t countFromParam(float paramValue) {

		return std::round(31 * paramValue);
	}

	// given VCV param in range 0 - 1, convert to the expected delay (for right hand side)
	uint16_t delayFromParam(float paramValue) {
		// max Rack sample rate is 768kHz - with updateClocksFrequency == 64, which means we ping the clocks,
		// delay.clock() and swinger.clock() every 64 samples, the largest reasonable value
		// of maxClockTicks is 12000. Tick counter of type uint16_t [0, +65535] shouldn't overflow.
		const float maxDelayTime = 1.f;
		const float maxClockTicks = (maxDelayTime / APP->engine->getSampleTime()) / updateClocksFrequency;

		return 1 + std::round(maxClockTicks * params[COUNT_OR_DELAY_PARAM].getValue());
	}

	void process(const ProcessArgs& args) override {

		// process knobs
		{
			divider.value = divisionFromParam(params[DIVISION_PARAM].getValue());
			divcounter.value = counter.value = countFromParam(params[COUNT_OR_DELAY_PARAM].getValue());
			delay.value = swinger.value = delayFromParam(params[COUNT_OR_DELAY_PARAM].getValue());
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


		// do lights
		{
			lights[DIVISION_LIGHT].setBrightnessSmooth(!divider.isOff(), args.sampleTime);
			lights[COMBINED_LIGHT].setBrightnessSmooth((bool) outputs[COMBINED_OUTPUT].getVoltage(), args.sampleTime);

			switch (mode) {
				case DELAY_MODE:
					lights[COUNT_OR_DELAY_LIGHT].setBrightnessSmooth(!delay.isOff(), args.sampleTime);
					break;
				case COUNT_MODE:
					lights[COUNT_OR_DELAY_LIGHT].setBrightnessSmooth((bool) outputs[ADDITION_DELAY_OUTPUT].getVoltage(), args.sampleTime);
					break;
				case DISABLED_MODE:
					outputs[ADDITION_DELAY_OUTPUT].setVoltage(0.f);
					break;
			}
		}
	}
};


struct LogoiWidget : ModuleWidget {
	LogoiWidget(Logoi* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Logoi.svg")));

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