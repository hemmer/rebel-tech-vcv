#include "plugin.hpp"


struct Phoreo : Module {
	enum ParamId {
		MOD_PARAM,
		MOD_CV_PARAM,
		MUL_PARAM,
		MUL_CV_PARAM,
		REP_PARAM,
		REP_CV_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		MOD_TRIG_INPUT,
		MOD_CV_INPUT,
		MULT_TRIG_INPUT,
		MULT_CV_INPUT,
		REP_TRIG_INPUT,
		REP_CV_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		MOD_OUTPUT,
		MULT_OUTPUT,
		REP_OUT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		PATH3574_59_LIGHT,
		PATH3574_49_LIGHT,
		LIGHTS_LEN
	};

	Phoreo() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(MOD_PARAM, 0.f, 1.f, 0.f, "");
		configParam(MOD_CV_PARAM, 0.f, 1.f, 0.f, "");
		configParam(MUL_PARAM, 0.f, 1.f, 0.f, "");
		configParam(MUL_CV_PARAM, 0.f, 1.f, 0.f, "");
		configParam(REP_PARAM, 0.f, 1.f, 0.f, "");
		configParam(REP_CV_PARAM, 0.f, 1.f, 0.f, "");
		configInput(MOD_TRIG_INPUT, "");
		configInput(MOD_CV_INPUT, "");
		configInput(MULT_TRIG_INPUT, "");
		configInput(MULT_CV_INPUT, "");
		configInput(REP_TRIG_INPUT, "");
		configInput(REP_CV_INPUT, "");
		configOutput(MOD_OUTPUT, "");
		configOutput(MULT_OUTPUT, "");
		configOutput(REP_OUT_OUTPUT, "");
	}

	void process(const ProcessArgs& args) override {
	}
};


struct PhoreoWidget : ModuleWidget {
	PhoreoWidget(Phoreo* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Phoreo.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<SynthTechAlco>(mm2px(Vec(12.583, 26.247)), module, Phoreo::MOD_PARAM));
		addParam(createParamCentered<SynthTechAlco>(mm2px(Vec(37.983, 26.247)), module, Phoreo::MOD_CV_PARAM));
		addParam(createParamCentered<SynthTechAlco>(mm2px(Vec(12.583, 45.296)), module, Phoreo::MUL_PARAM));
		addParam(createParamCentered<SynthTechAlco>(mm2px(Vec(37.983, 45.296)), module, Phoreo::MUL_CV_PARAM));
		addParam(createParamCentered<SynthTechAlco>(mm2px(Vec(12.583, 64.346)), module, Phoreo::REP_PARAM));
		addParam(createParamCentered<SynthTechAlco>(mm2px(Vec(37.983, 64.346)), module, Phoreo::REP_CV_PARAM));

		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(12.575, 83.325)), module, Phoreo::MOD_TRIG_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(25.275, 83.325)), module, Phoreo::MOD_CV_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(12.575, 96.025)), module, Phoreo::MULT_TRIG_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(25.275, 96.025)), module, Phoreo::MULT_CV_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(12.575, 108.725)), module, Phoreo::REP_TRIG_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(25.275, 108.725)), module, Phoreo::REP_CV_INPUT));

		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(37.975, 83.325)), module, Phoreo::MOD_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(37.975, 96.025)), module, Phoreo::MULT_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(37.975, 108.725)), module, Phoreo::REP_OUT_OUTPUT));

		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(18.925, 89.675)), module, Phoreo::PATH3574_59_LIGHT));
		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(31.625, 102.375)), module, Phoreo::PATH3574_49_LIGHT));
	}
};


Model* modelPhoreo = createModel<Phoreo, PhoreoWidget>("Phoreo");