#pragma once

#include "pa_util.h"
#include "pa_ringbuffer.h"
#include "portaudio.h"

#include "SFML/Graphics.hpp"
#include "SFML/Main.hpp"
#include "SFML/System.hpp"

#include "Visualiser.h"

#include <memory>
#include <deque>
#include <mutex>

#define PI 3.14159265359

#define SCRW 1280
#define SCRH 720

/* PortAudio setup */
#define NUM_CHANNELS    (2)
#define FRAMES_PER_BUFFER  512
#define SAMPLE_RATE 44100

#define PA_SAMPLE_TYPE  paFloat32
typedef float SAMPLE;
#define SAMPLE_SILENCE (0.0f)
#define PRINTF_S_FORMAT "%.8f"


class xmlConfigLoader;

typedef struct
{
	
 }
paTestData;

struct Config
{
	xmlConfigLoader* m_loader;

	//Window setup
#pragma region Window Setup
	bool transparent = false;

	float fullScrW;
	float fullScrH;
	float minScrW;
	float minScrH;
	float scrW;
	float scrH;
	float ratio;
	int scrX;
	int scrY;

	sf::RenderWindow m_window;
	sf::RenderWindow* m_currentWindow;
	sf::RenderTexture m_RT;

	sf::Shader m_shader;
	float minOpacity = 0.25f;

	bool startMaximised = false;
	bool isFullScreen = false;
	bool wasFullScreen = false;
	bool alwaysOnTop = false;
#pragma endregion

	//Audio variables
#pragma region Audio Variables
	float cutoff = 0.0006;

	SAMPLE frame = 0;
	SAMPLE runningAverage = 0.0001;
	SAMPLE runningMax = 0.0001;
	SAMPLE frameMax = runningMax;

	std::vector<SAMPLE> frames;
	std::vector<SAMPLE> FFTdata;
	std::vector<SAMPLE> FrequencyData;
	std::mutex FreqDataMutex;
	SAMPLE bassHi;
	SAMPLE bassMax;
	SAMPLE bassAverage;

	SAMPLE trebleHi;
	SAMPLE trebleMax;
	SAMPLE trebleAverage;

	SAMPLE frameHi;
	PaStreamParameters params;
	PaDeviceIndex devIdx = -1;
	int nDevices;
	std::vector<std::pair<std::string, int>> deviceList;
	paTestData *streamData;
	PaStream* AudioStr;
	bool leftChannel = true;
	int numChannels = 2;
#pragma endregion
	
	//Visualisers
#pragma region Visualisers
	struct visItem
	{
		std::string id;
		bool inCycle;
		std::shared_ptr<Visualiser> vis;
	};

	std::deque<visItem> m_visualisers;
	Visualiser* m_currentVis;
	int cycleTime = 30;
	bool cycleVis = true;
	int visIdx = 0;
	sf::Clock visTimer;

	sf::Clock quietTimer;


	bool gradient = false;
	bool gradientLoudness = false;
	sf::Color gradCol1 = { 255,0,0,255 };
	sf::Color gradCol2 = { 0,0,255,255 };

	std::shared_ptr<sf::Image> backgroundImage;
	std::shared_ptr<sf::Texture> backgroundTexture;
	std::string backgroundImageName;
#pragma endregion

#pragma region Menu settings
	//UI stuff
	sf::Image ico;

	bool menuShowing = false;
	bool advancedMenuShowing = false;
	bool showFPS = false;
	bool enableVSync = true;
	bool firstMenu = true;
	sf::Color* editingColour;

	sf::RectangleShape topLeftBox;
	sf::RectangleShape bottomRightBox;
	sf::RectangleShape resizeBox;
	std::pair<bool, bool> cornerGrabbed = { false, false };
	sf::Vector2i lastMiddleClickPosition = { -1, -1 };

	sf::Clock m_timer;

	std::string m_settingsFile;
	std::vector<std::string> m_presetNames;
	int m_presetIdx;
	std::vector<char> m_settingsFileBoxName;
	bool saveVisInfo = true;
	bool saveWindowInfo = false;
	bool clearVisInfo = false;
	bool clearWindowInfo = false;
	bool windowSettingsChanged = false;

#pragma endregion

	//debug audio bars
	std::vector<sf::RectangleShape> bars;


};
