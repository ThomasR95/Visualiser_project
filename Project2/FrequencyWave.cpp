#include "FrequencyWave.h"

#include "Config.h"
extern Config* gameConfig;

FrequencyWave::FrequencyWave()
{
	m_scale = { 1.f,1.f };
	m_rotation = 0;
	m_position = { 0.f,0.f };
	m_blendMode = sf::BlendAlpha;
	m_shape = FLAT;
}


FrequencyWave::~FrequencyWave()
{
}

void FrequencyWave::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
	auto myStates = sf::RenderStates::Default;
	myStates.transform.translate(m_position);
	myStates.transform.rotate(m_rotation);
	myStates.transform.scale(m_scale);
	myStates.blendMode = m_blendMode;
	
	
	target.draw(m_wavePoints, myStates);
}

void FrequencyWave::update()
{
	std::lock_guard<std::mutex> guard(gameConfig->FreqDataMutex);

	if (gameConfig->FrequencyData.size() == 0)
		return;

	int numPoints = m_wavePoints.getVertexCount();

	int pointWidth = numPoints / 2;
	float segAngle = 2*PI / pointWidth;
	int freqIdx = 0;

	for (int v = 0; v < numPoints; v++)
	{
		sf::Vertex& vtx = m_wavePoints[v];
		switch (m_shape)
		{
		case FrequencyWave::FLAT:
			if (v % 2 == 0)
			{
				vtx.position.y = m_maxHeight * (gameConfig->FrequencyData[freqIdx] / gameConfig->frameMax);
				freqIdx++;
			}
			break;
		case FrequencyWave::CIRCLE:
		case FrequencyWave::CIRCLE2:
		{
			if (v % 2 == 0)
			{
				float angle = segAngle * floor(v / 2);
				float thisRadius = m_radius * (gameConfig->FrequencyData[freqIdx] / gameConfig->frameMax);
				vtx.position.x = sin(angle)*thisRadius;
				vtx.position.y = cos(angle)*thisRadius;

				if (m_shape == FrequencyWave::CIRCLE2)
				{
					sf::Vertex& vtx2 = m_wavePoints[(numPoints - 1) - v];
					float angle2 = PI * 2 - angle;
					vtx2.position.x = sin(angle2)*thisRadius;
					vtx2.position.y = cos(angle2)*thisRadius;
				}

				freqIdx++;
			}
			break;
		}
		default:
			break;
		}
		
	}
}

void FrequencyWave::init(float width, float height)
{
	m_shape = FLAT;
	m_width = width;
	m_maxHeight = height;

	int numPoints = FRAMES_PER_BUFFER +1;

	m_wavePoints.resize(numPoints);
	m_wavePoints.setPrimitiveType(sf::PrimitiveType::TriangleStrip);

	int pointWidth = numPoints / 2;
	float segWidth = m_width / pointWidth;

	for (int v = 0; v < numPoints; v++)
	{
		sf::Vertex& vtx = m_wavePoints[v];
		vtx.position.x = segWidth * floor(v / 2);
		vtx.position.y = (m_maxHeight / 255.0) * (1- v % 2);
	}
}

void FrequencyWave::init(float radius, bool symmetry)
{
	m_shape = CIRCLE;
	if (symmetry) m_shape = CIRCLE2;
	m_radius = radius;

	int numPoints = FRAMES_PER_BUFFER;
	if (symmetry) numPoints *= 2;

	m_wavePoints.resize(numPoints);
	m_wavePoints.setPrimitiveType(sf::PrimitiveType::TriangleStrip);

	int pointWidth = numPoints / 2;
	float segAngle = 2 * PI / pointWidth;

	int forPoints = numPoints;
	if (symmetry) forPoints /= 2;

	for (int v = 0; v < forPoints; v++)
	{
		sf::Vertex& vtx = m_wavePoints[v];
		float angle = segAngle * floor(v / 2);
		float thisRadius = (m_radius / 255.0) * (1 - v % 2);
		vtx.position.x = sin(angle)*thisRadius;
		vtx.position.y = cos(angle)*thisRadius;

		if (symmetry)
		{
			sf::Vertex& vtx2 = m_wavePoints[(numPoints-1)-v];
			float angle2 = PI*2 - angle;
			vtx2.position.x = sin(angle2)*thisRadius;
			vtx2.position.y = cos(angle2)*thisRadius;
		}
	}
}

void FrequencyWave::setColour(const sf::Color & col)
{
	for (int v = 0; v < m_wavePoints.getVertexCount(); v++)
	{
		auto& vtx = m_wavePoints[v];
		vtx.color = col;
	}
}
