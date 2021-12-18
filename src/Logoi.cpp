#include "plugin.hpp"


struct Logoi : Module {
	enum ParamId {
		DIV_PARAM,
		PLUS_PARAM,
		DIV_CV_PARAM,
		PLUS_CV_PARAM,
		PATH2680_4_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		DIV_CV_INPUT,
		PLUS_CV_INPUT,
		PATH2680_84_INPUT,
		PATH2680_2_INPUT,
		PATH2680_0_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		PATH2680_13_OUTPUT,
		PATH2680_80_OUTPUT,
		PATH2680_3_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		PATH2680_41_LIGHT,
		PATH2680_6_LIGHT,
		PATH2680_41_2_LIGHT,
		LIGHTS_LEN
	};

	Logoi() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(DIV_PARAM, 0.f, 1.f, 0.f, "");
		configParam(PLUS_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DIV_CV_PARAM, 0.f, 1.f, 0.f, "");
		configParam(PLUS_CV_PARAM, 0.f, 1.f, 0.f, "");
		configParam(PATH2680_4_PARAM, 0.f, 1.f, 0.f, "");
		configInput(DIV_CV_INPUT, "");
		configInput(PLUS_CV_INPUT, "");
		configInput(PATH2680_84_INPUT, "");
		configInput(PATH2680_2_INPUT, "");
		configInput(PATH2680_0_INPUT, "");
		configOutput(PATH2680_13_OUTPUT, "");
		configOutput(PATH2680_80_OUTPUT, "");
		configOutput(PATH2680_3_OUTPUT, "");
	}

	void process(const ProcessArgs& args) override {
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

		addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(12.533, 26.112)), module, Logoi::DIV_PARAM));
		addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(37.933, 26.112)), module, Logoi::PLUS_PARAM));
		addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(12.533, 45.162)), module, Logoi::DIV_CV_PARAM));
		addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(37.933, 45.162)), module, Logoi::PLUS_CV_PARAM));
		addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(25.225, 70.48)), module, Logoi::PATH2680_4_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.525, 83.18)), module, Logoi::DIV_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(37.925, 83.18)), module, Logoi::PLUS_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(25.225, 95.88)), module, Logoi::PATH2680_84_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.525, 108.58)), module, Logoi::PATH2680_2_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(37.925, 108.58)), module, Logoi::PATH2680_0_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(12.525, 95.88)), module, Logoi::PATH2680_13_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(37.925, 95.88)), module, Logoi::PATH2680_80_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(25.225, 108.58)), module, Logoi::PATH2680_3_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(25.225, 57.78)), module, Logoi::PATH2680_41_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(12.525, 70.48)), module, Logoi::PATH2680_6_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(37.975, 70.625)), module, Logoi::PATH2680_41_2_LIGHT));
	}
};


Model* modelLogoi = createModel<Logoi, LogoiWidget>("Logoi");