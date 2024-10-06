/*
Copyright 2017 Alejandro Cosin

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

#ifndef _DYNAMICCOMPONENT_H_
#define _DYNAMICCOMPONENT_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/math/transform.h"
#include "../../include/component/basecomponent.h"

// CLASS FORWARDING

// NAMESPACE

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

class DynamicComponent : public BaseComponent
{
	friend class ComponentManager;

protected:
	/** Paremeter constructor
	* @param name [in] component name
	* @return nothing */
	DynamicComponent(string&& name);

public:
	/** Default destructor
	* @return nothing */
	virtual ~DynamicComponent();

	/** Initialize the resource
	* @return nothing */
	virtual void init();

	/** Assign the memebr variables with all the key-related information to animate the node that owns this component
	* @param vectorPositionKey [in] Vector with the 3D key position information (note: all 
	* @param vectorRotationKey [in] Vector with the 3D key rotation information
	* @param vectorScalingKey  [in] Vector with the 3D key scaling information
	* @param vectorKey         [in] Vector with the key values at each index, applies to the same index of all other variables with key-related information (m_vectorPositionKey, m_vectorRotationKey, m_vectorScalingKey)
	* @return nothing */
	void setKeyInformation(vectorVec3&& vectorPositionKey, vectorQuat&& vectorRotationKey, vectorVec3&& vectorScalingKey, vectorFloat&& vectorKey);

	/** Update the status of the current animation
	* @param delta [in]    elapsed time since the last frame, in ms
	* @param delta [inout] transform of the node owning this component, it will be modified if there are any changes
	* @return true if transform was modified, false, otherwise */
	bool update(float delta, Transform& transform);

	SET(float, m_duration, Duration)
	SET(float, m_ticksPerSecond, TicksPerSecond)
	GETCOPY(float, m_durationTime, DurationTime)
	GETCOPY(bool, m_keyframeInformationSet, KeyframeInformationSet)
	SET(bool, m_play, Play)
	REF(vectorVec3, m_vectorPositionKey, VectorPositionKey)

protected:
	float         m_duration;               //!< Animation duration in keyframes
	float         m_ticksPerSecond;         //!< Number of keyframes this animation has per second
	float         m_durationTime;           //!< Animation duration in seconds
	float         m_elapsedTime;            //!< Elapsed thime this animation has been playing in seconds
	float         m_playSpeedRatio;         //!< Ratio (value in interval [0, 1] for how fast to update the transform of the elemet in DynamicComponent::update
	bool          m_loop;                   //!< Whether to replay the animation in loop of just one time
	bool          m_play;                   //!< Flag to update ot not the animation
	bool          m_keyframeInformationSet; //!< Flag to know when the key frame information has been set through a call to setKeyInformation
	vectorVec3    m_vectorPositionKey;      //!< Vector with the 3D key position information (note: all 
	vectorQuat    m_vectorRotationKey;      //!< Vector with the 3D key rotation information
	vectorVec3    m_vectorScalingKey;       //!< Vector with the 3D key scaling information
	vectorFloat   m_vectorKey;              //!< Vector with the key values at each index, applies to the same index of all other variables with key-related information (m_vectorPositionKey, m_vectorRotationKey, m_vectorScalingKey)
};		   							  

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _DYNAMICCOMPONENT_H_
