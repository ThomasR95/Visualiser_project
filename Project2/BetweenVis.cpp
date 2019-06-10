#include "BetweenVis.h"
#include <iostream>

#include "Config.h"
extern Config* gameConfig;

BetweenVis::BetweenVis() : Visualiser()
{
}


BetweenVis::~BetweenVis()
{
}


void BetweenVis::init(sf::RenderWindow * wind, sf::RenderTexture * rTex)
{
	m_window = wind;
	m_RT = rTex;


	

	m_timer.restart();

	if (!m_textures.count("space"))
	{
		sf::Texture tex2;
		tex2.loadFromFile("space.jpg");
		tex2.setSmooth(true);
		tex2.setRepeated(true);
		m_textures["space"] = tex2;
		m_skyPlane.setTexture(&m_textures["space"], false);
	}

	if (!m_textures.count("spot"))
	{
		sf::Texture tex2;
		tex2.loadFromFile("spot.png");
		tex2.setSmooth(true);
		tex2.setRepeated(true);
		m_textures["spot"] = tex2;
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
}

void BetweenVis::render(float frameHi, float frameAverage, float frameMax)
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

	float mult = frameMax / frameAverage;
	if (mult > m_scrW / 20)
		mult = m_scrW / 20;
	if (mult < 1)
		mult = 1;

	float radius = (frameHi / frameMax)*(m_scrW / 9);
	radius += m_scrW / 15;


	if (frameHi > 2.f*frameAverage)
	{
		getRandomColour(rgb, gameConfig->gradient, &gameConfig->gradCol1, &gameConfig->gradCol2);

		m_shader.setUniform("inColour", sf::Glsl::Vec4(rgb[1], rgb[2], rgb[0], 1.f));
	}

	m_shader.setUniform("time", -elapsed.asSeconds());


	float baseColour = 500.f - (500.f * (mult / (m_scrW / 20)));
	if (baseColour > 250)
		baseColour = 250;

	
	//m_RT->draw(m_RTPlane);

	m_RTPlane.setFillColor({ (sf::Uint8)(baseColour + rgb[0] * 5), (sf::Uint8)(baseColour + rgb[1] * 5), (sf::Uint8)(baseColour + rgb[2] * 5), 200 });

	//m_rot = m_degreesPerFrame/2 +  ((m_degreesPerFrame/2) * (frameAverage / frameMax));
	//m_RTPlane.setRotation(m_rot);
	auto view = m_RT->getView();
	auto viewCpy = view;
	float stretch = 1.f - scale*0.5f;
	if (stretch > 1.f) stretch = 1.f;
	viewCpy.setSize(view.getSize().x*stretch, view.getSize().y);
	m_RT->setView(viewCpy);

	float fade = ((2 * frameAverage) / frameMax)*(m_scrW*2);
	m_shader.setUniform("fadeDist", fade);
	float opacity = 0.01f;
	float newOp = 0.04 - (scale / 10);
	if (newOp > opacity)
		opacity = newOp;
	m_shader.setUniform("opacity", opacity);
	m_shader.setUniform("point", sf::Glsl::Vec2(m_scrW / 2 + m_offset, m_scrH / 2));

	m_RT->draw(m_RTPlane);

	m_RTPlane.setFillColor({ 255, 255, 255, 255 });
	m_RT->setView(view);
	//m_RTPlane.setRotation(0);

	
	m_skyPlane.setFillColor({ (sf::Uint8)(rgb[0] * 255), (sf::Uint8)(rgb[1] * 200), (sf::Uint8)(rgb[2] * 200), 4 });

	sf::Color col((sf::Uint8)(rgb[0] * 255*scale), (sf::Uint8)(rgb[1] * 255*scale), (sf::Uint8)(rgb[2] * 255*scale), sf::Uint8(150 * scale));
	auto col2 = col;
	col2.a -= 60;

	m_waveform.update();

	m_waveform.setColour(col);
	m_waveform.scale({ 1, -1.6f });
	m_RT->draw(m_waveform);

	m_waveform.scale({ 1, 1.6f });
	m_RT->draw(m_waveform);

	m_waveform.setColour(sf::Color::Black);
	m_waveform.scale({ 1, -1 });
	m_RT->draw(m_waveform);

	m_waveform.scale({ 1, 1 });
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
	m_skyPlane.setFillColor({ (sf::Uint8)(rgb[0] * 255), (sf::Uint8)(rgb[1] * 200), (sf::Uint8)(rgb[2] * 200), sf::Uint8(150 * scale) });

	m_window->draw(m_skyPlane, addStates);

}

void BetweenVis::resetPositions(float scrW, float scrH, float ratio)
{
	m_scrW = scrW;
	m_scrH = scrH;
	m_ratio = ratio;

	m_RT2.create(scrW, scrH);
	m_RT2.clear({ 0,0,0,255 });
	m_RT2.setSmooth(true);

	m_waveform.init(m_scrH, m_scrH / 2);
	m_waveform.position({ m_scrW / 2, m_scrH });
	m_waveform.rotation(-90);
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

	
}

void BetweenVis::reloadShader()
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

	getRandomColour(rgb, gameConfig->gradient, &gameConfig->gradCol1, &gameConfig->gradCol2);

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
