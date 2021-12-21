#include "plugin.hpp"



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
public:
	uint8_t pos;
	uint8_t value;
	bool state;
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
		return !state;
	}
	virtual void on() {
		state = true;
	}
	virtual void off() {
		state = false;
	}
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
public:
	uint8_t pos;
	int8_t value;
	bool toggled;
	bool state;
	inline bool isOff() {
		return !state;
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
		state = !state;
	}
	void on() {
		state = true;
	}
	void off() {
		state = false;
	}
};

class ClockDelay {
public:
	uint16_t riseMark;
	uint16_t fallMark;
	uint16_t value;
	bool state;
	volatile uint16_t pos;
	volatile bool running;
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
		state = true;
	}
	virtual void off() {
		state = false;
	}
	virtual bool isOff() {
		return !state;
	}
};

class ClockSwing : public ClockDelay {
private:
	bool stateSwing;
public:
	void on() {
		stateSwing = true;
	}
	void off() {
		stateSwing = false;
	}
	bool isOff() {
		return !stateSwing;
	}
};

class DividingCounter : public ClockCounter {

private:
	bool stateDiv;

public:
	void on() {
		stateDiv = true;
	}
	void off() {
		stateDiv = false;
	}
	bool isOff() {
		return !stateDiv;
	}
};

struct Logoi : Module {
	enum ParamId {
		DIV_PARAM,
		PLUS_PARAM,
		DIV_CV_PARAM,
		PLUS_CV_PARAM,
		MODE_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		DIV_CV_INPUT,
		PLUS_CV_INPUT,
		RESET_INPUT,
		DIVISION_CLOCK_INPUT,
		ADDITION_CLOCK_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		DIVISION_OUTPUT,
		ADDITION_OUTPUT,
		COMBINED_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		PATH2680_41_LIGHT,
		PATH2680_6_LIGHT,
		PATH2680_41_2_LIGHT,
		LIGHTS_LEN
	};

	dsp::SchmittTrigger riseDetector[3];
	dsp::SchmittTrigger fallDetector[3];

	ClockCounter counter;
	ClockDivider divider;
	ClockDelay delay; // manually triggered from Timer0 interrupt
	ClockSwing swinger;
	DividingCounter divcounter;

	enum OperatingMode {
		DIVIDE_MODE,
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
		auto divParam = configParam(DIV_PARAM, 0.f, 32.f, 0.f, "");
		divParam->snapEnabled = true;
		configParam(PLUS_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DIV_CV_PARAM, 0.f, 1.f, 0.f, "");
		configParam(PLUS_CV_PARAM, 0.f, 1.f, 0.f, "");
		configSwitch(MODE_PARAM, 0.f, 2.f, 0.f, "Mode", {"Count", "Delay", "Off"});
		configInput(DIV_CV_INPUT, "");
		configInput(PLUS_CV_INPUT, "");
		configInput(RESET_INPUT, "");
		configInput(DIVISION_CLOCK_INPUT, "");
		configInput(ADDITION_CLOCK_INPUT, "");
		configOutput(DIVISION_OUTPUT, "");
		configOutput(ADDITION_OUTPUT, "");
		configOutput(COMBINED_OUTPUT, "");
	}

	void process(const ProcessArgs& args) override {

		divider.value = std::round(params[DIV_PARAM].getValue() - 1);

		// can get a normalled copy of right half clock if nothing is connected to left clock
		const float normalledClockLeft = inputs[DIVISION_CLOCK_INPUT].getNormalVoltage(inputs[ADDITION_CLOCK_INPUT].getVoltage()));
		if (riseDetector[0].process(normalledClockLeft) {
			divider.rise();
		}
		else if (fallDetector[0].process(normalledClockLeft)) {
			divider.fall();
		}
		outputs[DIVISION_OUTPUT].setVoltage(10.f * !divider.isOff());

		
		// can get a normalled copy of left half clock if nothing is connected to right clock
		const float normalledClockRight = inputs[ADDITION_CLOCK_INPUT].getNormalVoltage(inputs[DIVISION_CLOCK_INPUT].getVoltage()));
		switch (params[MODE_PARAM].getValue()) {
			case DIVIDE_MODE: {

				break;
			}
		}

		
		if (riseDetector[0].process(normalledClockLeft) {
			divider.rise();
		}
		else if (fallDetector[0].process(normalledClockLeft)) {
			divider.fall();
		}
		outputs[DIVISION_OUTPUT].setVoltage(10.f * !divider.isOff());

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

		addParam(createParamCentered<SynthTechAlco>(mm2px(Vec(12.533, 26.112)), module, Logoi::DIV_PARAM));
		addParam(createParamCentered<SynthTechAlco>(mm2px(Vec(37.933, 26.112)), module, Logoi::PLUS_PARAM));
		addParam(createParamCentered<SynthTechAlco>(mm2px(Vec(12.533, 45.162)), module, Logoi::DIV_CV_PARAM));
		addParam(createParamCentered<SynthTechAlco>(mm2px(Vec(37.933, 45.162)), module, Logoi::PLUS_CV_PARAM));
		addParam(createParamCentered<BefacoSwitch>(mm2px(Vec(25.225, 70.48)), module, Logoi::MODE_PARAM));

		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(12.525, 83.18)), module, Logoi::DIV_CV_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(37.925, 83.18)), module, Logoi::PLUS_CV_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(25.225, 95.88)), module, Logoi::RESET_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(12.525, 108.58)), module, Logoi::DIVISION_CLOCK_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(37.925, 108.58)), module, Logoi::ADDITION_CLOCK_INPUT));

		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(12.525, 95.88)), module, Logoi::DIVISION_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(37.925, 95.88)), module, Logoi::ADDITION_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(25.225, 108.58)), module, Logoi::COMBINED_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(25.225, 57.78)), module, Logoi::PATH2680_41_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(12.525, 70.48)), module, Logoi::PATH2680_6_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(37.975, 70.625)), module, Logoi::PATH2680_41_2_LIGHT));
	}
};


Model* modelLogoi = createModel<Logoi, LogoiWidget>("Logoi");