#include "AudioEngine.h"
#include "fmod_errors.h"
#include <iostream>

AudioEngine* AudioEngine::instance()
{
	static AudioEngine instance;
	return &instance;
}
int AudioEngine::ErrorCheck(FMOD_RESULT result)
{
	if (result != FMOD_OK)
	{
		std::cerr << "Whoops, FMOD error: " << FMOD_ErrorString(result) << std::endl;

#ifdef _DEBUG
		__debugbreak();
#endif
		return 1;
	}

	return 0;
}

void AudioEngine::init()
{
	ErrorCheck(FMOD::System_Create(&pSystem));
	ErrorCheck(pSystem->init(32, FMOD_INIT_NORMAL, nullptr));
}

void AudioEngine::update()
{
	ErrorCheck(pSystem->update());
}

void AudioEngine::shutdown()
{
	ErrorCheck(pSystem->close());
	ErrorCheck(pSystem->release());
}

void AudioEngine::loadSound(const std::string& soundName, const std::string& filename, bool b3d, bool bLooping, bool bStream)
{
	//Check if already loaded
	auto foundElement = sounds.find(soundName);
	if (foundElement != sounds.end())
	{
		//Has already been loaded
		return;
	}

	FMOD_MODE mode = FMOD_DEFAULT;
	mode |= (b3d) ? FMOD_3D : FMOD_2D;
	mode |= (bLooping) ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;
	mode |= (bStream) ? FMOD_CREATESTREAM : FMOD_CREATECOMPRESSEDSAMPLE;

	FMOD::Sound* loadedSound = nullptr;
	ErrorCheck(pSystem->createSound(filename.c_str(), mode, nullptr, &loadedSound));

	if (loadedSound != nullptr)
	{
		sounds[soundName] = loadedSound;
	}
}

void AudioEngine::unloadSound(const std::string& soundName)
{
	//Check if already loaded
	auto foundElement = sounds.find(soundName);
	if (foundElement != sounds.end())
	{
		ErrorCheck(foundElement->second->release());
		sounds.erase(foundElement);
	}
}

void AudioEngine::playSoundByName(const std::string& soundName)
{
	ErrorCheck(pSystem->playSound(sounds[soundName], nullptr, false, nullptr));
}

void AudioEngine::playSoundByName(const std::string& soundName, FMOD::Channel** channel)
{
	ErrorCheck(pSystem->playSound(sounds[soundName], nullptr, false, channel));
}

bool AudioEngine::hasSound(const std::string& soundName)
{
	return((bool)sounds.count(soundName));
}

AudioEngine::AudioEngine()
{
}
