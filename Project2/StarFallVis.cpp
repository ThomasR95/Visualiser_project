#include "StarFallVis.h"
#include <iostream>

#include "Config.h"
extern Config* gameConfig;

StarFallVis::StarFallVis() : Visualiser()
{
}


StarFallVis::~StarFallVis()
{
}


void StarFallVis::init(sf::RenderWindow * wind, sf::RenderTexture * rTex)
{
	m_window = wind;
	m_RT = rTex;

	m_timer.restart();
	m_spot = sf::CircleShape(m_spotRadius);


	m_spot.setOrigin({ m_spotRadius, m_spotRadius });
	m_spot.setFillColor({ 255, 255, 255, 255 });
	if (!m_textures.count("spot"))
	{
		m_textures["spot"] = std::make_shared<sf::Texture>();
		m_textures["spot"]->loadFromFile("spot.png");
		m_textures["spot"]->setSmooth(true);
		m_textures["spot"]->setRepeated(true);
	}
	m_spot.setTexture(m_textures["spot"].get());

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

	rgb = { 0, 0, 0 };
}

void StarFallVis::render(float frameHi, float frameAverage, float frameMax, sf::Texture* bgImage)
{
	m_window->clear({ 0,0,0,0 });

	float scale = frameHi / frameMax;

	auto dt = m_timer.restart();
	elapsed += dt;

	if (frameHi > frameAverage && scale>(0.1f))
	{
		int dist = m_scrW / 10;
		dist *= (frameHi / frameAverage);

		int numSpots = ceil(frameHi / frameAverage);
		if (numSpots > 10)
			numSpots = 10;
		while (numSpots--)
		{
			auto pos = spotPos + sf::Vector2f((rand() % dist), (rand() % dist));
			if (pos.x > m_scrW - 10 * scale)
				pos.x = m_scrW - 10 * scale;
			if (pos.y > m_scrH - 10 * scale)
				pos.y = m_scrH - 10 * scale;

			auto newSpot = m_spot;
			float r = (m_spotRadius / 10.f)*scale;
			newSpot.setRadius(r);
			newSpot.setOrigin({ r, r });
			newSpot.setPosition(sf::Vector2f(pos));
			if (spots.size() < 10)
				spots.push_back(newSpot);
			else
				break;
		}
	}

	if (frameHi > 1.8f*frameAverage)
	{
		getRandomColour(rgb, gameConfig->gradient, &gameConfig->gradCol1, &gameConfig->gradCol2);

		m_shader.setUniform("inColour", sf::Glsl::Vec4(rgb[0], rgb[1], rgb[2], 1.f));
	}

	m_shader.setUniform("time", elapsed.asSeconds());

	float r = m_spotRadius*scale;
	m_spot.setRadius(r);
	m_spot.setOrigin({ r, r });
	m_spot.setPosition(spotPos);
	float rot = m_spot.getRotation() + 0.01f*((rand() % 500) - 250);
	m_spot.setRotation(rot);

	

	float baseMoveR = (m_scrW/13) * dt.asSeconds();
	float baseMoveT = (m_scrH / 13) * dt.asSeconds();
	
	float mult = frameMax / frameAverage;
	if (mult > m_scrW / 20)
		mult = m_scrW / 20;
	if (mult < 1)
		mult = 1;

	float baseColour = 500.f - (500.f * (mult / (m_scrW / 20)));
	if (baseColour > 250)
		baseColour = 250;

	float rightF = baseMoveR*mult+catchupR;
	int right = floor(rightF);
	catchupR = rightF - right;

	float topF = baseMoveT*mult+ catchupT;
	int top = floor(topF);
	catchupT = topF - top;
	tRect.top += top;
	tRect.left += right;
	m_RTPlane.setTextureRect(sf::IntRect(tRect));
	//m_RTPlane.move({ -rightF, -topF });

	float v1x = baseMoveT*mult;
	float v1y = baseMoveR*mult;
	float mag = sqrt(v1x*v1x + v1y*v1y);
	//v1y = fmodf(v1y, PI);
	float angle = acos(v1y / mag);
	m_waveform.rotation(angle * 180 / PI + 90);

	skyRect.top += (baseMoveT* mult) / 5;
	skyRect.left += (baseMoveR*mult) / 5;

	float fade = ((2 * frameAverage) / frameMax)*m_scrW;
	m_shader.setUniform("fadeDist", fade);
	float opacity = 0.01f;
	float newOp = 0.02 - (scale / 20);
	if (newOp > opacity)
		opacity = newOp;
	m_shader.setUniform("opacity", opacity);

	

	m_RTPlane.setFillColor({ (sf::Uint8)(baseColour + rgb[0] * 5), (sf::Uint8)(baseColour + rgb[1] * 5), (sf::Uint8)(baseColour + rgb[2] * 5), 200 });
	m_RT->draw(m_RTPlane);
	m_RT->display();
	tRect.top -= top;
	tRect.left -= right;
	m_RTPlane.setTextureRect(sf::IntRect(tRect));
	//m_RTPlane.move({ rightF, topF });
	m_RTPlane.setFillColor({ 255, 255, 255, 255 });

	m_skyPlane.setTextureRect(sf::IntRect(skyRect));
	m_skyPlane.setFillColor({ (sf::Uint8)(rgb[0] * 255), (sf::Uint8)(rgb[1] * 200), (sf::Uint8)(rgb[2] * 200), 2 });

	m_RT->draw(m_skyPlane);
	m_RT->draw(m_shaderPlane, states);

	m_RT->draw(m_rightBar);
	m_RT->draw(m_bottomBar);

	//m_waveform.update();
	//m_RT->draw(m_waveform);

	//m_RT->display();


	m_RT->draw(m_spot, addStates);
	int si = 0;
	for (int si = 0; si < spots.size(); si++)
	{
		m_RT->draw(spots[si]);
	}

	spots.clear();
	m_RT->display();

	if (bgImage && !gameConfig->transparent)
	{
		//m_window->draw(m_RTPlane);
	}

	m_transparentShader.setUniform("minOpacity", gameConfig->minOpacity);
	if (gameConfig->transparent || bgImage)
		m_window->draw(m_RTPlane, m_transparentStates);
	else
		m_window->draw(m_RTPlane);
	
	m_transparentShader.setUniform("minOpacity", 0);
	m_skyPlane.setFillColor({ (sf::Uint8)(rgb[0] * 255), (sf::Uint8)(rgb[1] * 200), (sf::Uint8)(rgb[2] * 200), sf::Uint8(100 * scale) });
	m_window->draw(m_skyPlane, addStates);

	m_waveform.update();
	m_window->draw(m_waveform);
}

