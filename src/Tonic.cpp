#include "plugin.hpp"

using namespace simd;

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
		GATE_OUTPUT,
		CV_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(LED, 6 * 3), 	// 6 LEDS x RGB (3)
		NUM_LIGHTS
	};

	SchmittTrigger4 triggers[6][4];
	static constexpr float semitone = 1.f / 12.f;    // one semitone is a 1/12 volt

	const int numSemitones[6] = {0, 16, 8, 4, 2, -1};
	ModuleTheme theme = LIGHT_THEME;

	Tonic() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(SCALE_PARAM, -6.f, 12.f, 0.f, "Custom offset", " semitones");

		configButton(BUTTON, "Add N semitones");
		configInput(GATE_INPUT, "Add N semitones gate");

		for (int i = 1; i < ParamIds::BUTTON_LAST; ++i) {
			configButton(BUTTON + i, string::f("Add %d semitones", numSemitones[i]));
			configInput(GATE_INPUT + i, string::f("Add %d semitones gate", numSemitones[i]));
		}

		configOutput(GATE_OUTPUT, "Gate (logical OR of all inputs/buttons)");
		configOutput(CV_OUTPUT, "Quantized CV");

		theme = loadDefaultTheme();
	}

	void process(const ProcessArgs& args) override {

		int numPolyphonyEngines = 1;
		for (int i = 0; i < ParamIds::BUTTON_LAST; ++i) {
			numPolyphonyEngines = std::max(numPolyphonyEngines, inputs[i].getChannels());
		}

		float_4 globalState[4] = {};	// gate per polyphony channel (max 16)
		float_4 voltage[4] = {};		// cv per polyphony channel (max 16)

		for (int i = 0; i < ParamIds::BUTTON_LAST; ++i) {

			float stateForLight = 0.f;
			// process polyphony in blocks of 4 channels (simd)
			for (int c = 0; c < numPolyphonyEngines; c += 4) {

				// the state for offset i is determined by (Schmitt trigger high OR button)
				triggers[i][c / 4].process(inputs[GATE_INPUT + i].getVoltageSimd<float_4>(c));
				float_4 state = simd::ifelse(triggers[i][c / 4].isHigh(), 1.f, params[BUTTON + i].getValue());

				// top gate is custom offset
				if (i == 0) {
					int semitonesForScale = std::round(params[SCALE_PARAM].getValue());
					voltage[c / 4] += semitone * semitonesForScale * state;
				}
				else {
					voltage[c / 4] += semitone * numSemitones[i] * state;
				}

				globalState[c / 4] = ifelse(globalState[c / 4], globalState[c / 4], state);

				// SSE3 simd: put sum in all elements, https://stackoverflow.com/a/6996992/251715
				state.v = _mm_hadd_ps(state.v, state.v);
				state.v = _mm_hadd_ps(state.v, state.v);
				stateForLight += state[0];
			}
			stateForLight /= numPolyphonyEngines;

			if (numPolyphonyEngines == 1) {
				// mono is yellow (like the hardware)
				lights[LED + 3 * i + 0].setBrightness(stateForLight);
				lights[LED + 3 * i + 1].setBrightness(stateForLight * 0.839);
				lights[LED + 3 * i + 2].setBrightness(stateForLight * 0.0781);
			}
			else {
				// poly is blue
				lights[LED + 3 * i + 0].setBrightness(0);
				lights[LED + 3 * i + 1].setBrightness(0);
				lights[LED + 3 * i + 2].setBrightness(stateForLight);
			}
		}

		// set outputs
		for (int c = 0; c < numPolyphonyEngines; c += 4) {
			outputs[CV_OUTPUT].setVoltageSimd<float_4>(voltage[c / 4], c);
			outputs[GATE_OUTPUT].setVoltageSimd<float_4>(10.f * globalState[c / 4], c);
		}
		outputs[GATE_OUTPUT].setChannels(numPolyphonyEngines);
		outputs[CV_OUTPUT].setChannels(numPolyphonyEngines);
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

struct TonicButton : app::SvgSwitch {
	TonicButton() {
		momentary = true;
		addFrame(Svg::load(asset::plugin(pluginInstance, "res/components/TonicButton_0.svg")));
		addFrame(Svg::load(asset::plugin(pluginInstance, "res/components/TonicButton_1.svg")));
	}
};

struct TonicWidget : ModuleWidget {
	ModuleTheme lastPanelTheme = ModuleTheme::INVALID_THEME;

	std::shared_ptr<window::Svg> lightSvg;
	std::shared_ptr<window::Svg> darkSvg;

	TonicWidget(Tonic* module) {
		setModule(module);

		lightSvg = APP->window->loadSvg(asset::plugin(pluginInstance, "res/panels/Tonic.svg"));
		darkSvg = APP->window->loadSvg(asset::plugin(pluginInstance, "res/panels/Tonic_drk.svg"));

		setPanel(lightSvg);

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Davies1900hWhiteKnobSnap>(mm2px(Vec(15.02, 18.431)), module, Tonic::SCALE_PARAM));
		addParam(createParamCentered<TonicButton>(mm2px(Vec(22.645, 32.525)), module, Tonic::BUTTON + 0));
		addParam(createParamCentered<TonicButton>(mm2px(Vec(22.645, 45.225)), module, Tonic::BUTTON + 1));
		addParam(createParamCentered<TonicButton>(mm2px(Vec(22.645, 57.925)), module, Tonic::BUTTON + 2));
		addParam(createParamCentered<TonicButton>(mm2px(Vec(22.645, 70.625)), module, Tonic::BUTTON + 3));
		addParam(createParamCentered<TonicButton>(mm2px(Vec(22.645, 83.325)), module, Tonic::BUTTON + 4));
		addParam(createParamCentered<TonicButton>(mm2px(Vec(22.645, 96.025)), module, Tonic::BUTTON + 5));

		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(7.451, 32.525)), module, Tonic::GATE_INPUT + 0));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(7.451, 45.225)), module, Tonic::GATE_INPUT + 1));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(7.451, 57.925)), module, Tonic::GATE_INPUT + 2));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(7.451, 70.625)), module, Tonic::GATE_INPUT + 3));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(7.451, 83.325)), module, Tonic::GATE_INPUT + 4));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(7.451, 96.025)), module, Tonic::GATE_INPUT + 5));

		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(7.451, 108.725)), module, Tonic::GATE_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(22.645, 108.725)), module, Tonic::CV_OUTPUT));

		addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(mm2px(Vec(15.025, 32.493)), module, Tonic::LED + 0 * 3));
		addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(mm2px(Vec(15.025, 45.196)), module, Tonic::LED + 1 * 3));
		addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(mm2px(Vec(15.025, 57.899)), module, Tonic::LED + 2 * 3));
		addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(mm2px(Vec(15.025, 70.602)), module, Tonic::LED + 3 * 3));
		addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(mm2px(Vec(15.025, 83.304)), module, Tonic::LED + 4 * 3));
		addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(mm2px(Vec(15.025, 96.007)), module, Tonic::LED + 5 * 3));
	}

	void draw(const DrawArgs& args) override {

		Tonic* module = dynamic_cast<Tonic*>(this->module);


		std::vector<Tonic::ParamIds> potsToUpdate = {};

		updateComponentsForTheme<Tonic, TonicWidget, Tonic::ParamIds>(module, this, lastPanelTheme, potsToUpdate, lightSvg, darkSvg);


		ModuleWidget::draw(args);
	}


	void appendContextMenu(Menu* menu) override {
		Tonic* module = dynamic_cast<Tonic*>(this->module);
		assert(module);

		addThemeMenuItems(menu, &module->theme);
	}
};


Model* modelTonic = createModel<Tonic, TonicWidget>("Tonic");