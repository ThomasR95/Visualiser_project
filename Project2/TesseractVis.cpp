#include "TesseractVis.h"
#include <iostream>

TesseractVis::TesseractVis() : Visualiser()
{
}


TesseractVis::~TesseractVis()
{
}


void TesseractVis::init(sf::RenderWindow * wind, sf::RenderTexture * rTex, Config* config)
{
	m_window = wind;
	m_RT = rTex;
	gameConfig = config;

	m_timer.restart();

	if (!m_textures.count("space"))
	{
		m_textures["space"] = std::make_shared<sf::Texture>();
		m_textures["space"]->loadFromFile("space.jpg");
		m_textures["space"]->setSmooth(true);
		m_textures["space"]->setRepeated(true);
	}
	m_skyPlane.setTexture(m_textures["space"].get(), false);

	states.shader = &m_shader;
	addStates.blendMode = sf::BlendAdd;

	elapsed = sf::seconds(0);
	m_timer.restart();

	m_radians = 0;
	m_radiansPerSec = 0.5f*PI;

	rgb = { 0, 0, 0 };

	m_line.setPrimitiveType(sf::LineStrip);

	leftOverflow = 0;
	m_rot = 0;
}

void TesseractVis::render(float frameHi, float frameAverage, float frameMax, sf::Texture* bgImage)
{
	m_window->clear({ 0,0,0,0 });

	float scale = frameHi / frameMax;

	auto dt = m_timer.restart();
	elapsed += dt;

	m_radians += dt.asSeconds()*m_radiansPerSec;
	if (m_radians >= 2*PI)
		m_radians -= 2*PI;

	

	float mult = frameMax / frameAverage;
	if (mult > m_scrW / 20)
		mult = m_scrW / 20;
	if (mult < 1)
		mult = 1;

	float radius = (frameHi / frameMax)*(m_scrW / 9);
	radius += m_scrW / 15;
	float lineX = m_scrW / 2 + radius*cos(m_radians);
	float lineY = m_scrH / 2 + radius*sin(m_radians);
	linePos = { lineX, lineY };

	m_line.clear();
	m_line.append(sf::Vertex(lastLinePos));
	m_line.append(sf::Vertex(linePos));

	lastLinePos = linePos;

	if (gameConfig->trebleHi > 1.25f*gameConfig->trebleAverage && scale > (0.1f))
	{
		int dist = m_scrW / 7;
		dist *= (gameConfig->trebleHi / gameConfig->trebleAverage);

		int numSpots = ceil(gameConfig->trebleHi / gameConfig->trebleAverage);
		if (numSpots > 10)
			numSpots = 10;
		while (numSpots--)
		{
			auto pos = sf::Vector2f(m_scrW / 2, m_scrH / 2) + sf::Vector2f((rand() % dist) - (dist / 2), (rand() % dist) - (dist / 2));
			if (pos.x > m_scrW - 10 * scale)
				pos.x = m_scrW - 10 * scale;
			if (pos.y > m_scrH - 10 * scale)
				pos.y = m_scrH - 10 * scale;


			float r = (m_spotRadius / 10.f)*scale;
			auto newSpot = sf::CircleShape(r);
			newSpot.setOrigin({ r, r });
			newSpot.setPosition(sf::Vector2f(pos));
			if (spots.size() < 10)
				spots.push_back(newSpot);
			else
				break;
		}
	}

	if (frameHi > 1.25f*frameAverage && scale>(0.1f))
	{
		int chance = rand() % 2;
		if (chance == 1 || m_rot == 0)
			m_rot = ((float)(rand() % 15) / 10.f) - 1;
		else
			m_rot = 0;
	}

	mainColourChange(rgb, frameHi, frameAverage, frameMax);

	m_shader.setUniform("time", elapsed.asSeconds());

	
	float baseMove = 50 * dt.asSeconds();

	float leftF = (baseMove * m_ratio) + leftOverflow;
	int left = floor(leftF);
	float topF = baseMove + topOverflow;
	int top = floor(topF);
	leftOverflow = leftF - left;
	topOverflow = topF - top;
	tRect.top += top;
	tRect.left += left;
	tRect.height -= 2*top;
	tRect.width -= 2*left;
	m_RTPlane.setTextureRect(sf::IntRect(tRect));

	m_RTPlane.setRotation(m_rot);


	float fade = ((2 * frameAverage) / frameMax)*m_scrW;
	m_shader.setUniform("fadeDist", fade);
	float opacity = 0.01f;
	float newOp = 0.02 - (scale / 20);
	if (newOp > opacity)
		opacity = newOp;
	m_shader.setUniform("opacity", opacity);

	float baseColour = 500.f - (500.f * (mult / (m_scrW / 20)));
	if (baseColour > 250)
		baseColour = 250;

	m_RTPlane.setFillColor({ (sf::Uint8)(baseColour + rgb[0] * 5), (sf::Uint8)(baseColour + rgb[1] * 5), (sf::Uint8)(baseColour + rgb[2] * 5), 200 });
	m_RT->draw(m_RTPlane);
	m_RT->display();
	tRect.top -= top;
	tRect.left -= left;
	tRect.height += 2 * top;
	tRect.width += 2 * left;
	m_RTPlane.setTextureRect(sf::IntRect(tRect));
	m_RTPlane.setFillColor({ 255, 255, 255, 255 });
	m_RTPlane.setRotation(0);

	m_skyPlane.setFillColor({ (sf::Uint8)(rgb[0] * 200), (sf::Uint8)(rgb[1] * 180), (sf::Uint8)(rgb[2] * 180), 8 });

	m_RT->draw(m_skyPlane);
	m_RT->draw(m_shaderPlane, states);

	//m_RT->draw(m_rightBar);
	//m_RT->draw(m_bottomBar);

	m_RT->display();

	int si = 0;
	for (int si = 0; si < spots.size(); si++)
	{
		m_RT->draw(spots[si]);
	}
	spots.clear();
	m_RT->draw(m_line);
	m_RT->display();


	m_transparentShader.setUniform("minOpacity", gameConfig->minOpacity);
	if (gameConfig->transparent)
		m_window->draw(m_RTPlane, m_transparentStates);
	else
		m_window->draw(m_RTPlane);

	m_transparentShader.setUniform("minOpacity", 0);
	m_skyPlane.setFillColor({ (sf::Uint8)(rgb[0] * 255), (sf::Uint8)(rgb[1] * 200), (sf::Uint8)(rgb[2] * 200), sf::Uint8(100 * scale) });
	m_window->draw(m_skyPlane, addStates);
}

