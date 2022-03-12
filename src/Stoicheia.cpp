
#include "plugin.hpp"
#include "Sequence.h"

struct Stoicheia : Module {
	enum ParamIds {
		START_A_PARAM,
		START_B_PARAM,
		LENGTH_A_PARAM,
		LENGTH_B_PARAM,
		DENSITY_A_PARAM,
		DENSITY_B_PARAM,
		AB_MODE,
		MODE_A_PARAM,
		MODE_B_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		RESET_INPUT,
		CLOCK_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT_A_OUTPUT,
		OUT_B_OUTPUT,
		CLOCK_THRU,
		NUM_OUTPUTS
	};
	enum LightIds {
		A_AND_B_LIGHT,
		A_LIGHT,
		B_LIGHT,
		NUM_LIGHTS
	};


	enum ABMode {
		INDEPENDENT,
		ALTERNATING
	};

	struct FillParam : ParamQuantity {
		// effective number of fills will depend on on the sequence length
		std::string getDisplayValueString() override {
			if (module != nullptr) {
				if (paramId == DENSITY_A_PARAM) {
					int fillValue = paramToFill(getValue(), module->params[LENGTH_A_PARAM].getValue());
					return std::to_string(fillValue);
				}
				else if (paramId == DENSITY_B_PARAM) {
					int fillValue = paramToFill(getValue(), module->params[LENGTH_B_PARAM].getValue());
					return std::to_string(fillValue);
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
			float fill = std::atof(s.c_str());
			if (module != nullptr) {
				if (paramId == DENSITY_A_PARAM) {
					int lengthA = module->params[LENGTH_A_PARAM].getValue();
					ParamQuantity::setValue(fillToParam(fill, lengthA));
				}
				else if (paramId == DENSITY_B_PARAM) {
					int lengthB = module->params[LENGTH_B_PARAM].getValue();
					ParamQuantity::setValue(fillToParam(fill, lengthB));
				}
				else {
					assert(false);
				}
			}
		}
	};

	struct OffsetParam : ParamQuantity {
		// effective offset will depend on on the sequence length
		std::string getDisplayValueString() override {
			if (module != nullptr) {
				if (paramId == START_A_PARAM) {
					int offset = paramToOffset(getValue(), module->params[LENGTH_A_PARAM].getValue());
					return std::to_string(offset);
				}
				else if (paramId == START_B_PARAM) {
					int offset = paramToOffset(getValue(), module->params[LENGTH_B_PARAM].getValue());
					return std::to_string(offset);
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
			float offset = std::atof(s.c_str());
			if (module != nullptr) {
				if (paramId == START_A_PARAM) {
					int lengthA = module->params[LENGTH_A_PARAM].getValue();
					ParamQuantity::setValue(offsetToParam(offset, lengthA));
				}
				else if (paramId == START_B_PARAM) {
					int lengthB = module->params[LENGTH_B_PARAM].getValue();
					ParamQuantity::setValue(offsetToParam(offset, lengthB));
				}
				else {
					assert(false);
				}
			}
		}
	};

	struct ABModeParam : ParamQuantity {
		std::string getDisplayValueString() override {
			switch (static_cast<ABMode>(getValue())) {
				case INDEPENDENT: return "Independent A and B";
				case ALTERNATING: return "Alternating A then B";
				default: assert(false);
			}
		}
	};

	enum SequenceMode {
		LATCHED,
		MUTE,
		NORMAL
	};

	struct SequenceParams {
		int length = -1;
		int fill = -1;
		int start = -1;
		SequenceMode mode;
	};

	// uint16_t for 16 steps
	Sequence<uint16_t> seq[2];
	dsp::SchmittTrigger triggers[NUM_INPUTS];
	bool states[2] = {false};
	int activeSequence = 0;
	int combinedSequencePosition = 0;
	SequenceParams oldA, currentA, oldB, currentB;
	ModuleTheme theme = LIGHT_THEME;

	Stoicheia() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam<OffsetParam>(START_A_PARAM, 0.f, 1.f, 0.f, "Offset A");
		configParam<OffsetParam>(START_B_PARAM, 0.f, 1.f, 0.f, "Offset B");

		auto lengthA = configParam(LENGTH_A_PARAM, 1.f, 16.f, 1.f, "Length A");
		lengthA->snapEnabled = true;
		auto lengthB = configParam(LENGTH_B_PARAM, 1.f, 16.f, 1.f, "Length B");
		lengthB->snapEnabled = true;

		configParam<FillParam>(DENSITY_A_PARAM, 0.f, 1.f, 0.5f, "Fill density A");
		configParam<FillParam>(DENSITY_B_PARAM, 0.f, 1.f, 0.5f, "Fill density B");
		configParam<ABModeParam>(AB_MODE, INDEPENDENT, ALTERNATING, INDEPENDENT, "Sequence mode");
		configSwitch(MODE_A_PARAM, LATCHED, NORMAL, NORMAL, "Mode A", {"Alternating", "Mute", "Trigger"});
		configSwitch(MODE_B_PARAM, LATCHED, NORMAL, NORMAL, "Mode B", {"Alternating", "Mute", "Trigger"});

		configInput(CLOCK_INPUT, "Clock");
		configInput(RESET_INPUT, "Reset");

		configOutput(OUT_A_OUTPUT, "Sequence A");
		configOutput(OUT_B_OUTPUT, "Sequence B");
		configOutput(CLOCK_THRU, "Clock thru");

		seq[0].offset = 0;
		seq[0].calculate(12, 8);

		seq[1].offset = 0;
		seq[1].calculate(12, 8);
	}

	void processBypass(const ProcessArgs& args) override {
		triggers[CLOCK_INPUT].process(rescale(inputs[CLOCK_INPUT].getVoltage(), 0.1f, 2.f, 0.f, 1.f));
		outputs[OUT_A_OUTPUT].setVoltage(triggers[CLOCK_INPUT].isHigh() * 10.f);
		outputs[OUT_B_OUTPUT].setVoltage(triggers[CLOCK_INPUT].isHigh() * 10.f);
		outputs[CLOCK_THRU].setVoltage(triggers[CLOCK_INPUT].isHigh() * 10.f);
	}

	void process(const ProcessArgs& args) override {

		if (triggers[RESET_INPUT].process(rescale(inputs[RESET_INPUT].getVoltage(), 0.1f, 2.f, 0.f, 1.f))) {
			seq[0].reset();
			seq[1].reset();
			combinedSequencePosition = 0;
		}

		ABMode mode = static_cast<ABMode>(params[AB_MODE].getValue());

		// update params of sequence A (if changed)
		currentA.length = params[LENGTH_A_PARAM].getValue();
		currentA.fill = paramToFill(params[DENSITY_A_PARAM].getValue(), currentA.length);
		currentA.start = paramToOffset(params[START_A_PARAM].getValue(), currentA.length);
		currentA.mode = static_cast<SequenceMode>(params[MODE_A_PARAM].getValue());
		if (currentA.length != oldA.length || currentA.fill != oldA.fill) {
			seq[0].calculate(currentA.length, currentA.fill);
		}
		if (currentA.start != oldA.start) {
			seq[0].rotate(currentA.start);
		}
		// update params of sequence B (if changed)
		currentB.length = params[LENGTH_B_PARAM].getValue();
		currentB.fill = paramToFill(params[DENSITY_B_PARAM].getValue(), currentB.length);
		currentB.start = paramToOffset(params[START_B_PARAM].getValue(), currentB.length);
		currentB.mode = static_cast<SequenceMode>(params[MODE_B_PARAM].getValue());
		if (currentB.length != oldB.length || currentB.fill != oldB.fill) {
			seq[1].calculate(currentB.length, currentB.fill);
		}
		if (currentB.start != oldB.start) {
			seq[1].rotate(currentB.start);
		}

		const bool risingEdge = triggers[CLOCK_INPUT].process(rescale(inputs[CLOCK_INPUT].getVoltage(), 0.1f, 2.f, 0.f, 1.f));
		const float clockIn = 10.f * triggers[CLOCK_INPUT].isHigh();

		float outA = 0.f, outB = 0.f;
		if (mode == INDEPENDENT) {
			if (risingEdge) {
				states[0] = seq[0].next();
				states[1] = seq[1].next();
			}

			if (currentA.mode == NORMAL) {
				outA = states[0] * clockIn;
			}
			else if (currentA.mode == LATCHED) {
				outA = states[0] * 10.f;
			}
			else {
				outA = 0.f;
			}

			if (currentB.mode == NORMAL) {
				outB = states[1] * clockIn;
			}
			else if (currentB.mode == LATCHED) {
				outB = states[1] * 10.f;
			}
			else {
				outB = 0.f;
			}

		}
		else if (mode == ALTERNATING) {

			if (risingEdge) {

				if (++combinedSequencePosition >= seq[0].length + seq[1].length) {
					combinedSequencePosition = 0;
				}
				if (combinedSequencePosition < seq[0].length) {
					activeSequence = 0;
					states[activeSequence] = seq[activeSequence].next();
				}
				else {
					activeSequence = 1;
					states[activeSequence] = seq[activeSequence].next();
				}
			}

			if (currentA.mode == NORMAL) {
				outA = states[activeSequence] * clockIn;
			}
			else if (currentA.mode == LATCHED) {
				outA = states[activeSequence] * 10.f;
			}
			else {
				outA = 0.f;
			}
			if (currentB.mode == NORMAL) {
				outB = states[activeSequence] * clockIn;
			}
			else if (currentB.mode == LATCHED) {
				outB = states[activeSequence] * 10.f;
			}
			else {
				outB = 0.f;
			}

		}

		outputs[OUT_A_OUTPUT].setVoltage(outA);
		lights[A_LIGHT].setBrightness(outA / 10.f);
		oldA = currentA;

		outputs[OUT_B_OUTPUT].setVoltage(outB);
		lights[B_LIGHT].setBrightness(outB / 10.f);
		oldB = currentB;

		lights[A_AND_B_LIGHT].setBrightness(clockIn / 10.f);
		outputs[CLOCK_THRU].setVoltage(triggers[CLOCK_INPUT].isHigh() * 10.f);

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



struct StoicheiaWidget : ModuleWidget {
	StoicheiaWidget(Stoicheia* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/Stoicheia.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RebelTechPot>(mm2px(Vec(12.569, 26.174)), module, Stoicheia::START_A_PARAM));
		addParam(createParamCentered<RebelTechPot>(mm2px(Vec(37.971, 26.174)), module, Stoicheia::START_B_PARAM));
		addParam(createParamCentered<RebelTechPot>(mm2px(Vec(12.569, 45.374)), module, Stoicheia::LENGTH_A_PARAM));
		addParam(createParamCentered<RebelTechPot>(mm2px(Vec(37.971, 45.374)), module, Stoicheia::LENGTH_B_PARAM));
		addParam(createParamCentered<RebelTechPot>(mm2px(Vec(12.569, 64.574)), module, Stoicheia::DENSITY_A_PARAM));
		addParam(createParamCentered<RebelTechPot>(mm2px(Vec(37.971, 64.574)), module, Stoicheia::DENSITY_B_PARAM));
		addParam(createParamCentered<BefacoSwitch>(mm2px(Vec(25.275, 83.326)), module, Stoicheia::AB_MODE));
		addParam(createParamCentered<BefacoSwitch>(mm2px(Vec(12.347, 96.026)), module, Stoicheia::MODE_A_PARAM));
		addParam(createParamCentered<BefacoSwitch>(mm2px(Vec(37.976, 96.026)), module, Stoicheia::MODE_B_PARAM));

		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(25.275, 96.026)), module, Stoicheia::RESET_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(6.224, 108.712)), module, Stoicheia::CLOCK_INPUT));

		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(18.925, 108.712)), module, Stoicheia::OUT_A_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(31.625, 108.712)), module, Stoicheia::OUT_B_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(44.326, 108.712)), module, Stoicheia::CLOCK_THRU));

		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(25.275, 70.625)), module, Stoicheia::A_AND_B_LIGHT));
		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(12.574, 83.308)), module, Stoicheia::A_LIGHT));
		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(37.976, 83.326)), module, Stoicheia::B_LIGHT));
	}
};


Model* modelStoicheia = createModel<Stoicheia, StoicheiaWidget>("Stoicheia");