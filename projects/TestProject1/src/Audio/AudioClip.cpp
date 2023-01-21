#include "AudioClip.h"

AudioClip::AudioClip()
{
}

AudioClip::AudioClip(const std::string& filename)
{
}

nlohmann::json AudioClip::ToJson() const
{
	return nlohmann::json();
}

AudioClip::Sptr AudioClip::FromJson(const nlohmann::json& blob)
{
	return AudioClip::Sptr();
}