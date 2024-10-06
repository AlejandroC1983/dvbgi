/*
Copyright 2014 Alejandro Cosin

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/component/dynamiccomponent.h"

// NAMESPACE

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

DynamicComponent::DynamicComponent(string&& name) : BaseComponent(move(name), move(string("DynamicComponent")), GenericResourceType::GRT_DYNAMICCOMPONENT)
	, m_duration(0.0f)
	, m_ticksPerSecond(0.0f)
	, m_durationTime(0.0f)
	, m_elapsedTime(0.0f)
	, m_playSpeedRatio(1.0f)
	, m_loop(true)
	, m_play(true)
	, m_keyframeInformationSet(false)
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

DynamicComponent::~DynamicComponent()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

void DynamicComponent::init()
{
	if (m_keyframeInformationSet && (m_duration == 0.0f))
	{
		cout << "ERROR: DYNAMIC COMPONENT " << getName() << " HAS NO DURATION DATA" << endl;
		return;
	}

	if (m_keyframeInformationSet && (m_ticksPerSecond == 0.0f))
	{
		cout << "ERROR: DYNAMIC COMPONENT " << getName() << " HAS NO TICKS PER SECOND DATA" << endl;
		return;
	}

	if (m_keyframeInformationSet)
	{
		m_durationTime = m_duration / m_ticksPerSecond;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void DynamicComponent::setKeyInformation(vectorVec3&& vectorPositionKey, vectorQuat&& vectorRotationKey, vectorVec3&& vectorScalingKey, vectorFloat&& vectorKey)
{
	m_vectorPositionKey      = vectorPositionKey;
	m_vectorRotationKey      = vectorRotationKey;
	m_vectorScalingKey       = vectorScalingKey;
	m_vectorKey              = vectorKey;
	m_keyframeInformationSet = true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool DynamicComponent::update(float delta, Transform& transform)
{
	if (!m_play)
	{
		return false;
	}

	m_playSpeedRatio = 0.25f;

	m_elapsedTime += (delta * m_playSpeedRatio) / 1000.0f;

	if (m_elapsedTime > m_durationTime)
	{
		if (!m_loop)
		{
			m_play = false;
			return false;
		}

		m_elapsedTime = fmod(m_elapsedTime, m_durationTime);
	}

	float currentTick         = (m_elapsedTime / m_durationTime) * m_duration;
	int previousTick          = floor(currentTick);
	int nextTick              = ceil(currentTick);
	float interpolation       = currentTick - previousTick;

	vec3 interpolatedPosition = m_vectorPositionKey[previousTick] * (1.0f - interpolation) + m_vectorPositionKey[nextTick] * (interpolation);
	vec3 interpolatedScale    = m_vectorScalingKey[previousTick]  * (1.0f - interpolation) + m_vectorScalingKey[nextTick]  * (interpolation);
	quat interpolatedRotation = m_vectorRotationKey[previousTick] * (1.0f - interpolation) + m_vectorRotationKey[nextTick] * (interpolation);

	bool anyChanges = false;

	if (transform.getTraslation() != interpolatedPosition)
	{
		transform.setTraslation(interpolatedPosition);
		anyChanges = true;
	}

	if (transform.getScaling() != interpolatedScale)
	{
		transform.setScaling(interpolatedScale);
		anyChanges = true;
	}

	if (transform.getRotation() != interpolatedRotation)
	{
		transform.setRotation(interpolatedRotation);
		anyChanges = true;
	}

	return anyChanges;
}

/////////////////////////////////////////////////////////////////////////////////////////////
