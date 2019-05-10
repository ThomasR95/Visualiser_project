#pragma once
#include "Visualiser.h"
#include <deque>
#include "FrequencyWave.h"
class StarFallVis :
	public Visualiser
{
public:
	StarFallVis();
	~StarFallVis();
	void init(sf::RenderWindow * wind, sf::RenderTexture * rTex);
	void render(float frameHi, float frameAverage, float frameMax);
	void resetPositions(float scrW, float scrH, float ratio);
	void reloadShader();
	

	sf::RectangleShape m_RTPlane;
	sf::RectangleShape m_shaderPlane;
	sf::RectangleShape m_skyPlane;
	sf::RectangleShape m_rightBar;
	sf::RectangleShape m_bottomBar;
	sf::FloatRect tRect;
	sf::FloatRect skyRect;

	std::vector<float> rgb;

	float m_spotRadius = 50.f;
	sf::CircleShape m_spot;
	sf::Vector2f spotPos;
	std::deque<sf::CircleShape> spots;

	sf::RenderStates states = sf::RenderStates::Default;
	sf::RenderStates addStates = sf::RenderStates::Default;

	sf::Clock m_timer;
	sf::Time elapsed;

	float catchupR;
	float catchupT;

	FrequencyWave m_waveform;
	
};