void StarFallVis::resetPositions(float scrW, float scrH, float ratio)
{
	m_scrW = scrW;
	m_scrH = scrH;
	m_ratio = ratio;

	float dist = 3.5;

	spotPos = { scrW - scrW / (ratio * dist), scrH - scrH / dist };
	m_spotRadius = scrW / 50;
	m_spot.setPosition(spotPos);
	m_spot.setTexture(m_textures["spot"].get());

	//m_waveform.init(m_scrW, -m_scrH / 6);
	m_waveform.init(m_scrH/3, true);
	m_waveform.position( spotPos );
	m_waveform.setColour(sf::Color(255,255,50,40));
	m_waveform.blendMode(sf::BlendAdd);

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

	m_rightBar = sf::RectangleShape({ 1, scrH });
	m_rightBar.setPosition({ scrW - 1, 0 });
	m_rightBar.setFillColor({ 0,0,0,255 });
	m_bottomBar = sf::RectangleShape({ scrW, 1 });
	m_bottomBar.setPosition(0, scrH - 1);
	m_bottomBar.setFillColor({ 0,0,0,255 });
}

void StarFallVis::reloadShader()
{
	if (!m_shader.loadFromFile("Shaders/wave.frag", sf::Shader::Type::Fragment))
		std::cout << "Shader borken";

	sf::Shader::bind(&m_shader);

	m_shader.setUniform("amplitude", 1.f);
	m_shader.setUniform("waveWidth", m_scrW / 70);
	m_shader.setUniform("speed", 3.f);
	m_shader.setUniform("opacity", 0.01f);
	m_shader.setUniform("scrw", m_scrW);
	m_shader.setUniform("scrh", m_scrH);

	getRandomColour(rgb, gameConfig->gradient, &gameConfig->gradCol1, &gameConfig->gradCol2);

	m_shader.setUniform("inColour", sf::Glsl::Vec4(rgb[0], rgb[1], rgb[2], 1.f));

	m_shader.setUniform("point", sf::Glsl::Vec2(spotPos.x, m_scrH - spotPos.y));

	float fade = 0.3f*m_scrW;
	m_shader.setUniform("fadeDist", fade);

	if (!m_transparentShader.loadFromFile("Shaders/BlackToAlpha.frag", sf::Shader::Type::Fragment))
		std::cout << "Shader borken";

	m_transparentStates.shader = &m_transparentShader;
	addStates.shader = &m_transparentShader;
}
