#pragma once
#include "Application/ApplicationLayer.h"
#include "Audio/AudioListener.h"
#include "fmod_studio.hpp"
#include "AudioEngine.h"

class AudioLayer : public ApplicationLayer
{
public:
	static int ErrorCheck(FMOD_RESULT result);

	AudioLayer() { engine = AudioEngine::instance(); };

	void OnAppLoad(const nlohmann::json& config) override;
	void OnAppUnload() override;

	void OnUpdate() override;

	~AudioLayer() {};

	AudioEngine* engine;
};