void TesseractVis::resetPositions(float scrW, float scrH, float ratio)
{
	m_scrW = scrW;
	m_scrH = scrH;
	m_ratio = ratio;

	float dist = 3.5;

	float lineX = m_scrW / 2;
	float lineY = m_scrH / 2;
	linePos = { lineX, lineY };
	lastLinePos = linePos;

	m_spotRadius = scrW / 50;

	m_RTPlane.setSize({ scrW, scrH });
	m_shaderPlane.setSize(m_RTPlane.getSize());
	m_skyPlane.setSize({ scrW, scrH });

	skyRect = sf::FloatRect(0, 0, scrW / 2, scrH / 2);
	m_skyPlane.setTextureRect(sf::IntRect(skyRect));
	m_skyPlane.setTexture(m_textures["space"].get(), false);

	m_RTPlane.setTexture(&m_RT->getTexture());
	tRect = sf::FloatRect(m_RTPlane.getTextureRect());
	tRect.left = 0;
	tRect.top = 0;
	tRect.width = scrW;
	tRect.height = scrH;
	m_RTPlane.setTextureRect(sf::IntRect(tRect));
	m_RTPlane.setOrigin(scrW / 2, scrH / 2);
	m_RTPlane.setPosition({ scrW / 2, scrH / 2 });
	m_RTPlane.setScale({ 1,1 });

	m_rightBar = sf::RectangleShape({ 1, scrH });
	m_rightBar.setPosition({ scrW - 1, 0 });
	m_rightBar.setFillColor({ 0,0,0,255 });
	m_bottomBar = sf::RectangleShape({ scrW, 1 });
	m_bottomBar.setPosition(0, scrH - 1);
	m_bottomBar.setFillColor({ 0,0,0,255 });

	leftOverflow = 0;
	topOverflow = 0;
}

void TesseractVis::reloadShader()
{
	if (!m_shader.loadFromFile("Shaders/wave_square.frag", sf::Shader::Type::Fragment))
		std::cout << "Shader borken";

	sf::Shader::bind(&m_shader);

	m_shader.setUniform("amplitude", 1.f);
	m_shader.setUniform("waveWidth", m_scrW / 70);
	m_shader.setUniform("speed", 3.f);
	m_shader.setUniform("opacity", 0.01f);
	m_shader.setUniform("scrw", m_scrW);
	m_shader.setUniform("scrh", m_scrH);

	m_shader.setUniform("blackBetweenWaves", true);

	mainColourChange(rgb, gameConfig->frameHi, 0, gameConfig->frameMax);

	m_shader.setUniform("inColour", sf::Glsl::Vec4(rgb[0], rgb[1], rgb[2], 1.f));

	m_shader.setUniform("point", sf::Glsl::Vec2(m_scrW/2, m_scrH/2));

	float fade = 0.3f*m_scrW;
	m_shader.setUniform("fadeDist", fade);

	if (!m_transparentShader.loadFromFile("Shaders/BlackToAlpha.frag", sf::Shader::Type::Fragment))
		std::cout << "Shader borken";

	m_transparentStates.shader = &m_transparentShader;
	addStates.shader = &m_transparentShader;
}
