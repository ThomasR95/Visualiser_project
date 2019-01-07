#include "Visualiser.h"

Visualiser::Visualiser()
{
}


Visualiser::~Visualiser()
{
}

void Visualiser::init(sf::RenderWindow * wind, sf::RenderTexture * rTex)
{
}

void Visualiser::resetPositions(float scrW, float scrH, float ratio)
{
}

void Visualiser::render(float frameHi, float frameAverage, float frameMax)
{
}

void Visualiser::reloadShader()
{
}

void Visualiser::getRandomColour(std::vector<float>& rgb, bool useGradient, sf::Color* col1, sf::Color* col2)
{
	if (useGradient && col1 && col2)
	{
		float rDist = float(rand() % 100) / 100.f;
		rgb[0] = (float)(col1->r + (float)(col2->r - col1->r)*rDist) / 255.f;
		rgb[1] = (float)(col1->g + (float)(col2->g - col1->g)*rDist) / 255.f;
		rgb[2] = (float)(col1->b + (float)(col2->b - col1->b)*rDist) / 255.f;
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
