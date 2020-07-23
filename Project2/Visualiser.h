#pragma once
#include "SFML/Graphics.hpp"
#include <map>
#include <memory>

class Visualiser
{
public:
	Visualiser();
	~Visualiser();

	virtual void init(sf::RenderWindow* wind, sf::RenderTexture* rTex);

	virtual void resetPositions(float scrW, float scrH, float ratio);

	virtual void render(float frameHi, float frameAverage, float frameMax, sf::Texture* bgImage = nullptr);

	virtual void reloadShader();

protected:

	void getRandomColour(std::vector<float>& rgb, bool useGradient = false, sf::Color* col1 = nullptr, sf::Color* col2 = nullptr);

	sf::RenderWindow * m_window;
	sf::RenderTexture* m_RT;
	sf::Shader m_shader;
	float m_scrW;
	float m_scrH;
	float m_ratio;

	std::map<std::string, std::shared_ptr<sf::Texture>> m_textures;

	sf::Shader m_transparentShader;
	sf::RenderStates m_transparentStates;
};

