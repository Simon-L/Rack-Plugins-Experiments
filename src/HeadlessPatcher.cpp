#include "plugin.hpp"
#include <thread>

// #define PROCESS
// #define STEP
#define THREAD
#define DELAY 3.0

struct ThreadMsg
{
	uint8_t type;
};

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

	void onSave(const SaveEvent& e) override {
		DEBUG("onSave");
		if (settings::headless) {
			ThreadMsg msg;
			msg.type = 3;
			DEBUG("Sending remove cable action! %d empty? %d", msg.type, ch.empty());
			ch.push(msg);
			while (!ch.empty()) DEBUG("Waiting...");
		}
	}

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
		ch.clear();
	}

	size_t uiThreadId;
	dsp::RingBuffer<ThreadMsg, 64> ch;
	dsp::Timer mytimer;
	dsp::BooleanTrigger mytoggle;
	void process(const ProcessArgs& args) override {
		mytimer.process(args.sampleTime);
#ifdef PROCESS
		if (mytoggle.process((mytimer.getTime() > DELAY))) {
			DEBUG("process()");
			addMyCable(APP);
		}
#endif
#ifdef THREAD
		if (mytoggle.process((mytimer.getTime() > DELAY))) {
			ThreadMsg msg;
			msg.type = 1;
			DEBUG("Sending! %d empty? %d", msg.type, ch.empty());
			ch.push(msg);
		}
#endif
	}

	void addMyCable(rack::Context* ctx) {
		DEBUG("Cable!");

		Cable* cable = new Cable;
		cable->outputModule = ctx->engine->getModule(0x184bbb8c0bfc92);
		cable->outputId = 1;
		cable->inputModule = ctx->engine->getModule(0x5a28fcc5f65ce);
		cable->inputId = 3;
		cable->id = 0xdeadbeef1234;

		ctx->engine->addCable(cable);

		if (std::hash<std::thread::id>{}(std::this_thread::get_id()) == uiThreadId) {
			DEBUG("On the right thread :) !");
			rack::app::CableWidget* cw = new rack::app::CableWidget;
			cw->color = NVGcolor{0.76f, 0.11f, 0.22f, 1.00f};
			cw->setCable(cable);	
			cw->updateCable();
			if (cw->isComplete()) {
				DEBUG("Cable complete!");
				ctx->scene->rack->addCable(cw);
			}
		} else {
			DEBUG("Not the right thread sorry :/");
		}
	}
};

#ifdef THREAD
void threadfunc(HeadlessPatcher* module, rack::Context* ctx) {
	while (true) {
		if (!module->ch.empty()) {
			ThreadMsg msg = module->ch.shift();
			DEBUG("Receiving msg: %d", msg.type);
			switch (msg.type) {
				case 1:
					module->addMyCable(ctx);
					break;
				case 2:
					DEBUG("Stopping bg thread!");
					return;
					break;
				case 3:
					DEBUG("Removing cable from thread because headless!");
					Cable* cab = ctx->engine->getCable(0xdeadbeef1234);
					ctx->engine->removeCable(cab);
					break;
			}
			module->ch.clear();
		}
	}
}
#endif

struct HeadlessPatcherWidget : ModuleWidget {

	void onRemove (const RemoveEvent &e) override {
		DEBUG("ModuleWidget::onRemove()");
#ifdef THREAD
		if (module) {
			HeadlessPatcher* _module = dynamic_cast<HeadlessPatcher*>(module);
			if (_module) {
				ThreadMsg msg;
				msg.type = 2;
				DEBUG("Sending! %d empty? %d", msg.type, _module->ch.empty());
				_module->ch.push(msg);	
				DEBUG("Joining...?");
				mythread.join();
				DEBUG("Joined!");
			}
		}
#endif
		ModuleWidget::onRemove(e);
	}
	void onContextDestroy (const ContextDestroyEvent &e) override {
		DEBUG("ModuleWidget::onContextDestroy()");
#ifndef STEP
		if (!settings::headless) {
			Cable* cab = APP->engine->getCable(0xdeadbeef1234);
			APP->engine->removeCable(cab);
			DEBUG("Cable deleted in onContextDestroy because *not* headless and not STEP");
		}
#endif
		ModuleWidget::onContextDestroy(e);
	}

#ifdef THREAD
	std::thread mythread;
#endif

	void onAdd (const AddEvent &e) override {
		DEBUG("onAdd()");
		if (module) {
			HeadlessPatcher* _module = dynamic_cast<HeadlessPatcher*>(module);
			if (_module) {
				_module->uiThreadId = std::hash<std::thread::id>{}(std::this_thread::get_id());
#ifdef THREAD
				mythread = std::thread{threadfunc, _module, APP};
#endif
			}
		}
	}

	void step() override {
		if (module) {
			HeadlessPatcher* _module = dynamic_cast<HeadlessPatcher*>(module);
			if (_module) {
#ifdef STEP
				if (_module->mytoggle.process((_module->mytimer.getTime() > DELAY))) {
					DEBUG("step()");
					_module->addMyCable(APP);
				}
#endif
			}
		}
	}

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