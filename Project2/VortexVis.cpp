#include "VortexVis.h"
#include <iostream>

#include "Config.h"
extern Config* gameConfig;

VortexVis::VortexVis() : Visualiser()
{
}


VortexVis::~VortexVis()
{
}


void VortexVis::init(sf::RenderWindow * wind, sf::RenderTexture * rTex)
{
	m_window = wind;
	m_RT = rTex;


	

	m_timer.restart();

	if (!m_textures.count("space"))
	{
		m_textures["space"] = std::make_shared<sf::Texture>();
		m_textures["space"]->loadFromFile("space.jpg");
		m_textures["space"]->setSmooth(true);
		m_textures["space"]->setRepeated(true);
	}
	m_skyPlane.setTexture(m_textures["space"].get(), false);

	if (!m_textures.count("spot"))
	{
		m_textures["spot"] = std::make_shared<sf::Texture>();
		m_textures["spot"]->loadFromFile("spot.png");
		m_textures["spot"]->setSmooth(true);
		m_textures["spot"]->setRepeated(true);
		m_topBox.setTexture(m_textures["spot"].get(), false);
		m_bottomBox.setTexture(m_textures["spot"].get(), false);
	}

	if (!m_textures.count("ring"))
	{
		m_textures["ring"] = std::make_shared<sf::Texture>();
		m_textures["ring"]->loadFromFile("ring.png");
		m_textures["ring"]->setSmooth(true);
		m_textures["ring"]->setRepeated(true);

		m_ringBox.setTexture(m_textures["ring"].get());
		m_ringBox.setSize(sf::Vector2f(m_textures["ring"]->getSize() ));
		auto size = m_ringBox.getSize();
		m_ringBox.setOrigin(size * 0.5f);
	}
	
	
	m_ringBox.setFillColor({ 255,255,255,0 });

	states.shader = &m_shader;
	states.blendMode = sf::BlendAdd;
	addStates.blendMode = sf::BlendAdd;
	contrastStates.shader = &m_shader2;

	elapsed = sf::seconds(0);
	m_timer.restart();

	m_degrees = 0;
	m_degreesPerSec = 5.f;
	m_degreesPerFrame = 1.8f;

	rgb = { 0, 0, 0 };

	m_topBox.setFillColor({ 255,255,255,255 });
	m_bottomBox.setFillColor({ 255,255,255,255 });
	
	auto tRect = m_topBox.getTextureRect();
	tRect.height /= 5;
	tRect.top += tRect.height*2;
	m_topBox.setTextureRect(tRect);
	m_bottomBox.setTextureRect(tRect);
	m_topBar.setFillColor({ 0,0,0,50 });
	m_bottomBar.setFillColor({ 0,0,0,50 });

	leftOverflow = 0;
	m_rot = 0;
}

