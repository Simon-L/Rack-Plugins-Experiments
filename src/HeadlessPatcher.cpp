#include "plugin.hpp"

// #define PROCESS
#define STEP

struct HeadlessPatcher : Module {
	enum ParamId {
		K1_PARAM,
		K2_PARAM,
		// FMCV_PARAM,
		B1_PARAM,
		B2_PARAM,
		// PWMCV_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		// FM_INPUT,
		// PITCH_INPUT,
		// SYNC_INPUT,
		// PWM_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		// SIN_OUTPUT,
		// TRI_OUTPUT,
		// SAW_OUTPUT,
		// SQR_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		// FREQ_LIGHT,
		LIGHTS_LEN
	};

	HeadlessPatcher() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(K1_PARAM, 0.f, 1.f, 0.f, "K1_PARAM");
		configParam(K2_PARAM, 0.f, 1.f, 0.f, "K2_PARAM");
		// configParam(FMCV_PARAM, 0.f, 1.f, 0.f, "");
		configParam(B1_PARAM, 0.f, 1.f, 0.f, "B1_PARAM");
		configParam(B2_PARAM, 0.f, 1.f, 0.f, "B2_PARAM");
		// configParam(PWMCV_PARAM, 0.f, 1.f, 0.f, "");
		// configInput(FM_INPUT, "");
		// configInput(PITCH_INPUT, "");
		// configInput(SYNC_INPUT, "");
		// configInput(PWM_INPUT, "");
		// configOutput(SIN_OUTPUT, "");
		// configOutput(TRI_OUTPUT, "");
		// configOutput(SAW_OUTPUT, "");
		// configOutput(SQR_OUTPUT, "");
	}

	dsp::Timer mytimer;
	dsp::BooleanTrigger mytoggle;
	void process(const ProcessArgs& args) override {
		mytimer.process(args.sampleTime);
#ifdef PROCESS
		if (mytoggle.process((mytimer.getTime() > 0.4))) {
			DEBUG("process()");
			goCableGo();
		}
#endif
	}

	void goCableGo() {
		DEBUG("Cable!");

		Cable* cable = new Cable;
		cable->outputModule = APP->engine->getModule(0x184bbb8c0bfc92);
		cable->outputId = 1;
		cable->inputModule = APP->engine->getModule(0x5a28fcc5f65ce);
		cable->inputId = 3;

		APP->engine->addCable(cable);
		rack::app::CableWidget* cw = new rack::app::CableWidget;
		cw->setCable(cable);
		cw->color = NVGcolor{0.76f, 0.11f, 0.22f, 1.00f};
		cw->updateCable();
		if (cw->isComplete()) {
			DEBUG("Cable complete!");
	    	APP->scene->rack->addCable(cw);
		}
	}
};


struct HeadlessPatcherWidget : ModuleWidget {

#ifdef STEP
	void step() override {
		if (module) {
			HeadlessPatcher* _module = dynamic_cast<HeadlessPatcher*>(module);
			if (_module->mytoggle.process((_module->mytimer.getTime() > 0.4))) {
				DEBUG("step()");
				_module->goCableGo();
			}
		}
	}
#endif

	HeadlessPatcherWidget(HeadlessPatcher* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/HeadlessPatcher.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(22.905, 29.808)), module, HeadlessPatcher::K1_PARAM));
		addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(22.862, 56.388)), module, HeadlessPatcher::K2_PARAM));
		// addParam(createParamCentered<Trimpot>(mm2px(Vec(6.607, 80.603)), module, HeadlessPatcher::FMCV_PARAM));
		addParam(createParamCentered<LEDButton>(mm2px(Vec(15.444, 80.603)), module, HeadlessPatcher::B1_PARAM));
		addParam(createParamCentered<LEDButton>(mm2px(Vec(30.282, 80.603)), module, HeadlessPatcher::B2_PARAM));
		// addParam(createParamCentered<Trimpot>(mm2px(Vec(39.118, 80.603)), module, HeadlessPatcher::PWMCV_PARAM));

		// addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.607, 96.859)), module, HeadlessPatcher::FM_INPUT));
		// addInput(createInputCentered<PJ301MPort>(mm2px(Vec(17.444, 96.859)), module, HeadlessPatcher::PITCH_INPUT));
		// addInput(createInputCentered<PJ301MPort>(mm2px(Vec(28.282, 96.859)), module, HeadlessPatcher::SYNC_INPUT));
		// addInput(createInputCentered<PJ301MPort>(mm2px(Vec(39.15, 96.859)), module, HeadlessPatcher::PWM_INPUT));

		// addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(6.607, 113.115)), module, HeadlessPatcher::SIN_OUTPUT));
		// addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(17.444, 113.115)), module, HeadlessPatcher::TRI_OUTPUT));
		// addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(28.282, 113.115)), module, HeadlessPatcher::SAW_OUTPUT));
		// addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(39.119, 113.115)), module, HeadlessPatcher::SQR_OUTPUT));

		// addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(mm2px(Vec(31.089, 16.428)), module, HeadlessPatcher::FREQ_LIGHT));
	}
};


Model* modelHeadlessPatcher = createModel<HeadlessPatcher, HeadlessPatcherWidget>("HeadlessPatcher");