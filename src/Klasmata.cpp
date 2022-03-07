#include "plugin.hpp"
#include "Sequence.h"

struct Klasmata : Module {
	enum ParamIds {
		OFFSET_PARAM,
		LENGTH_PARAM,
		SWITCH_PARAM,
		DENSITY_PARAM,
		LENGTH_CV_PARAM,
		DENSITY_CV_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		RESET_INPUT,
		LENGTH_CV_INPUT,
		DENSITY_CV_INPUT,
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

	struct OffsetParam : ParamQuantity {
		// effective offset will depend on on the sequence length
		std::string getDisplayValueString() override {
			if (module != nullptr) {
				if (paramId == OFFSET_PARAM) {
					int offset = paramToOffset(getValue(), module->params[LENGTH_PARAM].getValue());
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
				if (paramId == OFFSET_PARAM) {
					int length = module->params[LENGTH_PARAM].getValue();
					ParamQuantity::setValue(offsetToParam(offset, length));
				}
				else {
					assert(false);
				}
			}
		}
	};

	struct FillParam : ParamQuantity {
		// effective number of fills will depend on on the sequence length
		std::string getDisplayValueString() override {
			if (module != nullptr) {
				if (paramId == DENSITY_PARAM) {
					int length = module->params[LENGTH_PARAM].getValue();
					return std::to_string(paramToFill(getValue(), length));
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
			int fill = std::atof(s.c_str());
			if (module != nullptr) {
				if (paramId == DENSITY_PARAM) {
					int length = module->params[LENGTH_PARAM].getValue();
					ParamQuantity::setValue(fillToParam(fill, length));
				}
				else {
					assert(false);
				}
			}
		}
	};

	enum SequenceMode {
		LATCHED,
		NORMAL,
		MUTE
	};

	struct SequenceParams {
		int length;
		int fill;
		int start;
		SequenceMode mode;
	};

	// uint32_t for 32 steps
	Sequence<uint32_t> seq;
	SequenceParams oldParams, currentParams;
	dsp::SchmittTrigger clockTrigger;
	dsp::SchmittTrigger resetTrigger;
	bool state, stateAlternating = false;
	ModuleTheme theme = LIGHT_THEME;

	Klasmata() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam<OffsetParam>(OFFSET_PARAM, 0.f, 1.f, 0.f, "Offset");

		auto lengthParam = configParam(LENGTH_PARAM, 1.f, 32.f, 1.f, "Length");
		lengthParam->snapEnabled = true;

		configSwitch(SWITCH_PARAM, LATCHED, MUTE, NORMAL, "Mode", {"Latched", "Normal", "Mute"});
		configParam<FillParam>(DENSITY_PARAM, 0.f, 1.f, 0.5f, "Fill Density");
		configParam(LENGTH_CV_PARAM, 0.f, 1.f, 0.f, "Length CV Attenution");
		configParam(DENSITY_CV_PARAM, 0.f, 1.f, 0.f, "Density CV Attenution");

		configInput(RESET_INPUT, "Reset");
		configInput(LENGTH_CV_INPUT, "Length CV");
		configInput(DENSITY_CV_INPUT, "Density CV");
		configInput(CLOCK_INPUT, "Clock");
		configOutput(OUT_OUTPUT, "Euclidean sequence");

		configLight(IN_LIGHT, "Clock input");
		configLight(OUT_LIGHT, "Sequence");

		seq.offset = 0;
		seq.calculate(12, 8);
	}

	void processBypass(const ProcessArgs& args) override {
		clockTrigger.process(rescale(inputs[CLOCK_INPUT].getVoltage(), 0.1f, 2.f, 0.f, 1.f));
		outputs[OUT_OUTPUT].setVoltage(clockTrigger.isHigh() * 10.f);
	}

