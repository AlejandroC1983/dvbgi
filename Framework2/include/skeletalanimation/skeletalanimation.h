/*
Copyright 2022 Alejandro Cosin

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

#ifndef _SKELETALANIMATION_H_
#define _SKELETALANIMATION_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../headers.h"
#include "../../include/util/genericresource.h"
#include "../../include/component/componentenum.h"

// CLASS FORWARDING

// NAMESPACE
using namespace componentenum;

// DEFINES

struct SkeletalAnimationKey
{
	vec3 m_position; //!< Key position
	vec3 m_scale;    //!< Key scale
	quat m_rotation; //!< Key rotation
};

/////////////////////////////////////////////////////////////////////////////////////////////

class SkeletalAnimation : public GenericResource
{
	friend class SkeletalAnimationManager;

protected:
	/** Parameter constructor
	* @param name [in] buffer's name
	* @return nothing */
	SkeletalAnimation(string &&name);

public:
	/** Destructor
	* @return nothing */
	virtual ~SkeletalAnimation();

	/** Utility function to print the position information in m_mapBoneVectorPositionKey and m_vectorTime
	* @return nothing */
	void printKeyPositionData();

	/** Duplicate the key information for the current animation, adding a while set of new keys to m_mapBoneVectorPositionKey and m_vectorTime,
	* and affecting the value of m_duration. The bone given as parameter is the one that will have its position value changed, and it is supposed to affect
	* the whole skeleton of the character, meaning changing its position will affect the whole character
	* @param baseBone     [in] base bone to edit position values when adding new keys to m_mapBoneVectorPositionKey and times to m_vectorTime
	* @param numberCopies [in] number of copies to do for the animation
	* @return nothing */
	void duplicateKeyInformation(string&& baseBone, int numberCopies);

	GETCOPY(float, m_duration, Duration)
	GETCOPY(float, m_timeDifference, TimeDifference)
	GETCOPY(int, m_numKeys, NumKeys)

	map<string, vector<SkeletalAnimationKey>>& refMapBoneVectorPositionKey() { return m_mapBoneVectorPositionKey; }

protected:
	float                                     m_duration;                 //!< Animation duration
	float                                     m_ticksPerSecond;           //!< Animation ticks per second
	int                                       m_numChannels;              //!< Animation number of channels
	int                                       m_numKeys;                  //!< Number of keys (keyframes) the animation has
	map<string, vector<SkeletalAnimationKey>> m_mapBoneVectorPositionKey; //!< Animation position keyframes for each bone
	vector<float>                             m_vectorTime;               //!< Vector with the time for each skeletal animation key in m_mapBoneVectorPositionKey (the times must be shared amongst all keys for all bones)
	vectorMat4                                m_vectorUpdatedBone;        //!< Vector with updated bones
	float                                     m_timeDifference;           //!< Time difference between keys (is a restriction added, all bones need to have the same amount of keys, and the time differences associated to each key has to be the same
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _SKELETALANIMATION_H_