void VortexVis::render(float frameHi, float frameAverage, float frameMax, sf::Texture* bgImage)
{
	m_window->clear({0,0,0,0});
	float scale = frameHi / frameMax;

	m_frameAverage -= m_frameAverage / 10;
	m_frameAverage += frameHi/10;

	auto dt = m_timer.restart();
	elapsed += dt;

	m_degrees += dt.asSeconds()*m_degreesPerSec;
	if (m_degrees > 360)
		m_degrees -= 360;

	float mult = frameMax / frameAverage;
	if (mult > m_scrW / 20)
		mult = m_scrW / 20;
	if (mult < 1)
		mult = 1;

	float radius = (frameHi / frameMax)*(m_scrW / 9);
	radius += m_scrW / 15;


	if (gameConfig->trebleHi > 1.5f*gameConfig->trebleAverage && scale>(0.1f) && !ringFadeUp && !ringFadeDown)
	{
		ringFadeUp = true;
		ringFadeDown = false;
		ringFadeFactor = 0.001f;
		ringFadeMult = 0.f;
		float ringRot = (rand() % 360);
		m_ringBox.setRotation(ringRot);
	}

	if (frameHi > 2.f*frameAverage)
	{
		getRandomColour(rgb, gameConfig->gradient, &gameConfig->gradCol1, &gameConfig->gradCol2);

		m_shader.setUniform("inColour", sf::Glsl::Vec4(rgb[0], rgb[1], rgb[2], 1.f));
	}

	m_shader.setUniform("time", elapsed.asSeconds());

	if (ringFadeUp)
	{
		if (ringFadeMult < 1.f)
		{
			ringFadeFactor += 0.001f;
			ringFadeMult += ringFadeFactor;
			float rscale = 1.f - ringFadeMult;
			
			m_ringBox.setScale({ ringScaleMult, ringScaleMult*rscale });

			sf::Uint8 alpha(255.f * ringFadeMult);
			m_ringBox.setFillColor({ 255,255,255,alpha });
		}
		else
		{
			ringFadeMult = 1.f;
			ringFadeUp = false;
			ringFadeDown = true;
		}
	}
	else if (ringFadeDown)
	{
		if (ringFadeMult > 0.f)
		{
			ringFadeFactor -= 0.001f;
			if (ringFadeFactor < 0.001)
				ringFadeFactor = 0.001;
			ringFadeMult -= ringFadeFactor;

			float rscale = -1.f + ringFadeMult;
			m_ringBox.setScale({ ringScaleMult, ringScaleMult*rscale });

			sf::Uint8 alpha(255.f * ringFadeMult);
			m_ringBox.setFillColor({ 255,255,255,alpha });
		}
		else
		{
			ringFadeMult = 0.f;
			ringFadeDown = false;
		}
	}
	
	float baseMove = 50 * dt.asSeconds();
	float dist = (m_scrW/2.0)*(m_frameAverage / frameMax) + m_scrW/2.0;
	auto lastTopPos = m_topBox.getPosition();
	float width = dist - lastTopPos.x;
	if (width < m_boxWidth)
		width = m_boxWidth;
	m_topBox.setPosition({ dist, 0 });
	m_bottomBox.setPosition({ m_scrW - dist, m_scrH });
	m_bottomBox.setSize({ width, m_boxHeight });
	m_topBox.setSize({ width, m_boxHeight });

	float baseColour = 500.f - (500.f * (mult / (m_scrW / 20)));
	if (baseColour > 250)
		baseColour = 250;

	float holeScale = (gameConfig->bassAverage / gameConfig->bassMax) + 0.4;
	m_Hole.setScale({ holeScale ,holeScale });
	m_Rim.setScale({ holeScale ,holeScale });

	
	//m_RT->draw(m_RTPlane);

	m_RT->draw(m_topBar);
	m_RT->draw(m_bottomBar);
	
	m_RTPlane.setFillColor({ (sf::Uint8)(baseColour + rgb[0] * 5), (sf::Uint8)(baseColour + rgb[1] * 5), (sf::Uint8)(baseColour + rgb[2] * 5), 200 });

	m_rot = m_degreesPerFrame/2 +  ((m_degreesPerFrame/2) * (frameAverage / frameMax));
	m_RTPlane.setRotation(m_rot);
	auto view = m_RT->getView();
	auto viewCpy = view;
	viewCpy.zoom(1.01);
	m_RT->setView(viewCpy);

	float fade = ((2 * frameAverage) / frameMax)*m_scrW;
	m_shader.setUniform("fadeDist", fade);
	float opacity = 0.01f;
	float newOp = 0.02 - (scale / 20);
	if (newOp > opacity)
		opacity = newOp;
	m_shader.setUniform("opacity", opacity);

	m_RT->draw(m_RTPlane);

	m_RTPlane.setFillColor({ 255, 255, 255, 255 });
	m_RT->setView(view);
	m_RTPlane.setRotation(0);

	
	m_skyPlane.setFillColor({ (sf::Uint8)(rgb[0] * 255), (sf::Uint8)(rgb[1] * 200), (sf::Uint8)(rgb[2] * 200), 4 });

	
	m_RT->draw(m_topBox);
	m_RT->draw(m_bottomBox);

	m_RT->draw(m_skyPlane);
	m_RT->draw(m_shaderPlane, states);

	m_RT->draw(m_Rim);
	m_RT->draw(m_Hole);

	

	m_RT->display();

	m_transparentShader.setUniform("minOpacity", gameConfig->minOpacity);
	if (gameConfig->transparent)
		m_window->draw(m_RTPlane, m_transparentStates);
	else
		m_window->draw(m_RTPlane);

	
	
	/*
	float high = (frameHi / frameMax) - 0.5f;
	if (high < 0)
		high = 0;
	m_skyPlane.setFillColor({ sf::Uint8(255u * high), sf::Uint8(255u * high), sf::Uint8(255u * high), 4 });
	m_RTPlane2.setScale({ 0.995f,0.995f });
	m_RTPlane2.setFillColor({ 250,250,250,255 });
	m_RT2.draw(m_RTPlane2);
	m_RT2.draw(m_skyPlane, contrastStates);
	m_RT2.display();
	m_RTPlane2.setScale({ 1,1 });
	m_RTPlane2.setFillColor({ 255,255,255,255 });
	m_window->draw(m_RTPlane2, addStates);
	*/

	m_transparentShader.setUniform("minOpacity", 0.0f);

	m_skyPlane.setRotation(m_degrees);
	m_skyPlane.setFillColor({ (sf::Uint8)(rgb[0] * 255), (sf::Uint8)(rgb[1] * 200), (sf::Uint8)(rgb[2] * 200), sf::Uint8(150 * scale) });
	if (ringFadeUp || ringFadeDown)
	{
		m_window->draw(m_ringBox, addStates);
	}
	m_window->draw(m_skyPlane, addStates);

}