	void process(const ProcessArgs& args) override {

		if (resetTrigger.process(rescale(inputs[RESET_INPUT].getVoltage(), 0.1f, 2.f, 0.f, 1.f))) {
			seq.reset();
		}

		// process knobs and CV
		{
			// value between -1 and 1
			const float lengthCV = clamp(inputs[LENGTH_CV_INPUT].getVoltage() / 10.f, -1.f, +1.f) * params[LENGTH_CV_PARAM].getValue();
			// actual length is knob value plus CV (adds)
			currentParams.length = std::round(clamp(params[LENGTH_PARAM].getValue() + lengthCV * 31, 1.f, 32.f));

			// value between -1 and 1
			const float fillCV = clamp(inputs[DENSITY_CV_INPUT].getVoltage() / 10.f, -1.f, +1.f) * params[DENSITY_CV_PARAM].getValue();
			// knob + CV gives a density in range [0, 1]
			const float density = clamp(fillCV + params[DENSITY_PARAM].getValue(), 0.f, 1.0f);
			// fill is then the this fraction of length
			currentParams.fill = paramToFill(density, currentParams.length);

			currentParams.start = paramToOffset(params[OFFSET_PARAM].getValue(), currentParams.length);
			currentParams.mode = static_cast<SequenceMode>(params[SWITCH_PARAM].getValue());
		}

		// update params of sequence (if changed)
		if (currentParams.length != oldParams.length || currentParams.fill != oldParams.fill) {
			seq.calculate(currentParams.length, currentParams.fill);
		}
		if (currentParams.start != oldParams.start) {
			seq.rotate(currentParams.start);
		}

		const float in = inputs[CLOCK_INPUT].getVoltage();
		if (clockTrigger.process(rescale(in, 0.1f, 2.f, 0.f, 1.f))) {
			bool newState = seq.next();
			if (newState != state) {
				stateAlternating = !stateAlternating;
			}
			state = newState;
		}

		float out = 0.f;
		if (currentParams.mode == NORMAL) {
			out = state * in;
		}
		else if (currentParams.mode == LATCHED) {
			out = stateAlternating * 10.f;
		}
		else {
			out = 0.f;
		}

		outputs[OUT_OUTPUT].setVoltage(out);
		lights[OUT_LIGHT].setBrightness(out / 10.f);
		lights[IN_LIGHT].setBrightness(in / 10.f);
		oldParams = currentParams;
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


struct KlasmataWidget : ModuleWidget {
	KlasmataWidget(Klasmata* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/Klasmata.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RebelTechPot>(mm2px(Vec(12.55, 26.256)), module, Klasmata::OFFSET_PARAM));
		addParam(createParamCentered<RebelTechPot>(mm2px(Vec(12.55, 45.306)), module, Klasmata::LENGTH_PARAM));
		addParam(createParamCentered<BefacoSwitch>(mm2px(Vec(32.875, 45.225)), module, Klasmata::SWITCH_PARAM));
		addParam(createParamCentered<RebelTechPot>(mm2px(Vec(12.55, 64.356)), module, Klasmata::DENSITY_PARAM));
		addParam(createParamCentered<RebelTechPot>(mm2px(Vec(12.55, 83.406)), module, Klasmata::LENGTH_CV_PARAM));
		addParam(createParamCentered<RebelTechPot>(mm2px(Vec(12.55, 102.456)), module, Klasmata::DENSITY_CV_PARAM));

		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(32.875, 57.925)), module, Klasmata::RESET_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(32.875, 83.325)), module, Klasmata::LENGTH_CV_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(32.875, 96.025)), module, Klasmata::DENSITY_CV_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(32.875, 108.725)), module, Klasmata::CLOCK_INPUT));

		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(32.875, 70.625)), module, Klasmata::OUT_OUTPUT));

		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(22.715, 70.625)), module, Klasmata::OUT_LIGHT));
		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(22.715, 108.725)), module, Klasmata::IN_LIGHT));
	}
};


Model* modelKlasmata = createModel<Klasmata, KlasmataWidget>("Klasmata");