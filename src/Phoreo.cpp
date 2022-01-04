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
		MUL_TRIG_INPUT,
		MUL_CV_INPUT,
		REP_TRIG_INPUT,
		REP_CV_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		MOD_OUTPUT,
		MULT_OUTPUT,
		REP_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		PWM_LIGHT,
		REP_LIGHT,
		LIGHTS_LEN
	};

	// derived from https://github.com/pingdynasty/ClockMultiplier/blob/master/ClockMultiplier.cpp
	typedef uint32_t ClockTick;

	class ClockDuration {
	public:
		ClockTick period;
		ClockTick fallMark;
		ClockTick pos;
		bool state;
		uint16_t duration;
		ClockDuration() {
			reset();
		}
		inline void reset() {
			period = fallMark = pos = 0;
			off();
		}
		inline void rise() {
			period = pos;
			fallMark = ((uint32_t)period * duration) >> 12;
			pos = 0;
			on();
		}
		inline void clock() {
			if (++pos >= fallMark) {
				off();
			}
		}
		void on() {
			state = true;
		}
		void off() {
			state = false;
		}
		bool isOff() {
			return !state;
		}
	};


	class ClockMultiplier {
	public:
		ClockTick fallMark;
		ClockTick pos;
		ClockTick counter;
		ClockTick period;
		bool state;
		uint16_t mul;

		ClockMultiplier() {
			reset();
		}
		inline void reset() {
			fallMark = pos = period = counter = 0;
			off();
		}
		inline void rise(ClockDuration& dur) {
			on();
			period = counter / mul;
			fallMark = (((uint32_t)period * dur.duration) >> 12);
			counter = 0;
			pos = 0;
		}
		inline void clock() {
			if (pos++ == fallMark) {
				off();
			}
			else if (pos >= period) {
				on();
				pos = 0;
			}
			counter++;
		}
		void on() {
			state = true;
		}
		void off() {
			state = false;
		}
		bool isOff() {
			return !state;
		}
	};

	class ClockRepeater {
	public:
		ClockRepeater() {
			reset();
		}
		ClockTick period;
		ClockTick fallMark;
		uint8_t reps;
		uint8_t times;
		ClockTick pos;
		bool running;
		bool state;
		uint16_t rep;
		inline void stop() {
			running = false;
		}
		inline void reset() {
			stop();
			off();
		}
		inline void rise(ClockDuration& dur, ClockMultiplier& mul) {
			on();
			times = 0;
			reps = rep;
			period = mul.period;
			fallMark = (((uint32_t)period * dur.duration) >> 12);
			pos = 0;
			running = true;
		}
		inline void clock() {
			if (running) {
				if (pos++ == fallMark) {
					off();
				}
				else if (pos >= period) {
					if (++times >= reps) {
						stop();
					}
					else {
						on();
					}
					pos = 0;
				}
			}
		}
		void on() {
			state = true;
		}
		void off() {
			state = false;
		}
		bool isOff() {
			return !state;
		}
	};

	ClockDuration dur;
	ClockMultiplier mul;
	ClockRepeater rep;

	dsp::SchmittTrigger clockTriggers[3];

	Phoreo() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(MOD_PARAM, 0.f, 100.f, 50.f, "Pulse width", "%");
		configParam(MOD_CV_PARAM, 0.f, 1.f, 0.f, "Pulse width CV");

		auto multParam = configParam(MUL_PARAM, 1.f, 16.f, 1.f, "Clock multiplication factor");
		multParam->snapEnabled = true;
		configParam(MUL_CV_PARAM, 0.f, 1.f, 0.f, "Clock multiplication CV");

		auto repParam = configParam(REP_PARAM, 1.f, 16.f, 1.f, "Number of repetions");
		repParam->snapEnabled = true;
		configParam(REP_CV_PARAM, 0.f, 1.f, 0.f, "Number of repetions CV");

		configInput(MOD_TRIG_INPUT, "Modulated clock");
		configInput(MOD_CV_INPUT, "Pulsewidth CV");
		configInput(MUL_TRIG_INPUT, "Multiplied clock (normalled to above clock)");
		configInput(MUL_CV_INPUT, "Multiplier CV");
		configInput(REP_TRIG_INPUT, "Trigger repetitions clock (normalled to above clocks)");
		configInput(REP_CV_INPUT, "Repetition CV");

		configOutput(MOD_OUTPUT, "Pulsewidth modulated clock");
		configOutput(MULT_OUTPUT, "Multiplied clock");
		configOutput(REP_OUTPUT, "Repeated clock");

		reset();
	}

	void process(const ProcessArgs& args) override {

		// knob and CV processing
		{
			// range -10V to +10V scaled to -1 to +1
			float durCV = params[MOD_CV_PARAM].getValue() * clamp(inputs[MOD_CV_INPUT].getVoltage(), -10.f, +10.f) / 10.f;
			float durTotal = clamp(params[MOD_PARAM].getValue() / 100.f + durCV, 0.f, 1.f);
			dur.duration = durTotal * 4097.f;

			// range -10V to +10V scaled to -1 to +1
			float mulCV = params[MUL_CV_PARAM].getValue() * clamp(inputs[MUL_CV_INPUT].getVoltage(), -10.f, +10.f) / 10.f;
			float mulTotal = clamp(params[MUL_PARAM].getValue() + mulCV * 16, 1.f, 16.f);
			// range of 1 to 16
			mul.mul = (uint16_t) std::round(mulTotal);

			// range -10V to +10V scaled to -1 to +1
			float repCV = params[REP_CV_PARAM].getValue() * clamp(inputs[REP_CV_INPUT].getVoltage(), -10.f, +10.f) / 10.f;
			float repTotal = clamp(params[REP_PARAM].getValue() + repCV * 16, 1.f, 16.f);
			// range 1 to 16
			rep.rep = (uint16_t) std::round(repTotal);
		}


		const float durClock = inputs[MOD_TRIG_INPUT].getVoltage();
		if (clockTriggers[0].process(durClock, 0.1f, 2.f)) {
			dur.rise();
		}

		// normalled from top clock
		const float mulClock = inputs[MUL_TRIG_INPUT].getNormalVoltage(durClock);
		if (clockTriggers[1].process(mulClock, 0.1f, 2.f)) {
			mul.rise(dur);
		}

		// normalled from top two clocks
		const float repClock = inputs[REP_TRIG_INPUT].getNormalVoltage(mulClock);
		if (clockTriggers[2].process(repClock, 0.1f, 2.f)) {
			rep.rise(dur, mul);
		}

		dur.clock();
		mul.clock();
		rep.clock();

		lights[PWM_LIGHT].setBrightnessSmooth(!dur.isOff(), args.sampleTime);
		lights[REP_LIGHT].setBrightnessSmooth(!rep.isOff(), args.sampleTime);

		outputs[MOD_OUTPUT].setVoltage(10.f * !dur.isOff());
		outputs[MULT_OUTPUT].setVoltage(10.f * !mul.isOff());
		outputs[REP_OUTPUT].setVoltage(10.f * !rep.isOff());
	}

	void reset() {
		mul.reset();
		dur.reset();
		rep.reset();
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
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(12.575, 96.025)), module, Phoreo::MUL_TRIG_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(25.275, 96.025)), module, Phoreo::MUL_CV_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(12.575, 108.725)), module, Phoreo::REP_TRIG_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(25.275, 108.725)), module, Phoreo::REP_CV_INPUT));

		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(37.975, 83.325)), module, Phoreo::MOD_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(37.975, 96.025)), module, Phoreo::MULT_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(37.975, 108.725)), module, Phoreo::REP_OUTPUT));

		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(18.925, 89.675)), module, Phoreo::PWM_LIGHT));
		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(31.625, 102.375)), module, Phoreo::REP_LIGHT));
	}
};


Model* modelPhoreo = createModel<Phoreo, PhoreoWidget>("Phoreo");