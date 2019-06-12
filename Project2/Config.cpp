#include "Config.h"
#include <iostream>
#include <fstream>

void Config::saveToSettingsFile(const std::string & presetName)
{
	//open the settings file
	std::ifstream settings(m_settingsFile);

	std::ofstream settings_tmp(m_settingsFile + ".tmp");
	if (!settings_tmp.is_open())
	{
		__debugbreak();
		return;
	}

	//check the file to see if the preset already exists
	bool presetExists = gotoPresetLine(settings, presetName, &settings_tmp);
	std::string lineStr;
	if (!presetExists)
	{
		settings_tmp << "preset_" << presetName << std::endl;
	}
	if (saveVisInfo)
	{
		if (presetExists)
		{
			for (int l = 0; l < 4; l++) std::getline(settings, lineStr);
		}
		settings_tmp << gradient << std::endl;
		settings_tmp << gradCol1.r << gradCol1.g << gradCol1.b << std::endl;
		settings_tmp << gradCol2.r << gradCol2.g << gradCol2.b << std::endl;
		for (auto& v : m_visualisers)
		{
			if (v.inCycle)
			{
				settings_tmp << v.id << ",";
			}
		}
		settings_tmp << std::endl;
	}
	else
	{
		if (presetExists)
		{
			for (int l = 0; l < 4; l++)
			{
				std::getline(settings, lineStr);
				if (clearVisInfo)  settings_tmp << "-" << std::endl;
				else settings_tmp << lineStr << std::endl;
			}
		}
		else
		{
			for (int l = 0; l < 4; l++) settings_tmp << "-" << std::endl;
		}
		
	}
	if (saveWindowInfo)
	{
		if (presetExists)
		{
			for (int l = 0; l < 5; l++) std::getline(settings, lineStr);
		}
		settings_tmp << m_currentWindow->getPosition().x << "," << m_currentWindow->getPosition().y << std::endl;
		settings_tmp << m_currentWindow->getSize().x << "," << m_currentWindow->getSize().y << std::endl;
		settings_tmp << alwaysOnTop << std::endl;
		settings_tmp << transparent << std::endl;
		settings_tmp << minOpacity << std::endl;
	}
	else
	{
		if (presetExists)
		{
			for (int l = 0; l < 5; l++)
			{
				std::getline(settings, lineStr);
				if(clearWindowInfo)  settings_tmp << "-" << std::endl;
				else settings_tmp << lineStr << std::endl;
			}
		}
		else
		{
			for (int l = 0; l < 5; l++)	settings_tmp << "-" << std::endl;
		}
		
	}


	if (presetExists)
	{
		//copy the rest of the file to the tmp file
		while (std::getline(settings, lineStr))
		{
			settings_tmp << lineStr << std::endl;
		}
	}

	settings_tmp.close();
	settings.close();

	//replace the old file with the new one
	remove(m_settingsFile.c_str());
	rename((m_settingsFile + ".tmp").c_str(), m_settingsFile.c_str());

}

void Config::loadFromSettingsFile(const std::string & presetName)
{
	//open the settings file
	std::ifstream settings;
	settings.open(m_settingsFile);
	if (!settings.is_open())
		return;

	//find the preset
	int presetLine = 0;
	bool presetExists = gotoPresetLine(settings, presetName);

	if (!presetExists)
		return;

	std::string lineStr;
	std::getline(settings, lineStr); //gradient
	if (lineStr != "-")
	{
		gradient = lineStr == "1";
	}
	
	std::getline(settings, lineStr); //gradCol1
	if (lineStr != "-")
	{
		gradCol1 = sf::Color(lineStr[0], lineStr[1], lineStr[2]);
	}
	
	std::getline(settings, lineStr); //gradCol2
	if (lineStr != "-")
	{
		gradCol2 = sf::Color(lineStr[0], lineStr[1], lineStr[2]);
	}
	
	std::getline(settings, lineStr); //visualisers
	if (lineStr != "-")
	{
		int pos = 0;
		for (auto& v : m_visualisers)
		{
			v.inCycle = false;
		}
		while (!lineStr.empty() && (pos = lineStr.find(",")) != std::string::npos)
		{
			std::string visName = lineStr.substr(0, pos);
			for (auto& v : m_visualisers)
			{
				if (v.id == visName)
				{
					v.inCycle = true;
					break;
				}
			}
			lineStr.erase(0, pos + 1);
		}
	}
	
	std::getline(settings, lineStr); //window pos
	if (lineStr != "-")
	{
		int commaPos = lineStr.find(",");
		scrX = atoi(lineStr.substr(0, commaPos).c_str());
		scrY = atoi(lineStr.substr(commaPos+1).c_str());
		windowSettingsChanged = true;
	}
	std::getline(settings, lineStr); //window size
	if (lineStr != "-")
	{
		int commaPos = lineStr.find(",");
		minScrW = atoi(lineStr.substr(0, commaPos).c_str());
		minScrH = atoi(lineStr.substr(commaPos + 1).c_str());
		windowSettingsChanged = true;
	}
	std::getline(settings, lineStr); //alwaysOnTop
	if (lineStr != "-")
	{
		alwaysOnTop = lineStr == "1";
		windowSettingsChanged = true;
	}
	std::getline(settings, lineStr); //transparent
	if (lineStr != "-")
	{
		transparent = lineStr == "1";
		windowSettingsChanged = true;
	}
	std::getline(settings, lineStr); //minopacity
	if (lineStr != "-")
	{
		minOpacity = atof(lineStr.c_str());
		windowSettingsChanged = true;
	}

	settings.close();
}

void Config::loadPresetList()
{
	//open the settings file
	std::ifstream settings;
	settings.open(m_settingsFile);
	if (!settings.is_open())
		return;

	m_presetNames.clear();

	std::string lineStr;
	while (std::getline(settings, lineStr))
	{
		if (lineStr.find("preset_") != std::string::npos)
		{
			lineStr.erase(0u, 7u);
			m_presetNames.push_back(lineStr);
		}
	}

	settings.close();
}

bool Config::gotoPresetLine(std::ifstream & file, const std::string & presetName, std::ofstream* tmp)
{
	std::string lineStr;
	std::string filePresetName = "preset_" + presetName;
	bool presetExists(false);
	while (std::getline(file, lineStr))
	{
		if (tmp)
		{
			(*tmp) << lineStr << std::endl;
		}

		if (lineStr == filePresetName)
		{
			presetExists = true;
			break;
		}
	}
	return presetExists;
}
