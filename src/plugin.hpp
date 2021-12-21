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

struct BefacoOutputPort : app::SvgPort {
	BefacoOutputPort() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BefacoOutputPort.svg")));
	}
};

struct BefacoInputPort : app::SvgPort {
	BefacoInputPort() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BefacoInputPort.svg")));
	}
};


enum SequenceMode {
	LATCHED,
	MUTE,
	NORMAL
};

struct SequenceParams {
	int length;
	int fill;
	int start;
	SequenceMode mode;
};

struct Davies1900hWhiteKnobSnap : Davies1900hWhiteKnob {
	Davies1900hWhiteKnobSnap() {
		snap = true;
	}
};