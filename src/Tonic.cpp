#include "plugin.hpp"
#include "Common.hpp"

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
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Tonic.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Davies1900hWhiteKnobSnap>(mm2px(Vec(15.02, 18.431)), module, Tonic::SCALE_PARAM));
		addParam(createParamCentered<BefacoPush>(mm2px(Vec(22.645, 32.525)), module, Tonic::BUTTON + 0));
		addParam(createParamCentered<BefacoPush>(mm2px(Vec(22.645, 45.225)), module, Tonic::BUTTON + 1));
		addParam(createParamCentered<BefacoPush>(mm2px(Vec(22.645, 57.925)), module, Tonic::BUTTON + 2));
		addParam(createParamCentered<BefacoPush>(mm2px(Vec(22.645, 70.625)), module, Tonic::BUTTON + 3));
		addParam(createParamCentered<BefacoPush>(mm2px(Vec(22.645, 83.325)), module, Tonic::BUTTON + 4));
		addParam(createParamCentered<BefacoPush>(mm2px(Vec(22.645, 96.025)), module, Tonic::BUTTON + 5));

		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(7.451, 32.525)), module, Tonic::GATE_INPUT + 0));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(7.451, 45.225)), module, Tonic::GATE_INPUT + 1));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(7.451, 57.925)), module, Tonic::GATE_INPUT + 2));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(7.451, 70.625)), module, Tonic::GATE_INPUT + 3));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(7.451, 83.325)), module, Tonic::GATE_INPUT + 4));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(7.451, 96.025)), module, Tonic::GATE_INPUT + 5));

		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(7.451, 108.725)), module, Tonic::OUT_A_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(22.645, 108.725)), module, Tonic::OUT_B_OUTPUT));

		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(15.025, 32.493)), module, Tonic::LED + 0));
		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(15.025, 45.196)), module, Tonic::LED + 1));
		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(15.025, 57.899)), module, Tonic::LED + 2));
		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(15.025, 70.602)), module, Tonic::LED + 3));
		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(15.025, 83.304)), module, Tonic::LED + 4));
		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(15.025, 96.007)), module, Tonic::LED + 5));
	}
};


Model* modelTonic = createModel<Tonic, TonicWidget>("Tonic");