#pragma once
#include "SFML/Graphics.hpp"
#include <map>
#include <memory>

class Config;

class Visualiser
{
public:
	Visualiser();
	~Visualiser();

	virtual void init(sf::RenderWindow* wind, sf::RenderTexture* rTex, Config* config);

	virtual void resetPositions(float scrW, float scrH, float ratio);

	virtual void render(float frameHi, float frameAverage, float frameMax, sf::Texture* bgImage = nullptr);

	virtual void reloadShader();

protected:

	void getRandomColour(std::vector<float>& rgb);

	void getLoudnessColour(std::vector<float>& rgb, float soundLevel, float minLevel, float maxLevel);

	// pass 0 to frameAverage to force a colour change
	// return true if colour changed
	bool mainColourChange(std::vector<float>& rgb, float frameHi, float frameAverage, float frameMax);

	sf::RenderWindow * m_window;
	sf::RenderTexture* m_RT;
	sf::Shader m_shader;
	float m_scrW;
	float m_scrH;
	float m_ratio;

	Config* gameConfig = nullptr;

	std::map<std::string, std::shared_ptr<sf::Texture>> m_textures;

	sf::Shader m_transparentShader;
	sf::RenderStates m_transparentStates;
};

