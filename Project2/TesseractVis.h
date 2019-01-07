#pragma once
#include "Visualiser.h"
#include <deque>
class TesseractVis :
	public Visualiser
{
public:
	TesseractVis();
	~TesseractVis();
	void init(sf::RenderWindow * wind, sf::RenderTexture * rTex);
	void render(float frameHi, float frameAverage, float frameMax);
	void resetPositions(float scrW, float scrH, float ratio);
	void reloadShader();
	
	sf::Time elapsed;

	sf::RectangleShape m_RTPlane;
	sf::RectangleShape m_shaderPlane;
	sf::RectangleShape m_skyPlane;
	sf::RectangleShape m_rightBar;
	sf::RectangleShape m_bottomBar;
	sf::FloatRect tRect;
	sf::FloatRect skyRect;

	std::vector<float> rgb;

	float m_radians;
	float m_radiansPerSec;

	float m_spotRadius = 50.f;
	sf::VertexArray m_line;
	sf::Vector2f linePos;
	sf::Vector2f lastLinePos;
	std::deque<sf::CircleShape> spots;

	sf::RenderStates states = sf::RenderStates::Default;
	sf::RenderStates addStates = sf::RenderStates::Default;

	sf::Clock m_timer;

	float leftOverflow;
	float topOverflow;

	float m_rot;

	
};

