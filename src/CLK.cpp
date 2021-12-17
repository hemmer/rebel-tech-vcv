#include "plugin.hpp"
#include "Common.hpp"


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
		_1_OUTPUT,
		_2_OUTPUT,
		_3_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		_1_LIGHT,
		_2_LIGHT,
		_3_LIGHT,
		LIGHTS_LEN
	};

	CLK() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(BPM_PARAM, 0.f, 1.f, 0.f, "");
		configParam(SCALE_8_PARAM, -4.f, 4.f, 0.f, "");
		configParam(SCALE_24_PARAM, -4.f, 4.f, 0.f, "");
		configOutput(_1_OUTPUT, "");
		configOutput(_2_OUTPUT, "");
		configOutput(_3_OUTPUT, "");
	}

	void process(const ProcessArgs& args) override {
	}
};


struct CLKWidget : ModuleWidget {
	CLKWidget(CLK* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/CLK.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Davies1900hWhiteKnobSnap>(mm2px(Vec(8.984, 19.8)), module, CLK::BPM_PARAM));
		addParam(createParamCentered<Davies1900hWhiteKnobSnap>(mm2px(Vec(8.984, 38.85)), module, CLK::SCALE_8_PARAM));
		addParam(createParamCentered<Davies1900hWhiteKnobSnap>(mm2px(Vec(8.984, 57.9)), module, CLK::SCALE_24_PARAM));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.0, 76.95)), module, CLK::_1_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.852, 92.205)), module, CLK::_2_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.0, 108.7)), module, CLK::_3_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(8.222, 69.33)), module, CLK::_1_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(8.222, 85.205)), module, CLK::_2_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(8.222, 101.08)), module, CLK::_3_LIGHT));
	}
};


Model* modelCLK = createModel<CLK, CLKWidget>("CLK");