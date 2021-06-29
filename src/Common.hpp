#pragma once

#include <rack.hpp>
#include <dsp/common.hpp>

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
		minAngle = -0.75 * M_PI;
		maxAngle = 0.75 * M_PI;
	}
};

struct ModeParam : ParamQuantity {
	std::string getDisplayValueString() override {
		switch (static_cast<SequenceMode>(getValue())) {
			case MUTE: return "Mute";
			case LATCHED: return "Latched";
			case NORMAL: return "Normal";
			default: assert(false);
		}
	}
};