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

#ifndef _SKELETALANIMATIONMANAGER_H_
#define _SKELETALANIMATIONMANAGER_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/util/singleton.h"
#include "../../include/util/managertemplate.h"
#include "../headers.h"

// CLASS FORWARDING
class SkeletalAnimation;
class aiAnimation;

// NAMESPACE

// DEFINES
#define skeletalAnimationM s_pSkeletalAnimationManager->instance()

/////////////////////////////////////////////////////////////////////////////////////////////

class SkeletalAnimationManager : public ManagerTemplate<SkeletalAnimation>, public Singleton<SkeletalAnimationManager>
{
	friend class CoreManager;

public:
	/** Constructor
	* @return nothing */
	SkeletalAnimationManager();

	/** Destructor
	* @return nothing */
	virtual ~SkeletalAnimationManager();

	/** Load the information from the animation from assimp struct given as parameter and add a new element to the manager
	* @param animation    [in] animation struct from assimp
	* @param instanceName [in] name of the new instance
	* @return pointer to the new animation element generated */
	SkeletalAnimation* buildAnimation(string&& instanceName, aiAnimation* animation);
};

static SkeletalAnimationManager* s_pSkeletalAnimationManager;

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _SKELETALANIMATIONMANAGER_H_
