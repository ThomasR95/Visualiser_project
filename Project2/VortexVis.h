#pragma once
#include "Visualiser.h"
#include <deque>
class VortexVis :
	public Visualiser
{
public:
	VortexVis();
	~VortexVis();
	void init(sf::RenderWindow * wind, sf::RenderTexture * rTex);
	void render(float frameHi, float frameAverage, float frameMax);
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
	sf::RectangleShape m_topBar;
	sf::RectangleShape m_bottomBar;

	sf::CircleShape m_Hole;
	sf::CircleShape m_Rim;

	sf::RenderStates states = sf::RenderStates::Default;
	sf::RenderStates addStates = sf::RenderStates::Default;
	sf::RenderStates contrastStates = sf::RenderStates::Default;

	sf::Clock m_timer;

	float leftOverflow;
	float topOverflow;

	float m_frameAverage;

	float m_rot;

	sf::RectangleShape m_ringBox;
	bool ringFadeUp;
	bool ringFadeDown;
	float ringFadeMult;
	float ringFadeFactor;
	float ringScaleMult;

	
	
	
};

