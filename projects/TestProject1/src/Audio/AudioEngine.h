#pragma once

#include "fmod.hpp"
#include <string>
#include <unordered_map>
#include <GLM/vec3.hpp>

//Simple AudioEngine that can load and play sounds by filename
//TODO: Clean up/release sound data
//TODO: Update Listener position/speed
//TODO: Add SoundSource class
//TODO: PlaySoundAtLocation
//
//class SoundSource
//{
//public:
//	bool setSound(std::string soundName)
//	{
//		//AudioEngine::instance()->hasSound(soundName);
//	}
//
//	bool isPlaying()
//	{
//		bool returnValue = false;
//		if (channel != nullptr)
//		{
//			channel->getPaused(&returnValue);
//		}
//		return returnValue;
//	}
//
//	void setPosition(glm::vec3 position, glm::vec3 velocity)
//	{
//		pos = AudioEngine::glm_to_FMOD_Vector(position);
//		vel = AudioEngine::glm_to_FMOD_Vector(velocity);
//	}
//
//	void update()
//	{
//		channel->set3DAttributes(&pos, &vel);
//	}
//
//	void play()
//	{
//		AudioEngine::instance()->playSoundByName(sound, &channel);
//		channel->setVolume(volume);
//		channel->setMode(bLoop);
//	}
//
//private:
//	//A better model than strings might be to use Smart Pointers (OTTER uses smart pointers and unique IDs for all its Resources)
//	std::string sound;
//
//	FMOD_VECTOR pos;
//	FMOD_VECTOR vel;
//
//	FMOD::Channel* channel;
//	float volume;
//	bool bLoop;
//
//};

class AudioEngine
{
public:
	static AudioEngine* instance();

	const static inline FMOD_VECTOR glm_to_FMOD_Vector(const glm::vec3& glmVector)
	{
		return FMOD_VECTOR{ glmVector.x, glmVector.y, glmVector.z };
	}

	const static inline glm::vec3 FMOD_to_glm_Vector(const FMOD_VECTOR& FMODVector)
	{
		return glm::vec3(FMODVector.x, FMODVector.y, FMODVector.z);
	}

public:
	//prints out fmod errors
	static int ErrorCheck(FMOD_RESULT result);

	void init();
	void update();
	void shutdown();

	void loadSound(const std::string& soundName, const std::string& filename, bool b3d, bool bLooping = false, bool bStream = false);
	void unloadSound(const std::string& soundName);
	void playSoundByName(const std::string& soundName);
	void playSoundByName(const std::string& soundName, FMOD::Channel** channel);
	bool hasSound(const std::string& soundName);

private:
	FMOD::System* pSystem;
	std::unordered_map<std::string, FMOD::Sound*> sounds;

//Singleton
	AudioEngine();

};

