#pragma once
#include "Config.h"
#include "Visualiser.h"
#include <deque>
#include "FrequencyWave.h"

class SingularityVis :
	public Visualiser
{
public:
	SingularityVis();
	~SingularityVis();
	void init(sf::RenderWindow * wind, sf::RenderTexture * rTex, Config* config);
	void render(float frameHi, float frameAverage, float frameMax, sf::Texture* bgImage) override;
	void resetPositions(float scrW, float scrH, float ratio);
	void reloadShader();
	
	sf::Time elapsed;

	sf::RectangleShape m_RTPlane;
	sf::RectangleShape m_shaderPlane;
	sf::RectangleShape m_skyPlane;

	sf::RenderTexture m_RT2;
	sf::RectangleShape m_RTPlane2;

	sf::Shader m_shader2;

	sf::FloatRect tRect;
	sf::FloatRect skyRect;

	std::vector<float> rgb;

	float m_degrees;
	float m_degreesPerSec;
	float m_degreesPerFrame;

	sf::RenderStates states = sf::RenderStates::Default;
	sf::RenderStates addStates = sf::RenderStates::Default;
	sf::RenderStates contrastStates = sf::RenderStates::Default;

	sf::Clock m_timer;

	float leftOverflow;
	float topOverflow;

	float m_frameAverage;

	float m_offset;

	float m_rot;
	
	FrequencyWave m_waveform;

	float m_betweenBeatGain;
	
	
};

