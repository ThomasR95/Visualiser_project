#include "xmlConfig.h"

using namespace tinyxml2;

xmlConfigLoader::xmlConfigLoader(Config* gameConfig, const std::string& settingsXMLFile) :
	m_gameConfig(gameConfig),
	m_settingsFileName(settingsXMLFile)
{
}


xmlConfigLoader::~xmlConfigLoader()
{
}

bool xmlConfigLoader::loadCommon()
{
	XMLDocument doc;

	doc.LoadFile(m_settingsFileName.c_str());

	auto root = doc.FirstChildElement("Config");
	if (root)
	{
		auto common = root->FirstChildElement("Common");
		if (common)
		{
			common->QueryBoolAttribute("startMaximised", &m_gameConfig->startMaximised);
			common->QueryFloatAttribute("lastWidth", &m_gameConfig->minScrW);
			common->QueryFloatAttribute("lastHeight", &m_gameConfig->minScrH);
			common->QueryIntAttribute("lastX", &m_gameConfig->scrX);
			common->QueryIntAttribute("lastY", &m_gameConfig->scrY);
			common->QueryBoolAttribute("alwaysOnTop", &m_gameConfig->alwaysOnTop);
			common->QueryIntAttribute("lastAudioDevice", &m_gameConfig->devIdx);
		}
	}
	else return false;


	return true;
}

bool xmlConfigLoader::saveCommon()
{
	XMLDocument doc;

	doc.LoadFile(m_settingsFileName.c_str());

	auto root = doc.FirstChildElement("Config");
	if (!root) root = doc.InsertFirstChild(doc.NewElement("Config"))->ToElement();
	if (root)
	{
		auto common = root->FirstChildElement("Common");
		if (!common) common = doc.NewElement("Common");
		common = root->InsertFirstChild(common)->ToElement();
		if (common)
		{
			common->SetAttribute("startMaximised", m_gameConfig->startMaximised);
			common->SetAttribute("lastWidth", m_gameConfig->minScrW);
			common->SetAttribute("lastHeight", m_gameConfig->minScrH);
			common->SetAttribute("lastX", m_gameConfig->scrX);
			common->SetAttribute("lastY", m_gameConfig->scrY);
			common->SetAttribute("alwaysOnTop", m_gameConfig->alwaysOnTop);
			common->SetAttribute("lastAudioDevice", m_gameConfig->devIdx);
		}
	}
	else return false;
	
	doc.SaveFile(m_settingsFileName.c_str());

	return true;
}

bool xmlConfigLoader::loadPresetNames()
{
	XMLDocument doc;

	m_gameConfig->m_presetNames.clear();

	doc.LoadFile(m_settingsFileName.c_str());

	auto root = doc.FirstChildElement("Config");
	if (!root) return false;

	auto presets = root->FirstChildElement("Presets");
	if (!presets) return false;

	auto preset = presets->FirstChildElement("Preset");
	while (preset)
	{
		m_gameConfig->m_presetNames.push_back(preset->Attribute("Name"));
		preset = preset->NextSiblingElement("Preset");
	}
	
	return true;
}

