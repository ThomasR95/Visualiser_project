#pragma once

#include <SFML\Graphics.hpp>


class FrequencyWave : public sf::Drawable
{
public:

	enum Shape
	{
		FLAT,
		CIRCLE,
		CIRCLE2
	};

	FrequencyWave();
	~FrequencyWave();

	virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

	void update();

	void init(float width, float height);
	void init(float radius, bool symmetry = false);

	void setColour(const sf::Color& col);

	void position(const sf::Vector2f& pos) { m_position = pos; }
	sf::Vector2f position() { return m_position; }

	void scale(const sf::Vector2f& pos) { m_scale = pos; }
	sf::Vector2f scale() { return m_scale; }

	void rotation(float rot) { m_rotation = rot; }
	float rotation() { return m_rotation; }

	void blendMode(const sf::BlendMode& bm) { m_blendMode = bm; }
	sf::BlendMode blendMode() { return m_blendMode; }

private:

	sf::VertexArray m_wavePoints;

	float m_width;
	float m_maxHeight;

	float m_radius;

	sf::Vector2f m_position;
	sf::Vector2f m_scale;
	float m_rotation;

	sf::BlendMode m_blendMode;

	Shape m_shape;
};

