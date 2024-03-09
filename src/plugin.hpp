#pragma once
#include <rack.hpp>

using namespace rack;

// Declare the Plugin, defined in plugin.cpp
extern Plugin* pluginInstance;

// Declare each Model, defined in each module source file
extern Model* modelStoicheia;
extern Model* modelTonic;
extern Model* modelKlasmata;
extern Model* modelCLK;
extern Model* modelLogoi;
extern Model* modelPhoreo;

enum ModuleTheme {
	INVALID_THEME = -1,
	LIGHT_THEME,
	DARK_THEME,
	NUM_THEMES
};


struct BefacoOutputPort : app::SvgPort {
	BefacoOutputPort() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/BefacoOutputPort.svg")));
	}
};

struct BefacoInputPort : app::SvgPort {
	BefacoInputPort() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/BefacoInputPort.svg")));
	}
};

struct RebelTechBigPot : app::SvgKnob {
	widget::SvgWidget* bg;

	std::shared_ptr<Svg> fgSvgs[NUM_THEMES];
	std::shared_ptr<Svg> bgSvgs[NUM_THEMES];

	RebelTechBigPot() {
		minAngle = -0.82 * M_PI;
		maxAngle = 0.82 * M_PI;

		bg = new widget::SvgWidget;
		fb->addChildBelow(bg, tw);

		fgSvgs[0] = Svg::load(asset::plugin(pluginInstance, "res/components/Pot.svg"));
		fgSvgs[1] = Svg::load(asset::system("res/ComponentLibrary/SynthTechAlco.svg"));

		bgSvgs[0] = Svg::load(asset::plugin(pluginInstance, "res/components/Pot_bg.svg"));
		bgSvgs[1] = Svg::load(asset::system("res/ComponentLibrary/SynthTechAlco_bg.svg"));

		setSvg(fgSvgs[0]);
		bg->setSvg(bgSvgs[0]);
	}

	void setGraphicsForTheme(ModuleTheme theme) {

		setSvg(fgSvgs[theme]);
		bg->setSvg(bgSvgs[theme]);
		fb->dirty = true;
	}
};

struct RebelTechSmallPot : app::SvgKnob {
	widget::SvgWidget* bg;

	std::shared_ptr<Svg> fgSvgs[NUM_THEMES];
	std::shared_ptr<Svg> bgSvgs[NUM_THEMES];

	RebelTechSmallPot() {
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;

		bg = new widget::SvgWidget;
		fb->addChildBelow(bg, tw);

		fgSvgs[0] = Svg::load(asset::system("res/ComponentLibrary/Davies1900hWhite.svg"));
		fgSvgs[1] = Svg::load(asset::system("res/ComponentLibrary/Davies1900hBlack.svg"));

		bgSvgs[0] = Svg::load(asset::system("res/ComponentLibrary/Davies1900hWhite_bg.svg"));
		bgSvgs[1] = Svg::load(asset::system("res/ComponentLibrary/Davies1900hBlack_bg.svg"));

		setSvg(fgSvgs[0]);
		bg->setSvg(bgSvgs[0]);
	}

	void setGraphicsForTheme(ModuleTheme theme) {

		setSvg(fgSvgs[theme]);
		bg->setSvg(bgSvgs[theme]);
		fb->dirty = true;
	}
};



ModuleTheme loadDefaultTheme();
void readDefaultTheme();
void saveDefaultTheme(ModuleTheme darkAsDefault);
void writeDefaultTheme();

struct RebelTechModuleWidget : ModuleWidget {

	ModuleTheme theme = ModuleTheme::INVALID_THEME;

	RebelTechModuleWidget(std::string lightPanelSvgPath, std::string darkPanelSvgPath) {
		lightSvg = APP->window->loadSvg(asset::plugin(pluginInstance, lightPanelSvgPath));
		darkSvg = APP->window->loadSvg(asset::plugin(pluginInstance, darkPanelSvgPath));		
	}

	std::shared_ptr<window::Svg> lightSvg;
	std::shared_ptr<window::Svg> darkSvg;
	std::vector<SvgScrew*> screws;
};


template <class M>
void updateComponentsForTheme(M* module, RebelTechModuleWidget* moduleWidget, ModuleTheme& lastPanelTheme) {

#ifdef USING_CARDINAL_NOT_RACK
	ModuleTheme currentTheme = settings::preferDarkPanels ? DARK_THEME : LIGHT_THEME;
#else
	ModuleTheme currentTheme = module ? module->theme : loadDefaultTheme();
#endif

	const bool redrawRequired = moduleWidget && lastPanelTheme != currentTheme;
	const bool browserView = !module; 	// nullptr for module implies we're in browser view

	// if theme can be found, and it has changed since last draw()
	if (redrawRequired || browserView) {

		lastPanelTheme = currentTheme;

		SvgPanel* panel = static_cast<SvgPanel*>(moduleWidget->getPanel());
		panel->setBackground(lastPanelTheme == LIGHT_THEME ? moduleWidget->lightSvg : moduleWidget->darkSvg);
		panel->fb->dirty = true;

		for (ParamWidget* p : moduleWidget->getParams()) {

			// TODO: parent class RebelTechWdiget
			RebelTechBigPot* bigPot = dynamic_cast<RebelTechBigPot*>(p);
			if (bigPot) {
				bigPot->setGraphicsForTheme(lastPanelTheme);
			}
			else {
				RebelTechSmallPot* smallPot = dynamic_cast<RebelTechSmallPot*>(p);
				if (smallPot) {
					smallPot->setGraphicsForTheme(lastPanelTheme);
				}
			}
		}

		for (auto screw : moduleWidget->screws) {
			if (lastPanelTheme == LIGHT_THEME) {
				screw->setSvg(Svg::load(asset::system("res/ComponentLibrary/ScrewSilver.svg")));
			}
			else {
				screw->setSvg(Svg::load(asset::system("res/ComponentLibrary/ScrewBlack.svg")));
			}
			screw->fb->dirty = true;
		}
	}
}

void addThemeMenuItems(Menu* menu, ModuleTheme* themePtr);

struct Davies1900hWhiteKnobSnap : Davies1900hWhiteKnob {
	Davies1900hWhiteKnobSnap() {
		snap = true;
	}
};

// given offset (in range 0-1), return the offset (based on current length)
inline int paramToOffset(float param, int length) {
	return std::round((length - 1) * param);
}

// given an offset (in range 0 - (length-1)), convert to a param in range 0 - 1
inline float offsetToParam(float offset, int length) {
	return clamp(offset / (length - 1), 0.f, 1.f);
}

// given fill (in range 0-1), return the fill (based on current length)
inline int paramToFill(float param, int length) {
	return 1 + std::round((length - 1) * param);
}

// given an fill (in range 1 - length), convert to a param in range 0 - 1
inline float fillToParam(float fill, int length) {
	return clamp((fill - 1) / (length - 1), 0.f, 1.f);
}

typedef rack::dsp::TSchmittTrigger<simd::float_4> SchmittTrigger4;