void VortexVis::resetPositions(float scrW, float scrH, float ratio)
{
	m_scrW = scrW;
	m_scrH = scrH;
	m_ratio = ratio;

	m_RT2.create(scrW, scrH);
	m_RT2.clear({ 0,0,0,255 });
	m_RT2.setSmooth(true);

	float dist = 3.5;
	float width = m_scrH / 50;
	float height = m_scrW / 80;
	m_boxWidth = width;
	m_boxHeight = height;
	m_topBox.setSize({ width, height });
	m_topBox.setOrigin({ width, 0 });
	m_bottomBox.setSize({ width, height });
	m_bottomBox.setOrigin({ 0, height });
	m_topBox.setPosition({ m_scrW / 2, 0 });
	m_bottomBox.setPosition({ m_scrW / 2, m_scrH  });

	m_topBar.setSize({ m_scrW, height });
	m_bottomBar.setSize({ m_scrW, height });
	m_topBar.setPosition(0, 0);
	m_bottomBar.setPosition(0, m_scrH- height);


	ringScaleMult = (m_scrW/7) / m_ringBox.getSize().x;
	m_ringBox.setPosition({ m_scrW / 2, m_scrH / 2 });
	m_ringBox.setScale({ ringScaleMult,ringScaleMult });
	ringFadeDown = false;
	ringFadeUp = false;

	m_RTPlane.setSize({ scrW, scrH });

	m_RTPlane.setTexture(&m_RT->getTexture());
	tRect = sf::FloatRect(m_RTPlane.getTextureRect());
	tRect.left = 0;
	tRect.top = 0;
	tRect.width = scrW;
	tRect.height = scrH;
	m_RTPlane.setTextureRect(sf::IntRect(tRect));
	m_RTPlane.setSize({ scrW, scrH });
	m_RTPlane.setOrigin(scrW / 2, scrH / 2);
	m_RTPlane.setPosition({ scrW / 2, scrH / 2 });
	m_RTPlane.setScale({ 1,1 });

	m_RTPlane2 = m_RTPlane;
	m_RTPlane2.setTexture(&m_RT2.getTexture());

	m_shaderPlane.setSize(m_RTPlane.getSize());
	m_skyPlane.setSize({ scrW*1.5f, scrW*1.5f });
	m_skyPlane.setOrigin(scrW*0.75f, scrW*0.75f);
	m_skyPlane.setPosition(m_scrW/2, m_scrH/2);

	float radius = m_scrW / 20;
	m_Hole.setRadius(radius);
	m_Rim.setRadius(radius + 1);
	m_Hole.setFillColor({ 0,0,0,255 });
	m_Rim.setFillColor({ 255,255,255,200 });
	m_Rim.setOutlineColor({ 255,255,255,50 });
	m_Rim.setOutlineThickness(1);

	m_Hole.setOrigin(radius, radius);
	m_Hole.setPosition(m_scrW / 2, m_scrH / 2);
	m_Rim.setOrigin(radius+1, radius+1);
	m_Rim.setPosition(m_scrW / 2, m_scrH / 2);

	skyRect = sf::FloatRect(0, 0, scrW, scrW);
	m_skyPlane.setTextureRect(sf::IntRect(skyRect));

	m_frameAverage = gameConfig->frameHi;
}

void VortexVis::reloadShader()
{
	if (!m_shader.loadFromFile("Shaders/wave_diamond.frag", sf::Shader::Type::Fragment))
		std::cout << "Shader borken";

	sf::Shader::bind(&m_shader);

	m_shader.setUniform("amplitude", 1.f);
	m_shader.setUniform("waveWidth", m_scrW / 70);
	m_shader.setUniform("speed", float(3.f));
	m_shader.setUniform("opacity", 0.01f);
	m_shader.setUniform("scrw", m_scrW);
	m_shader.setUniform("scrh", m_scrH);

	getRandomColour(rgb, gameConfig->gradient, &gameConfig->gradCol1, &gameConfig->gradCol2);

	m_shader.setUniform("inColour", sf::Glsl::Vec4(rgb[0], rgb[1], rgb[2], 1.f));

	m_shader.setUniform("point", sf::Glsl::Vec2(m_scrW/2, m_scrH/2));

	float fade = 0.3f*m_scrW;
	m_shader.setUniform("fadeDist", fade);

	if (!m_shader2.loadFromFile("Shaders/bright+contrast.frag", sf::Shader::Type::Fragment))
		std::cout << "Shader borken";

	sf::Shader::bind(&m_shader2);
	m_shader2.setUniform("brightness", 0.99f);
	m_shader2.setUniform("contrast", 3.8f);
	m_shader2.setUniform("texture", sf::Shader::CurrentTexture);

	if (!m_transparentShader.loadFromFile("Shaders/BlackToAlpha.frag", sf::Shader::Type::Fragment))
		std::cout << "Shader borken";

	m_transparentStates.shader = &m_transparentShader;
	addStates.shader = &m_transparentShader;
}
