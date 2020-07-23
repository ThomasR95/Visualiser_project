#pragma once
#include "Visualiser.h"
#include <deque>
#include "FrequencyWave.h"

class DoorwayVis :
	public Visualiser
{
public:
	DoorwayVis();
	~DoorwayVis();
	void init(sf::RenderWindow * wind, sf::RenderTexture * rTex);
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

	float m_boxWidth;
	float m_boxHeight;

	sf::RectangleShape m_topBox;
	sf::RectangleShape m_bottomBox;
	sf::RectangleShape m_leftBox;
	sf::RectangleShape m_rightBox;

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
	
	
};

