#include "plugin.hpp"
#include "Sequence.h"

struct Davies1900hWhiteKnobSnap : Davies1900hWhiteKnob {
	Davies1900hWhiteKnobSnap() {
		snap = true;
		minAngle = -0.75 * M_PI;
		maxAngle = 0.75 * M_PI;
	}
};


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
		IN_A_INPUT,
		IN_B_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT_A_OUTPUT,
		OUT_B_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		A_AND_B_LIGHT,
		A_LIGHT,
		B_LIGHT,
		NUM_LIGHTS
	};

	enum SequenceMode {
		LATCHED,
		MUTE,
		NORMAL
	};

	struct SequenceParams {
		int length;
		int fill;
		int start;
		SequenceMode mode;
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
					int lengthA = module->params[LENGTH_A_PARAM].getValue();
					int fillValue = std::round(getValue() * lengthA);
					return std::to_string(fillValue);
				}
				else if (paramId == DENSITY_B_PARAM) {
					int lengthB = module->params[LENGTH_B_PARAM].getValue();
					int fillValue = std::round(getValue() * lengthB);
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
	};

	struct ModeParam : ParamQuantity {
		std::string getDisplayValueString() override {
			switch (static_cast<SequenceMode>(getValue())) {
				case MUTE: return "Mute";
				case LATCHED: return "Latched";
				case NORMAL: return "Normal";
				default: assert(false);
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


	Stoicheia() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(START_A_PARAM, 0.f, 15.f, 0.f, "Offset A");
		configParam(START_B_PARAM, 0.f, 15.f, 0.f, "Offset B");
		configParam(LENGTH_A_PARAM, 1.f, 16.f, 0.f, "Length A");
		configParam(LENGTH_B_PARAM, 1.f, 16.f, 0.f, "Length B");
		configParam<FillParam>(DENSITY_A_PARAM, 0.f, 1.f, 0.f, "Fill density A");
		configParam<FillParam>(DENSITY_B_PARAM, 0.f, 1.f, 0.f, "Fill density B");
		configParam<ABModeParam>(AB_MODE, INDEPENDENT, ALTERNATING, INDEPENDENT, "Sequence mode");
		configParam<ModeParam>(MODE_A_PARAM, 0.f, NORMAL, NORMAL, "Mode A");
		configParam<ModeParam>(MODE_B_PARAM, 0.f, NORMAL, NORMAL, "Mode B");

		seq[0].offset = 0;
		seq[0].calculate(12, 8);

		seq[1].offset = 0;
		seq[1].calculate(12, 8);
	}

	Sequence<uint16_t> seq[2];
	dsp::SchmittTrigger triggers[NUM_INPUTS];
	bool states[2] = {false};
	int activeSequence = 0;
	int combinedSequencePosition = 0;
	SequenceParams oldA, currentA, oldB, currentB;

	void process(const ProcessArgs& args) override {

		if (triggers[RESET_INPUT].process(rescale(inputs[RESET_INPUT].getVoltage(), 0.1f, 2.f, 0.f, 1.f))) {
			seq[0].reset();
			seq[1].reset();
			combinedSequencePosition = 0;
		}

		ABMode mode = static_cast<ABMode>(params[AB_MODE].getValue());

		// update params of sequence A (if changed)
		currentA.length = params[LENGTH_A_PARAM].getValue();
		currentA.fill = std::round(currentA.length * params[DENSITY_A_PARAM].getValue());
		currentA.start = params[START_A_PARAM].getValue();
		currentA.mode = static_cast<SequenceMode>(params[MODE_A_PARAM].getValue());
		if (currentA.length != oldA.length || currentA.fill != oldA.fill) {
			seq[0].calculate(currentA.length, currentA.fill);
		}
		if (currentA.start != oldA.start) {
			seq[0].rotate(currentA.start);
		}
		// update params of sequence B (if changed)
		currentB.length = params[LENGTH_B_PARAM].getValue();
		currentB.fill = std::round(currentB.length * params[DENSITY_B_PARAM].getValue());
		currentB.start = params[START_B_PARAM].getValue();
		currentB.mode = static_cast<SequenceMode>(params[MODE_B_PARAM].getValue());
		if (currentB.length != oldB.length || currentB.fill != oldB.fill) {
			seq[1].calculate(currentB.length, currentB.fill);
		}
		if (currentB.start != oldB.start) {
			seq[1].rotate(currentB.start);
		}

		const float inA = inputs[IN_A_INPUT].getVoltage();
		const float inB = inputs[IN_B_INPUT].getNormalVoltage(inA);
		float outA = 0.f, outB = 0.f;
		if (mode == INDEPENDENT) {
			if (triggers[IN_A_INPUT].process(rescale(inA, 0.1f, 2.f, 0.f, 1.f))) {
				states[0] = seq[0].next();
			}
			if (triggers[IN_B_INPUT].process(rescale(inB, 0.1f, 2.f, 0.f, 1.f))) {
				states[1] = seq[1].next();
			}

			if (currentA.mode == NORMAL) {
				outA = states[0] * inA;
			}
			else if (currentA.mode == LATCHED) {
				outA = states[0] * 10.f;
			}
			else {
				outA = 0.f;
			}

			if (currentB.mode == NORMAL) {
				outB = states[1] * inB;
			}
			else if (currentB.mode == LATCHED) {
				outB = states[1] * 10.f;
			}
			else {
				outB = 0.f;
			}

		}
		else if (mode == ALTERNATING) {

			if (triggers[IN_A_INPUT].process(rescale(inA, 0.1f, 2.f, 0.f, 1.f))) {

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
				outA = states[activeSequence] * inA;	// TODO: is inB ignore in this mode?
			}
			else if (currentA.mode == LATCHED) {
				outA = states[activeSequence] * 10.f;
			}
			else {
				outA = 0.f;
			}
			if (currentB.mode == NORMAL) {
				outB = states[activeSequence] * inA;	// TODO: is inB ignore in this mode?
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

		lights[A_AND_B_LIGHT].setBrightness(clamp(inA / 10.f + inB / 10.f, 0.f, 1.f));
	}
};


struct StoicheiaWidget : ModuleWidget {
	StoicheiaWidget(Stoicheia* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Stoicheia.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Davies1900hWhiteKnobSnap>(mm2px(Vec(11.117, 25.644)), module, Stoicheia::START_A_PARAM));
		addParam(createParamCentered<Davies1900hWhiteKnobSnap>(mm2px(Vec(38.249, 25.644)), module, Stoicheia::START_B_PARAM));
		addParam(createParamCentered<Davies1900hWhiteKnobSnap>(mm2px(Vec(11.117, 45.374)), module, Stoicheia::LENGTH_A_PARAM));
		addParam(createParamCentered<Davies1900hWhiteKnobSnap>(mm2px(Vec(38.249, 45.374)), module, Stoicheia::LENGTH_B_PARAM));
		addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(11.117, 65.103)), module, Stoicheia::DENSITY_A_PARAM));
		addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(38.249, 65.103)), module, Stoicheia::DENSITY_B_PARAM));
		addParam(createParamCentered<BefacoSwitch>(mm2px(Vec(25.391, 83.054)), module, Stoicheia::AB_MODE));
		addParam(createParamCentered<BefacoSwitch>(mm2px(Vec(11.76, 96.351)), module, Stoicheia::MODE_A_PARAM));
		addParam(createParamCentered<BefacoSwitch>(mm2px(Vec(37.885, 97.152)), module, Stoicheia::MODE_B_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(25.391, 96.351)), module, Stoicheia::RESET_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.746, 108.712)), module, Stoicheia::IN_A_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(43.632, 109.246)), module, Stoicheia::IN_B_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(18.843, 108.511)), module, Stoicheia::OUT_A_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(31.906, 108.731)), module, Stoicheia::OUT_B_OUTPUT));

		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(25.391, 70.515)), module, Stoicheia::A_AND_B_LIGHT));
		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(12.478, 83.054)), module, Stoicheia::A_LIGHT));
		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(37.819, 83.054)), module, Stoicheia::B_LIGHT));
	}
};


Model* modelStoicheia = createModel<Stoicheia, StoicheiaWidget>("Stoicheia");