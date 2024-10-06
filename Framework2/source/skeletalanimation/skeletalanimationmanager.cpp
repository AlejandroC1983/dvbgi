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

// GLOBAL INCLUDES
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// PROJECT INCLUDES
#include "../../include/skeletalanimation/skeletalanimationmanager.h"
#include "../../include/skeletalanimation/skeletalanimation.h"
#include "../../include/parameter/attributedefines.h"
#include "../../include/model/modelenum.h"

// NAMESPACE
using namespace attributedefines;
using namespace modelenum;

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

SkeletalAnimationManager::SkeletalAnimationManager()
{
	m_managerName = g_skeletalAnimationManager;
}

/////////////////////////////////////////////////////////////////////////////////////////////

SkeletalAnimationManager::~SkeletalAnimationManager()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

SkeletalAnimation* SkeletalAnimationManager::buildAnimation(string&& instanceName, aiAnimation* animation)
{
	if (existsElement(move(string(instanceName))))
	{
		return getElement(move(instanceName));
	}

	SkeletalAnimation* skeletalAnimation = new SkeletalAnimation(move(string(instanceName)));

	addElement(move(string(instanceName)), skeletalAnimation);
	skeletalAnimation->m_name           = move(instanceName);
	skeletalAnimation->m_ready          = true;
	skeletalAnimation->m_name           = string(animation->mName.C_Str());
	skeletalAnimation->m_duration       = animation->mDuration;
	skeletalAnimation->m_ticksPerSecond = animation->mTicksPerSecond;
	skeletalAnimation->m_numChannels    = animation->mNumChannels;

	string boneName;
	int numPositionKey;
	int numScaleKey;
	int numRotationKey;

	// First make sure each channel has the same amount of position, scaling and rotation keys, and the times associated to each of them are the same
	forI(skeletalAnimation->m_numChannels)
	{
		numPositionKey = animation->mChannels[i]->mNumPositionKeys;
		numScaleKey    = animation->mChannels[i]->mNumScalingKeys;
		numRotationKey = animation->mChannels[i]->mNumRotationKeys;

		if (numPositionKey != numScaleKey)
		{
			cout << "ERROR not same values for numPositionKey and numScaleKey in Model::loadAnimationInformation" << endl;
			assert(false);
		}

		if (numScaleKey != numRotationKey)
		{
			cout << "ERROR not same values for numScaleKey and numRotationKey in Model::loadAnimationInformation" << endl;
			assert(false);
		}
	}

	if (skeletalAnimation->m_numChannels == 0)
	{
		cout << "ERROR animation has no channel information in Model::loadAnimationInformation" << endl;
		assert(false);
	}

	skeletalAnimation->m_vectorTime.resize(animation->mChannels[0]->mNumPositionKeys);

	if (animation->mChannels[0]->mNumPositionKeys < 2)
	{
		cout << "ERROR not enough animation key number in Model::loadAnimationInformation" << endl;
		assert(false);
	}

	float timeDifference = animation->mChannels[0]->mPositionKeys[1].mTime - animation->mChannels[0]->mPositionKeys[0].mTime;

	skeletalAnimation->m_numKeys = animation->mChannels[0]->mNumPositionKeys;

	forI(animation->mChannels[0]->mNumPositionKeys)
	{
		if (i < (animation->mChannels[0]->mNumPositionKeys - 1))
		{
			float tempTimeDifference = animation->mChannels[0]->mPositionKeys[i + 1].mTime - animation->mChannels[0]->mPositionKeys[i].mTime;

			if (abs(tempTimeDifference - timeDifference) > SMALL_FLOAT)
			{
				cout << "ERROR key not having same time difference" << endl;
				assert(false);
			}
		}
		skeletalAnimation->m_vectorTime[i] = animation->mChannels[0]->mPositionKeys[i].mTime;
	}

	skeletalAnimation->m_timeDifference = timeDifference;

	aiNodeAnim* nodeAnim;
	vec3 position;
	vec3 scale;
	quat rotation;
	int numKey;

	forI(skeletalAnimation->m_numChannels)
	{
		boneName = string(animation->mChannels[i]->mNodeName.C_Str());
		numKey   = animation->mChannels[i]->mNumPositionKeys;

		skeletalAnimation->m_mapBoneVectorPositionKey.insert(make_pair(boneName, vector<SkeletalAnimationKey>()));

		vector<SkeletalAnimationKey>& mapBoneVectorPositionKey = skeletalAnimation->m_mapBoneVectorPositionKey[boneName];
		mapBoneVectorPositionKey.resize(numKey);

		forJ(numKey)
		{
			nodeAnim                    = animation->mChannels[i];
			position                    = vec3AssimpCast(nodeAnim->mPositionKeys[j].mValue);
			scale                       = vec3AssimpCast(nodeAnim->mScalingKeys[j].mValue);
			rotation                    = quatAssimpCast(nodeAnim->mRotationKeys[j].mValue);
			mapBoneVectorPositionKey[j] = SkeletalAnimationKey{ position, scale, rotation };
		}
	}

	return skeletalAnimation;
}

/////////////////////////////////////////////////////////////////////////////////////////////
