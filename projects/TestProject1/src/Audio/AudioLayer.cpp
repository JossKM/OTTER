#include "AudioLayer.h"
#include "AudioListener.h"
#include "Gameplay/Components/ComponentManager.h"
#include <iostream>

int AudioLayer::ErrorCheck(FMOD_RESULT result)
{
	if (result != FMOD_OK) {
		std::cout << "FMOD ERROR " << result << std::endl;
		return 1;
	}
	// cout << "FMOD all good" << endl;
	return 0;
}

void AudioLayer::OnAppLoad(const nlohmann::json& config)
{
	engine->init();
}

void AudioLayer::OnAppUnload()
{
	engine->shutdown();
}

void AudioLayer::OnUpdate()
{
	engine->update();
}
