#pragma once
#include "Gameplay/Components/IComponent.h"
//#include "fmod.h"
#include "fmod_studio.hpp"

//Store per-sound info
class AudioClip : public IResource
{
public:
	typedef std::shared_ptr<AudioClip> Sptr;

	// Default constructor
	AudioClip();
	/// <summary>
	/// Constructor for loading from file
	/// </summary>
	/// <param name="filename"></param>
	AudioClip(const std::string& filename);


	virtual nlohmann::json ToJson() const override;
	static AudioClip::Sptr FromJson(const nlohmann::json& blob);

	FMOD::Sound* m_pSound;
};

