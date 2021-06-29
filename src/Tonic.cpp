#include "plugin.hpp"


struct Tonic : Module {
	enum ParamIds {
		SCALE_PARAM,
		ENUMS(BUTTON, 6),
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(GATE_INPUT, 6),
		NUM_INPUTS
	};
	enum OutputIds {
		OUT_A_OUTPUT,
		OUT_B_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(LED, 6),
		NUM_LIGHTS
	};

	Tonic() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(SCALE_PARAM, -6.f, 12.f, 0.f, "Scale");
		for (int i = 0; i < ParamIds::BUTTON_LAST; ++i) {
			configParam(BUTTON + i, 0.f, 1.f, 0.f, "Button " + std::to_string(i + 1));
		}
	}

	bool states[6];
	dsp::SchmittTrigger triggers[6];
	static constexpr float semitone = 1.f / 12.f;    // one semitone is a 1/12 volt
	
	const int numSemitones[6] = {0, 16, 8, 4, 2, -1};

	void process(const ProcessArgs& args) override {

		bool globalState = false;
		float voltage = 0.f;
		for (int i = 0; i < ParamIds::BUTTON_LAST; ++i) {
			//rescale(inA, 0.1f, 2.f, 0.f, 1.f))
			triggers[i].process(inputs[GATE_INPUT + i].getVoltage());


			states[i] = params[BUTTON + i].getValue() || triggers[i].isHigh();

			if (i == 0) {
				int semitonesForScale = std::round(params[SCALE_PARAM].getValue());
				voltage += semitone * semitonesForScale * states[i];
			}
			else {
				voltage += semitone * numSemitones[i] * states[i];
			}

			globalState = globalState || states[i];
			lights[LED + i].setBrightness(states[i]);
		}

		outputs[OUT_A_OUTPUT].setVoltage(10.f * globalState);
		outputs[OUT_B_OUTPUT].setVoltage(voltage);
	}
};


struct TonicWidget : ModuleWidget {
	TonicWidget(Tonic* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Tonic.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(15.119, 16.82)), module, Tonic::SCALE_PARAM));
		addParam(createParamCentered<BefacoPush>(mm2px(Vec(22.641, 32.222)), module, Tonic::BUTTON + 0));
		addParam(createParamCentered<BefacoPush>(mm2px(Vec(22.641, 44.928)), module, Tonic::BUTTON + 1));
		addParam(createParamCentered<BefacoPush>(mm2px(Vec(22.641, 57.634)), module, Tonic::BUTTON + 2));
		addParam(createParamCentered<BefacoPush>(mm2px(Vec(22.641, 70.339)), module, Tonic::BUTTON + 3));
		addParam(createParamCentered<BefacoPush>(mm2px(Vec(22.641, 83.045)), module, Tonic::BUTTON + 4));
		addParam(createParamCentered<BefacoPush>(mm2px(Vec(22.641, 95.75)), module, Tonic::BUTTON + 5));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.938, 32.222)), module, Tonic::GATE_INPUT + 0));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.938, 44.928)), module, Tonic::GATE_INPUT + 1));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.938, 57.634)), module, Tonic::GATE_INPUT + 2));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.938, 70.339)), module, Tonic::GATE_INPUT + 3));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.938, 83.045)), module, Tonic::GATE_INPUT + 4));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.938, 95.75)), module, Tonic::GATE_INPUT + 5));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.938, 108.456)), module, Tonic::OUT_A_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(22.641, 108.456)), module, Tonic::OUT_B_OUTPUT));

		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(15.119, 32.222)), module, Tonic::LED + 0));
		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(15.119, 44.928)), module, Tonic::LED + 1));
		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(15.119, 57.634)), module, Tonic::LED + 2));
		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(15.119, 70.339)), module, Tonic::LED + 3));
		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(15.119, 83.045)), module, Tonic::LED + 4));
		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(15.119, 95.75)), module, Tonic::LED + 5));
	}
};


Model* modelTonic = createModel<Tonic, TonicWidget>("Tonic");