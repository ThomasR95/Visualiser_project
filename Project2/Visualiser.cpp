#include "Config.h"
#include "Visualiser.h"

Visualiser::Visualiser()
{
}


Visualiser::~Visualiser()
{
}

void Visualiser::init(sf::RenderWindow * wind, sf::RenderTexture * rTex, Config* config)
{
}

void Visualiser::resetPositions(float scrW, float scrH, float ratio)
{
}

void Visualiser::render(float frameHi, float frameAverage, float frameMax, sf::Texture* bgImage)
{
}

void Visualiser::reloadShader()
{
}

void Visualiser::getRandomColour(std::vector<float>& rgb)
{
	bool useGradient = gameConfig->gradient;
	auto& col1 = gameConfig->gradCol1;
	auto& col2 = gameConfig->gradCol2;

	if (useGradient)
	{
		float rDist = float(rand() % 100) / 100.f;
		rgb[0] = (float)(col1.r + (float)(col2.r - col1.r)*rDist) / 255.f;
		rgb[1] = (float)(col1.g + (float)(col2.g - col1.g)*rDist) / 255.f;
		rgb[2] = (float)(col1.b + (float)(col2.b - col1.b)*rDist) / 255.f;
	}
	else
	{
		int full = rand() % 3;
		int none = full;
		while (none == full)
			none = rand() % 3;
		int ran = -1;
		for (int r = 0; r < 3; r++)
			if (r != full && r != none)
				ran = r;

		rgb[full] = 1;
		rgb[none] = 0;
		rgb[ran] = (float)(rand() % 255) / 255.f;
	}
}

void Visualiser::getLoudnessColour(std::vector<float>& rgb, float soundLevel, float minLevel, float maxLevel)
{
	float soundRange = maxLevel - minLevel;
	float realLevel = (soundLevel - minLevel);
	float factor = realLevel / soundRange;
	factor = std::min(1.0f, std::max(0.0f, factor));

	sf::Color col1 = gameConfig->gradCol1;
	sf::Color col2 = gameConfig->gradCol2;
	sf::Color colHalf = gameConfig->gradColHalf;

	float lowHalf = soundRange*gameConfig->gradientMidPoint;
	float highHalf = soundRange - lowHalf;

	if (realLevel < lowHalf)
	{
		col2 = colHalf;
		factor = realLevel / lowHalf;
		factor = std::min(1.0f, std::max(0.0f, factor));
	}
	else
	{
		col1 = colHalf;
		factor = (realLevel-lowHalf) / highHalf;
		factor = std::min(1.0f, std::max(0.0f, factor));
	}


	rgb[0] = (float)(col1.r + (float)(col2.r - col1.r)*factor) / 255.f;
	rgb[1] = (float)(col1.g + (float)(col2.g - col1.g)*factor) / 255.f;
	rgb[2] = (float)(col1.b + (float)(col2.b - col1.b)*factor) / 255.f;
}

bool Visualiser::mainColourChange(std::vector<float>& rgb, float frameHi, float frameAverage, float frameMax)
{
	bool colourChanged = false;
	if (gameConfig->gradientLoudness && gameConfig->gradient)
	{
		getLoudnessColour(rgb, frameHi, 0.0f, frameMax);
		colourChanged = true;
	}
	else if (frameHi > 2.f*frameAverage)
	{
		getRandomColour(rgb);
		colourChanged = true;
	}

	if (colourChanged)
	{
		m_shader.setUniform("inColour", sf::Glsl::Vec4(rgb[1], rgb[2], rgb[0], 1.f));
	}
	return colourChanged;
}
