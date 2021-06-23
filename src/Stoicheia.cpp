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
		A_PLUS_B_PARAM,
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

	Stoicheia() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(START_A_PARAM, 0.f, 15.f, 0.f, "");
		configParam(START_B_PARAM, 0.f, 15.f, 0.f, "");
		configParam(LENGTH_A_PARAM, 1.f, 16.f, 0.f, "");
		configParam(LENGTH_B_PARAM, 1.f, 16.f, 0.f, "");
		configParam(DENSITY_A_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DENSITY_B_PARAM, 0.f, 1.f, 0.f, "");
		configParam(A_PLUS_B_PARAM, 0.f, 1.f, 0.f, "");
		configParam(MODE_A_PARAM, 0.f, NORMAL, NORMAL, "Mode A: Latched/Mute/Normal");
		configParam(MODE_B_PARAM, 0.f, NORMAL, NORMAL, "Mode B: Latched/Mute/Normal");

		seq[0].offset = 0;
		seq[0].calculate(12, 8);

		seq[1].offset = 0;
		seq[1].calculate(12, 8);
	}

	Sequence<uint16_t> seq[2];
	dsp::SchmittTrigger triggers[NUM_INPUTS];
	bool states[2] = {false};

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

	SequenceParams oldA, currentA, oldB, currentB;

	void process(const ProcessArgs& args) override {

		if (triggers[RESET_INPUT].process(rescale(inputs[RESET_INPUT].getVoltage(), 0.1f, 2.f, 0.f, 1.f))) {
			seq[0].reset();
			seq[1].reset();
		}

		// TODO: normalling

		currentA.length = params[LENGTH_A_PARAM].getValue();
		currentA.fill = currentA.length * params[DENSITY_A_PARAM].getValue();
		currentA.start = params[START_A_PARAM].getValue();
		currentA.mode = static_cast<SequenceMode>(params[MODE_A_PARAM].getValue());
	
		if (currentA.length != oldA.length || currentA.fill != oldA.fill) {
			seq[0].calculate(currentA.length, currentA.fill);
		}
		if (currentA.start != oldA.start) {
			seq[0].rotate(currentA.start);
		}
		
		if (triggers[IN_A_INPUT].process(rescale(inputs[IN_A_INPUT].getVoltage(), 0.1f, 2.f, 0.f, 1.f))) {
			states[0] = seq[0].next();
			//DEBUG(("state A:" + std::to_string(seq[0].pos) + " " + std::to_string(states[0])).c_str());
		}
		float outA;
		if (currentA.mode == NORMAL) {
			outA = states[0] * inputs[IN_A_INPUT].getVoltage();
		} else if (currentA.mode == LATCHED) {
			outA = states[0] * 10.f;
		}
		else {
			outA = 0.f;			
		}
		outputs[OUT_A_OUTPUT].setVoltage(outA);
		lights[A_LIGHT].setBrightness(outA/10.f);
		oldA = currentA;

		currentB.length = params[LENGTH_B_PARAM].getValue();
		currentB.fill = currentB.length * params[DENSITY_B_PARAM].getValue();
		currentB.start = params[START_B_PARAM].getValue();
		currentB.mode = static_cast<SequenceMode>(params[MODE_B_PARAM].getValue());
		if (currentB.length != oldB.length || currentB.fill != oldB.fill) {
			seq[1].calculate(currentB.length, currentB.fill);
		}
		if (currentB.start != oldB.start) {
			seq[1].rotate(currentB.start);
		}
		//DEBUG("here2");
		if (triggers[IN_B_INPUT].process(rescale(inputs[IN_B_INPUT].getVoltage(), 0.1f, 2.f, 0.f, 1.f))) {
			states[1] = seq[1].next();
			//DEBUG(("state B:" + std::to_string(seq[1].pos) + " " + std::to_string(states[1])).c_str());
		}
		float outB;
		if (currentB.mode == NORMAL) {
			outB = states[1] * inputs[IN_B_INPUT].getVoltage();
		} else if (currentB.mode == LATCHED) {
			outB = states[1] * 10.f;
		}
		else {
			outB = 0.f;
		}
		outputs[OUT_B_OUTPUT].setVoltage(outB);
		lights[B_LIGHT].setBrightness(outB/10.f);
		oldB = currentB;
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
		addParam(createParamCentered<BefacoSwitch>(mm2px(Vec(25.391, 83.054)), module, Stoicheia::A_PLUS_B_PARAM));
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