bool xmlConfigLoader::loadPreset(const std::string & presetName)
{
	XMLDocument doc;
	doc.LoadFile(m_settingsFileName.c_str());

	auto root = doc.FirstChildElement("Config");
	if (!root) root = doc.InsertFirstChild(doc.NewElement("Config"))->ToElement();
	if (!root) return false;

	auto presets = root->FirstChildElement("Presets");
	if (!presets) presets = root->InsertEndChild(doc.NewElement("Presets"))->ToElement();
	if (!presets) return false;

	auto preset = presets->FirstChildElement("Preset");

	//Check to see if a preset with this name exists already
	while (preset)
	{
		if (preset->Attribute("Name") == presetName)
		{
			break;
		}
		preset = preset->NextSiblingElement("Preset");
	}

	if (!preset)
	{
		//preset doesn't exist
		return false;
	}

	preset->QueryBoolAttribute("AffectsVisuals", &m_gameConfig->saveVisInfo);
	preset->QueryBoolAttribute("AffectsWindow", &m_gameConfig->saveWindowInfo);

	if (m_gameConfig->saveVisInfo)
	{
		auto gradient = preset->FirstChildElement("Gradient");
		if (gradient)
		{
			gradient->QueryBoolAttribute("enabled", &m_gameConfig->gradient);
			gradient->QueryBoolAttribute("isLoudness", &m_gameConfig->gradientLoudness);
			int col1, col2;
			gradient->QueryIntAttribute("col1", &col1);
			gradient->QueryIntAttribute("col2", &col2);
			m_gameConfig->gradCol1 = sf::Color((sf::Uint32)col1);
			m_gameConfig->gradCol2 = sf::Color((sf::Uint32)col2);
		}
		auto visCycle = preset->FirstChildElement("VisCycle");
		if (visCycle) 
		{
			for (auto& vis : m_gameConfig->m_visualisers)
			{
				vis.inCycle = false;
			}
			auto vID = visCycle->FirstChildElement("vID");

			while (vID)
			{
				std::string visName = vID->GetText();
				for (auto& vis : m_gameConfig->m_visualisers)
				{
					if (vis.id == visName)
					{
						vis.inCycle = true;
						break;
					}
				}

				vID = vID->NextSiblingElement("vID");
			}
		}
	}
	if (m_gameConfig->saveWindowInfo)
	{
		auto window = preset->FirstChildElement("Window");
		if (window)
		{
			window->QueryIntAttribute("x", &m_gameConfig->scrX);
			window->QueryIntAttribute("y", &m_gameConfig->scrY);
			window->QueryFloatAttribute("w", &m_gameConfig->minScrW);
			window->QueryFloatAttribute("h", &m_gameConfig->minScrH);
			window->QueryBoolAttribute("AlwaysOnTop", &m_gameConfig->alwaysOnTop);
			window->QueryBoolAttribute("Transparent", &m_gameConfig->transparent);
			window->QueryFloatAttribute("MinOpacity", &m_gameConfig->minOpacity);

			m_gameConfig->windowSettingsChanged = true;
		}
	}

	return true;
}

bool xmlConfigLoader::savePreset(const std::string & presetName)
{
	XMLDocument doc;
	doc.LoadFile(m_settingsFileName.c_str());

	auto root = doc.FirstChildElement("Config");
	if (!root) root = doc.InsertFirstChild(doc.NewElement("Config"))->ToElement();
	if (!root) return false;

	auto presets = root->FirstChildElement("Presets");
	if (!presets) presets = root->InsertEndChild(doc.NewElement("Presets"))->ToElement();
	if (!presets) return false;

	auto preset = presets->FirstChildElement("Preset");

	//Check to see if a preset with this name exists already
	while (preset)
	{
		if (preset->Attribute("Name") == presetName)
		{
			break;
		}
		preset = preset->NextSiblingElement("Preset");
	}

	if (!preset)
	{
		preset = presets->InsertEndChild(doc.NewElement("Preset"))->ToElement();
	}

	
	preset->SetAttribute("Name", presetName.c_str());
	preset->SetAttribute("AffectsVisuals", m_gameConfig->saveVisInfo);
	preset->SetAttribute("AffectsWindow", m_gameConfig->saveWindowInfo);

	if (m_gameConfig->saveVisInfo)
	{
		auto gradient = preset->FirstChildElement("Gradient");
		if(!gradient) gradient = preset->InsertFirstChild(doc.NewElement("Gradient"))->ToElement();

		gradient->SetAttribute("enabled", m_gameConfig->gradient);
		gradient->SetAttribute("isLoudness", m_gameConfig->gradientLoudness);
		gradient->SetAttribute("col1", m_gameConfig->gradCol1.toInteger());
		gradient->SetAttribute("col2", m_gameConfig->gradCol2.toInteger());

		//just delete and recreate this one for simplicity
		auto visCycle = preset->FirstChildElement("VisCycle");
		if (visCycle) preset->DeleteChild(visCycle);
		visCycle = preset->InsertAfterChild(gradient, doc.NewElement("VisCycle"))->ToElement();

		for (auto& v : m_gameConfig->m_visualisers)
		{
			if (v.inCycle)
			{
				auto vID = visCycle->InsertEndChild(doc.NewElement("vID"))->ToElement();
				vID->SetText(v.id.c_str());
			}
		}
	}
	if (m_gameConfig->saveWindowInfo)
	{
		auto window = preset->FirstChildElement("Window");
		if (!window) window = preset->InsertEndChild(doc.NewElement("Window"))->ToElement();

		window->SetAttribute("x", m_gameConfig->scrX);
		window->SetAttribute("y", m_gameConfig->scrY);
		window->SetAttribute("w", m_gameConfig->minScrW);
		window->SetAttribute("h", m_gameConfig->minScrH);
		window->SetAttribute("AlwaysOnTop", m_gameConfig->alwaysOnTop);
		window->SetAttribute("Transparent", m_gameConfig->transparent);
		window->SetAttribute("MinOpacity", m_gameConfig->minOpacity);
	}

	doc.SaveFile(m_settingsFileName.c_str());

	return true;
}
