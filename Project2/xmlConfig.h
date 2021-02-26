#pragma once

#include "Config.h"
#include "tinyxml2\tinyxml2.h"

class xmlConfigLoader
{
public:
	xmlConfigLoader(Config* gameConfig, const std::string& settingsXMLFile);
	~xmlConfigLoader();

	bool loadCommon();
	bool saveCommon();

	bool loadPresetNames();

	bool loadPreset(const std::string& presetName);
	bool savePreset(const std::string& presetName);

private:

	Config* m_gameConfig;
	std::string m_settingsFileName;
};

