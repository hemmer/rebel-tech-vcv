#include "plugin.hpp"

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)<(b)?(b):(a))

// Master clock runs using floating point calculations to be as accurate as possible
typedef float MasterClockTick;
// subclocks (subdivisions of master) just use integer counting
typedef uint16_t SubClockTick;


const SubClockTick B_MULTIPLIERS[] = { 48 * 8, 48 * 6, 48 * 4, 48 * 3, 48 * 2, 48 * 1, 48 / 2, 48 / 3, 48 / 4, 48 / 6, 48 / 8 };
const SubClockTick C_MULTIPLIERS[] = { 48 * 24, 48 * 12, 48 * 8, 48 * 4, 48 * 2, 48 * 1, 48 / 2, 48 / 4, 48 / 8, 48 / 12, 48 / 24 };

const std::string B_STRINGS[] = { "/8", "/6", "/4", "/3", "/2", "x1", "x2", "x3", "x4", "x6", "x8" };
const std::string C_STRINGS[] = { "/24", "/12", "/8", "/4", "/2", "x1", "x2", "x4", "x8", "x12", "x24" };

class MasterClock;


template<bool CAN_RESET_MASTER_CLOCK>
class ClockGenerator {
private:
	bool state = false;
	MasterClock* master;
public:
	ClockGenerator(MasterClock* master_) {
		master = master_;
	}

	SubClockTick period = 0;
	SubClockTick duty = 0;
	SubClockTick pos = 0;
	void setPeriod(SubClockTick ticks, SubClockTick maxDuty) {
		duty = min(maxDuty, ticks / 2);
		period = ticks - 1;
	}
	void resetPhase() {
		pos = -1;
		on();
	}
	void clock() {
		if (++pos >= duty) {
			off();
		}
		if (pos > period) {
			pos = 0;
			on();
		}
	}
	void on();
	void off() {
		state = false;
	}
	bool isOn() {
		return state;
	}

};

class MasterClock {
private:
public:
	bool resetB = false;
	bool resetC = false;

	ClockGenerator<true> clockA;
	ClockGenerator<false> clockB;
	ClockGenerator<false> clockC;

	MasterClock() : clockA(this), clockB(this), clockC(this) {

	}

	MasterClockTick period = 0.f;
	MasterClockTick pos = 0.f;

	// clock period in seconds
	void setPeriod(MasterClockTick period_) {
		period = period_;
	}
	void clock(float sampleTime) {
		pos += sampleTime;
		if (pos > period) {
			pos -= period;
			clockA.clock();
			clockB.clock();
			clockC.clock();
		}
	}
	void reset() {
		if (resetB) {
			clockB.resetPhase();
			resetB = false;
		}
		if (resetC) {
			clockC.resetPhase();
			resetC = false;
		}
	}
};

template<> void ClockGenerator<false>::on() {
	state = true;
}
template<> void ClockGenerator<true>::on() {
	state = true;
	if (master)
		master->reset();
}


struct CLK : Module {
	enum ParamId {
		BPM_PARAM,
		SCALE_8_PARAM,
		SCALE_24_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		INPUTS_LEN
	};
	enum OutputId {
		MAIN_OUTPUT,
		CLOCK_8_OUTPUT,
		CLOCK_24_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		MAIN_LIGHT,
		CLOCK_8_LIGHT,
		CLOCK_24_LIGHT,
		LIGHTS_LEN
	};
	enum TriggerMode {
		TRIGGER_MODE,
		GATE_MODE,
		ORIGINAL_MODE,
		OUTPUT_MODE_LEN
	};

	MasterClock master;

	SubClockTick mulB = 5;
	SubClockTick mulC = 5;

	int modifier8Cached = 0, modifier24Cached = 0;
	int outputMultiplier = 0;
	TriggerMode triggerMode = ORIGINAL_MODE;

	struct Scale8ParamQuantity : ParamQuantity {
		std::string getDisplayValueString() override {
			int index = getValue();
			return B_STRINGS[clamp(index, 0, 10)];
		}
	};
	struct Scale24ParamQuantity : ParamQuantity {
		std::string getDisplayValueString() override {
			int index = getValue();
			return C_STRINGS[clamp(index, 0, 10)];
		}
	};
	ModuleTheme theme = LIGHT_THEME;

