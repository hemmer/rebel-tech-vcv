#include "plugin.hpp"
#include "Common.hpp"
#include "Sequence.h"

struct Klasmata : Module {
	enum ParamIds {
		OFFSET_PARAM,
		LENGTH_PARAM,
		SWITCH_PARAM,
		DENSITY_PARAM,
		LENGTH_ATTEN_PARAM,
		DENSITY_ATTEN_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		RESET_INPUT,
		LENGTH_INPUT,
		DENSITY_INPUT,
		CLOCK_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		OUT_LIGHT,
		IN_LIGHT,
		NUM_LIGHTS
	};

	struct FillParam : ParamQuantity {
		// effective number of fills will depend on on the sequence length
		std::string getDisplayValueString() override {
			if (module != nullptr) {
				if (paramId == DENSITY_PARAM) {
					int lengthA = module->params[LENGTH_PARAM].getValue();
					int fillValue = 1 + std::round(getValue() * (lengthA - 1));
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

	Sequence<uint16_t> seq;
	SequenceParams oldParams, currentParams;
	dsp::SchmittTrigger clockTrigger;
	dsp::SchmittTrigger resetTrigger;
	bool state = false;

	Klasmata() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(OFFSET_PARAM, 0.f, 31.f, 0.f, "Offset");
		configParam(LENGTH_PARAM, 1.f, 32.f, 1.f, "Length");
		configParam<ModeParam>(SWITCH_PARAM, 0.f, NORMAL, NORMAL, "Mode");
		configParam<FillParam>(DENSITY_PARAM, 0.f, 1.f, 0.5f, "");
		configParam(LENGTH_ATTEN_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DENSITY_ATTEN_PARAM, 0.f, 1.f, 0.f, "");

		seq.offset = 0;
		seq.calculate(12, 8);
	}

	void process(const ProcessArgs& args) override {

		if (resetTrigger.process(rescale(inputs[RESET_INPUT].getVoltage(), 0.1f, 2.f, 0.f, 1.f))) {
			seq.reset();
		}

		// update params of sequence (if changed)
		currentParams.length = params[LENGTH_PARAM].getValue();
		currentParams.fill = 1 + std::round((currentParams.length - 1) * params[DENSITY_PARAM].getValue());
		currentParams.start = params[OFFSET_PARAM].getValue();
		currentParams.mode = static_cast<SequenceMode>(params[SWITCH_PARAM].getValue());

		if (currentParams.length != oldParams.length || currentParams.fill != oldParams.fill) {
			seq.calculate(currentParams.length, currentParams.fill);
		}
		if (currentParams.start != oldParams.start) {
			seq.rotate(currentParams.start);
		}

		const float in = inputs[CLOCK_INPUT].getVoltage();
		if (clockTrigger.process(rescale(in, 0.1f, 2.f, 0.f, 1.f))) {
			state = seq.next();
		}

		float out = 0.f;
		if (currentParams.mode == NORMAL) {
			out = state * in;
		}
		else if (currentParams.mode == LATCHED) {
			out = state * 10.f;
		}
		else {
			out = 0.f;
		}

		outputs[OUT_OUTPUT].setVoltage(out);
		lights[OUT_LIGHT].setBrightness(out / 10.f);
		lights[IN_LIGHT].setBrightness(in / 10.f);
		oldParams = currentParams;

	}
};


struct KlasmataWidget : ModuleWidget {
	KlasmataWidget(Klasmata* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Klasmata.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Davies1900hWhiteKnobSnap>(mm2px(Vec(11.359, 24.259)), module, Klasmata::OFFSET_PARAM));
		addParam(createParamCentered<Davies1900hWhiteKnobSnap>(mm2px(Vec(11.359, 44.12)), module, Klasmata::LENGTH_PARAM));
		addParam(createParamCentered<BefacoSwitch>(mm2px(Vec(32.58, 44.901)), module, Klasmata::SWITCH_PARAM));
		addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(11.359, 63.98)), module, Klasmata::DENSITY_PARAM));
		addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(11.359, 83.84)), module, Klasmata::LENGTH_ATTEN_PARAM));
		addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(11.359, 103.701)), module, Klasmata::DENSITY_ATTEN_PARAM));

		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(32.58, 58.053)), module, Klasmata::RESET_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(32.58, 83.691)), module, Klasmata::LENGTH_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(32.58, 96.51)), module, Klasmata::DENSITY_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(32.58, 109.329)), module, Klasmata::CLOCK_INPUT));

		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(32.58, 70.872)), module, Klasmata::OUT_OUTPUT));

		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(22.317, 70.626)), module, Klasmata::OUT_LIGHT));
		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(23.052, 109.18)), module, Klasmata::IN_LIGHT));
	}
};


Model* modelKlasmata = createModel<Klasmata, KlasmataWidget>("Klasmata");