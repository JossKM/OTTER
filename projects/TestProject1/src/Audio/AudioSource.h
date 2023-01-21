#pragma once
#include "Gameplay/Components/IComponent.h"
#include "AudioClip.h"

class AudioSource : public Gameplay::IComponent
{
	//sound clip
	AudioClip clip;
};