	CLK() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(BPM_PARAM, 40.f, 200.f, 120.f, "BPM");
		configParam<Scale8ParamQuantity>(SCALE_8_PARAM, 0, 10, 5.f, "Multiplication/division");
		configParam<Scale24ParamQuantity>(SCALE_24_PARAM, 0.f, 10.f, 5.f, "Multiplication/division");
		configOutput(MAIN_OUTPUT, "Main clock");
		configOutput(CLOCK_8_OUTPUT, "Multiplied/divided clock #1");
		configOutput(CLOCK_24_OUTPUT, "Multiplied/divided clock #2");
	}

	void process(const ProcessArgs& args) override {

		const SubClockTick b = params[SCALE_8_PARAM].getValue();
		if (b != mulB) {
			mulB = b;
			master.resetB = true;
		}
		const SubClockTick c = params[SCALE_24_PARAM].getValue();
		if (c != mulC) {
			mulC = c;
			master.resetC = true;
		}

		// context menu allows x1, x2, x4, x8, x16 - this applies that factor
		const uint32_t scale = (1 << outputMultiplier);
		// length of a tick of the master clock (which runs at x48 to make mult/division easier,
		// and which includes above scale)
		const float tickTime = 1. / (scale * 48. * params[BPM_PARAM].getValue() / 60.);

		// master clock, running at 48x intended BPM
		master.setPeriod(tickTime);
		float maxDuty;
		switch (triggerMode) {
			case ORIGINAL_MODE:
				maxDuty = 48 >> 1;
				break;
			case GATE_MODE:
				maxDuty = INT16_MAX;
				break;
			case TRIGGER_MODE:
			default:
				maxDuty = max(1, (1e-3 / tickTime) / 48);
				break;
		}

		// A ticks every 48 master clock ticks
		master.clockA.setPeriod(48, maxDuty);
		master.clockB.setPeriod(B_MULTIPLIERS[b], maxDuty);
		master.clockC.setPeriod(C_MULTIPLIERS[c], maxDuty);

		master.clock(args.sampleTime);

		outputs[MAIN_OUTPUT].setVoltage(10.f * master.clockA.isOn());
		outputs[CLOCK_8_OUTPUT].setVoltage(10.f * master.clockB.isOn());
		outputs[CLOCK_24_OUTPUT].setVoltage(10.f * master.clockC.isOn());

		lights[MAIN_LIGHT].setBrightnessSmooth(master.clockA.isOn(), args.sampleTime);
		lights[CLOCK_8_LIGHT].setBrightnessSmooth(master.clockB.isOn(), args.sampleTime);
		lights[CLOCK_24_LIGHT].setBrightnessSmooth(master.clockC.isOn(), args.sampleTime);
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* multiplier = json_object_get(rootJ, "multiplier");
		if (multiplier) {
			outputMultiplier = json_integer_value(multiplier);
		}
		json_t* modeJ = json_object_get(rootJ, "mode");
		if (modeJ) {
			triggerMode = (TriggerMode)json_integer_value(modeJ);
		}
		json_t* themeJ = json_object_get(rootJ, "theme");
		if (themeJ) {
			theme = (ModuleTheme) json_integer_value(themeJ);
		}
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "multiplier", json_integer(outputMultiplier));
		json_object_set_new(rootJ, "theme", json_integer(theme));
		json_object_set_new(rootJ, "mode", json_integer(triggerMode));

		return rootJ;
	}
};


struct CLKWidget : ModuleWidget {
	CLKWidget(CLK* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/CLK.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Davies1900hWhiteKnobSnap>(mm2px(Vec(8.984, 19.8)), module, CLK::BPM_PARAM));
		addParam(createParamCentered<Davies1900hWhiteKnobSnap>(mm2px(Vec(8.984, 38.85)), module, CLK::SCALE_8_PARAM));
		addParam(createParamCentered<Davies1900hWhiteKnobSnap>(mm2px(Vec(8.984, 57.9)), module, CLK::SCALE_24_PARAM));

		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(10.0, 79.066666)), module, CLK::MAIN_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(10.0, 94.941666)), module, CLK::CLOCK_8_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(10.0, 110.81666)), module, CLK::CLOCK_24_OUTPUT));

		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(8.222, 71.4466)), module, CLK::MAIN_LIGHT));
		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(8.222, 87.321671)), module, CLK::CLOCK_8_LIGHT));
		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(8.222, 103.19668)), module, CLK::CLOCK_24_LIGHT));
	}

	void appendContextMenu(Menu* menu) override {
		CLK* module = dynamic_cast<CLK*>(this->module);
		assert(module);

		menu->addChild(createIndexPtrSubmenuItem("Output multiplier",	{"x1", "x2", "x4", "x8", "x16"}, &module->outputMultiplier));
		menu->addChild(createIndexPtrSubmenuItem("Trigger mode", {"Trigger", "Gate", "Original"}, &module->triggerMode));
	}
};


Model* modelCLK = createModel<CLK, CLKWidget>("CLK");
