#include "SingularityVis.h"
#include <iostream>

SingularityVis::SingularityVis() : Visualiser()
{
}


SingularityVis::~SingularityVis()
{
}


void SingularityVis::init(sf::RenderWindow * wind, sf::RenderTexture * rTex, Config* config)
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

	if (!m_textures.count("spot"))
	{
		m_textures["spot"] = std::make_shared<sf::Texture>();
		m_textures["spot"]->loadFromFile("spot.png");
		m_textures["spot"]->setSmooth(true);
		m_textures["spot"]->setRepeated(true);
	}

	states.shader = &m_shader;
	states.blendMode = sf::BlendAdd;
	addStates.blendMode = sf::BlendAdd;
	contrastStates.shader = &m_shader2;

	elapsed = sf::seconds(0);
	m_timer.restart();

	m_degrees = 0;
	m_degreesPerSec = 5.f;
	m_degreesPerFrame = 0.0f;

	rgb = { 0, 0, 0 };

	leftOverflow = 0;
	m_rot = 0;
	m_offset = 0;
	m_betweenBeatGain = 0;
}

void SingularityVis::render(float frameHi, float frameAverage, float frameMax, sf::Texture* bgImage)
{
	m_window->clear({0,0,0,0});
	float scale = frameHi / frameMax;

	m_frameAverage -= m_frameAverage / 15;
	m_frameAverage += frameHi/15;

	auto dt = m_timer.restart();
	elapsed += dt;

	m_degrees += dt.asSeconds()*m_degreesPerSec;
	if (m_degrees > 360)
		m_degrees -= 360;

	float mult = frameAverage/frameMax;

	float radius = (frameHi / frameMax)*(m_scrW / 9);
	radius += m_scrW / 15;

	m_betweenBeatGain -= 0.01;

	bool colChange = mainColourChange(rgb, frameHi, frameAverage, frameMax);
	if (!colChange && (gameConfig->bassHi > 2.f*gameConfig->bassAverage))
	{
		m_betweenBeatGain = 1.0;
	}

	if (m_betweenBeatGain < 0.0)
		m_betweenBeatGain = 0.0;

	m_shader.setUniform("time", -elapsed.asSeconds());

	float baseColour = 200.f * (mult);

	m_RT->draw(m_RTPlane);

	// Draw previous frame here, slightly darker, so that it fades out over time
	m_RTPlane.setFillColor({ 200, 200, 200, 255 });

	auto view = m_RT->getView();
	auto viewCpy = view;

	m_RT->setView(viewCpy);

	m_RT->draw(m_RTPlane);

	float fade = ((2 * frameAverage) / frameMax)*(m_scrW*2);
	m_shader.setUniform("fadeDist", fade);
	float opacity = 0.01f;
	float newOp = 0.04 - (scale / 10);
	if (newOp > opacity)
		opacity = newOp;
	m_shader.setUniform("opacity", opacity);
	m_shader.setUniform("point", sf::Glsl::Vec2(m_scrW / 2 + m_offset, m_scrH / 2));

	m_RTPlane.setFillColor({ 255, 255, 255, 255 });
	m_RT->setView(view);
	m_RTPlane.setRotation(0);

	m_skyPlane.setFillColor({ (sf::Uint8)(rgb[0] * 200), (sf::Uint8)(rgb[1] * 200), (sf::Uint8)(rgb[2] * 200), 4 });

	// Coloured waveform outline
	auto oldInnerRadius = m_waveform.innerRadius();
	m_waveform.innerRadius(oldInnerRadius*0.5);
	sf::Color col((sf::Uint8)(rgb[0] * 155*scale + 100), (sf::Uint8)(rgb[1] * 155*scale + 100), (sf::Uint8)(rgb[2] * 155*scale + 100), sf::Uint8(150 * scale + 50));
	m_waveform.setColour(col);
	m_waveform.rotation(m_waveform.rotation() + (dt.asSeconds()*m_degreesPerSec));
	m_waveform.update(gameConfig);

	m_RT->draw(m_waveform);

	float bassFactor = 0;
	if(gameConfig->bassHi - gameConfig->bassAverage > 0)
		bassFactor = gameConfig->bassAverage / gameConfig->bassMax;

	// "Black" core waveform
	sf::Color col3((sf::Uint8)(bassFactor * rgb[1]*180 + bassFactor*75), (sf::Uint8)(bassFactor * rgb[2]*180 + bassFactor * 75), (sf::Uint8)(bassFactor * rgb[0]*180 + bassFactor * 75), sf::Uint8(255));
	m_waveform.setColour(col3);
	m_waveform.innerRadius(oldInnerRadius);
	m_waveform.update(gameConfig);

	m_RT->draw(m_waveform);

	m_RT->draw(m_skyPlane, addStates);
	m_RT->draw(m_shaderPlane, states);

	m_RT->display();

	m_transparentShader.setUniform("minOpacity", gameConfig->minOpacity);
	if (gameConfig->transparent)
		m_window->draw(m_RTPlane, m_transparentStates);
	else
		m_window->draw(m_RTPlane);

	m_transparentShader.setUniform("minOpacity", 0.0f);

	m_skyPlane.setRotation(m_degrees);
	m_skyPlane.setFillColor({ (sf::Uint8)(rgb[0] * 250), (sf::Uint8)(rgb[1] * 250), (sf::Uint8)(rgb[2] * 250), sf::Uint8(255 * mult) });

	m_window->draw(m_skyPlane, addStates);

}

void SingularityVis::resetPositions(float scrW, float scrH, float ratio)
{
	m_scrW = scrW;
	m_scrH = scrH;
	m_ratio = ratio;

	m_RT2.create(scrW, scrH);
	m_RT2.clear({ 0,0,0,255 });
	m_RT2.setSmooth(true);

	m_waveform.init(m_scrH / 2.2, true, m_scrH/3.5, true);
	m_waveform.position({ m_scrW / 2, m_scrH / 2 });
	m_waveform.rotation(0);
	m_waveform.setColour(sf::Color::Black);

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

	skyRect = sf::FloatRect(0, 0, scrW, scrW);
	m_skyPlane.setTextureRect(sf::IntRect(skyRect));
	m_skyPlane.setTexture(m_textures["space"].get(), false);

	m_betweenBeatGain = 0;
	
}

void SingularityVis::reloadShader()
{
	if (!m_shader.loadFromFile("Shaders/wave.frag", sf::Shader::Type::Fragment))
		std::cout << "Shader borken";

	sf::Shader::bind(&m_shader);

	m_shader.setUniform("amplitude", 1.f);
	m_shader.setUniform("waveWidth", m_scrW / 30);
	m_shader.setUniform("speed", float(3.f));
	m_shader.setUniform("opacity", 0.1f);
	m_shader.setUniform("scrw", m_scrW);
	m_shader.setUniform("scrh", m_scrH);

	mainColourChange(rgb, gameConfig->frameHi, 0, gameConfig->frameMax);

	m_shader.setUniform("inColour", sf::Glsl::Vec4(1-rgb[0], 1-rgb[1], 1-rgb[2], 1.f));

	m_shader.setUniform("point", sf::Glsl::Vec2(m_scrW/2, m_scrH/2));

	float fade = m_scrW;
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
