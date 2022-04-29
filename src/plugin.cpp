#include "plugin.hpp"


Plugin* pluginInstance;
ModuleTheme defaultPanelTheme;


void init(Plugin* p) {
	pluginInstance = p;

	readDefaultTheme();

	// Add modules here
	p->addModel(modelStoicheia);
	p->addModel(modelTonic);
	p->addModel(modelKlasmata);
	p->addModel(modelCLK);
	p->addModel(modelLogoi);
	p->addModel(modelPhoreo);
}

// write to disk
void writeDefaultTheme() {
	json_t* settingsJ = json_object();
	json_object_set_new(settingsJ, "defaultTheme", json_integer(defaultPanelTheme));
	std::string settingsFilename = asset::user("RebelTech.json");
	FILE* file = fopen(settingsFilename.c_str(), "w");
	if (file) {
		json_dumpf(settingsJ, file, JSON_INDENT(2) | JSON_REAL_PRECISION(9));
		fclose(file);
	}
	json_decref(settingsJ);
}

// update the global theme variable (then write to disk, i.e. to json)
void saveDefaultTheme(ModuleTheme darkAsDefault) {
	defaultPanelTheme = darkAsDefault;
	writeDefaultTheme();
}

// return what the global theme variable is
ModuleTheme loadDefaultTheme() {
	return defaultPanelTheme;
}

// read what the default theme is (from disk)
void readDefaultTheme() {
	std::string settingsFilename = asset::user("RebelTech.json");
	FILE* file = fopen(settingsFilename.c_str(), "r");
	// if the settings file can't be found, write one to disk
	if (!file) {
		defaultPanelTheme = LIGHT_THEME;
		writeDefaultTheme();
		return;
	}
	// same if invalid
	json_error_t error;
	json_t* settingsJ = json_loadf(file, 0, &error);
	if (!settingsJ) {
		// invalid setting json file
		fclose(file);
		defaultPanelTheme = LIGHT_THEME;
		writeDefaultTheme();
		return;
	}
	// otherwise read the default setting
	json_t* darkAsDefaultJ = json_object_get(settingsJ, "defaultTheme");
	if (darkAsDefaultJ) {
		defaultPanelTheme = (ModuleTheme) json_integer_value(darkAsDefaultJ);
	}
	else {
		defaultPanelTheme = LIGHT_THEME;
	}

	fclose(file);
	json_decref(settingsJ);
}

void addThemeMenuItems(Menu* menu, ModuleTheme* themePtr) {

	menu->addChild(new MenuSeparator());

	menu->addChild(createIndexPtrSubmenuItem("Theme", {"Light", "Dark"}, themePtr));

	// if we're updating the default, chances are we want to set the current module's theme to the selection
	menu->addChild(createIndexSubmenuItem("Default Theme",
			{"Light", "Dark"},
			[=]() { return loadDefaultTheme(); },
			[=](int mode) { saveDefaultTheme((ModuleTheme) mode); *themePtr = (ModuleTheme) mode; }
	));
}