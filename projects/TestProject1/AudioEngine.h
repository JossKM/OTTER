#pragma once

#include "Application/ApplicationLayer.h"

// FMOD
#include "fmod_studio.hpp"
#include "fmod_errors.h"

// Standard Library
#include <string>
#include <unordered_map>
#include <vector>
#include <math.h>
#include <iostream>

#include <glm/glm.hpp>



class AudioListener
{
	friend class AudioEngine;

public:

	void SetPosition(const glm::vec3& pos);
	glm::vec3 GetPosition();


	// Vel not working
	void SetVelocity(const glm::vec3& vel);
	glm::vec3 GetVelocity();


	// TODO: Test these
	void SetForward(const glm::vec3& forward);
	glm::vec3 GetForward();
	void SetUp(const glm::vec3& up);
	glm::vec3 GetUp();


private:

	// Only AudioEngine can create a listener
	// Get a ref from AudioEngine::GetListener()
	AudioListener() {}
	AudioListener(AudioListener const&) = delete;
	void operator=(AudioListener const&) = delete;

private:

	// Ref to the FMOD System
	FMOD::Studio::System* m_StudioSystem;

	// Save the most recent changes
	FMOD_3D_ATTRIBUTES m_Attributes;
	FMOD_VECTOR m_AttenuationPosition;

	// Basic ID, first listener is 0
	int m_ID;
	void SetID(const int& id);

};


class AudioEvent
{
	friend class AudioEngine;

public:

	~AudioEvent();

	// Will only play if event is not currently playing
	void Play();

	// Restarts the event
	void Restart();

	// Allows AHDSR modulators to complete their release, and DSP effect tails to play out.
	void Stop();

	// Stops the event instance immediately.
	void StopImmediately();

	// Checks if event is playing
	bool isPlaying();

	// Parameters
	void SetParameter(const char* name, const float& value, const bool& ignoreSeekSpeed = false);
	float GetParameterValue(const char* name);

	void SetPosition(const glm::vec3& pos);
	glm::vec3 GetPosition();


private:

	// AudioEngine class uses this to create Event objects
	AudioEvent(FMOD::Studio::EventInstance* eventInstance);

	// Don't want copies, should only grab refs from audio engine
	AudioEvent(AudioEvent const&) = delete;
	void operator=(AudioEvent const&) = delete;

private:

	FMOD_3D_ATTRIBUTES m_Attributes;

	FMOD::Studio::EventInstance* m_EventInstance;
};


class AudioEngine : public ApplicationLayer
{
public:
	//Inherited from ApplicationLayer
	/**
	 * Invoked when the application is first loaded
	 *
	 * @param config The global application config
	 */
	virtual void OnAppLoad(const nlohmann::json& config);
	/**
	 * Invoked when the application has quit, and is unloading resources
	 */
	virtual void OnAppUnload();

	static int ErrorCheck(FMOD_RESULT result);
	static float dbToVolume(float db);
	static float VolumeTodb(float volume);
	static FMOD_VECTOR VectorToFmod(const glm::vec3& vec);
	static glm::vec3 FmodToVector(const FMOD_VECTOR& vec);

	//// Singleton ///////////////////

	static AudioEngine& Instance();

	AudioEngine(AudioEngine const&) = delete;
	void operator=(AudioEngine const&) = delete;

	//////////////////////////////////

	void Init();
	void Update();
	void Shutdown();

	//// Individual Sounds ////
	void Loadsound(const std::string& strSoundName, bool b3d = true, bool bLooping = false, bool bStream = false);
	void UnLoadSound(const std::string& strSoundName);

	//// Banks ////
	void LoadBank(const std::string& strBankName, FMOD_STUDIO_LOAD_BANK_FLAGS flags = FMOD_STUDIO_LOAD_BANK_NORMAL);

	//// Listener ////
	AudioListener& GetListener();

	//// Events ////
	AudioEvent& CreateEvent(const std::string& strEventName, const std::string& GUID);
	AudioEvent& GetEvent(const std::string& strEventName);

	//// Global Parameters ////
	void SetGlobalParameter(const char* name, const float& value, const bool& ignoreSeekSpeed = false);
	float GetGlobalParameterValue(const char* name);


private:

	AudioEngine() {}

private:

	// FMOD Systems
	FMOD::Studio::System* m_StudioSystem;
	FMOD::System* m_System;

	// Listener
	AudioListener m_Listener;

	// Sounds
	std::unordered_map<std::string, FMOD::Sound*> m_SoundMap;

	// Banks
	std::unordered_map<std::string, FMOD::Studio::Bank*> m_BankMap;

	// Events
	std::unordered_map<std::string, AudioEvent*> m_EventMap;

};
