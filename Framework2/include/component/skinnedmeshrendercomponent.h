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

#ifndef _SKINNEDMESHRENDERCOMPONENT_H_
#define _SKINNEDMESHRENDERCOMPONENT_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/component/rendercomponent.h"
#include "../../include/component/componentenum.h"
#include "../../include/skeletalanimation/skeletalanimation.h"

// CLASS FORWARDING

// NAMESPACE
using namespace componentenum;

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

class SkinnedMeshRenderComponent: public RenderComponent
{
	friend class ComponentManager;
	friend class Node;

protected:
	/** Paremeter constructor
	* @param name         [in] component resource name
	* @param className    [in] component class name
	* @param resourceType [in] component resource type
	* @return nothing */
	SkinnedMeshRenderComponent(string&& name, string&& className, GenericResourceType resourceType);

public:
	/** Default destructor
	* @return nothing */
	virtual ~SkinnedMeshRenderComponent();

	/** Update the bones and set the results in SkinnedMeshRenderComponent::m_vectorUsedBoneCurrentPose
	* @param dt [in] elapsed time since last update
	* @return nothing */
	void updateBones(float dt);

	/** Utility function to update the vertices with the current bone information available
	* @return nothing */
	void updatePoseVertexData();

	GET_SET(vectorVec4, m_boneWeight, BoneWeight)
	GET_SET(vectorUVec4, m_boneID, BoneID)
	SET_PTR(SkeletalAnimation, m_skeletalAnimation, SkeletalAnimation)
	GET(vectorMat4, m_vectorUsedBoneCurrentPose, VectorUsedBoneCurrentPose)
	REF(vectorFloat, m_vectorUsedBoneCurrentPoseFloat, VectorUsedBoneCurrentPoseFloat)
	GET(mat4, m_sceneLoadTransform, SceneLoadTransform)
	
protected:
	vectorVec4              m_boneWeight;                     //!< Vector with the bone weight information per vertex, up to MAXIMUM_BONES_PER_VERTEX bones per vertex
	vectorUVec4             m_boneID;                         //!< Vector with the bone ID information per vertex, up to MAXIMUM_BONES_PER_VERTEX bones per vertex
	map<string, int>        m_mapUsedBoneNameIndex;           //!< Map with information bone name->bone index
	vector<BoneInformation> m_vectorBoneInformation;          //!< Vector with information for each bone
	SkeletalAnimation*      m_skeletalAnimation;              //!< Skeletal animation to play for this skeletal mesh
	float                   m_skeletalAnimationTime;          //!< Current skeletal animation time for this skeletal mesh
	vectorMat4              m_vectorCurrentPose;              //!< Vector with the bone information for the current antimation being played m_skeletalAnimation, for the time given m_skeletalAnimationTime, for those bones being used in the skeletal animation (which can vary from the ones the skeletal animation might have, using more than the skeletal mesh)
	vectorMat4              m_vectorCurrentPoseNoOffset;      //!< Vector with the bone information with no bone offset for the current antimation being played m_skeletalAnimation, for the time given m_skeletalAnimationTime, for those bones being used in the skeletal animation (which can vary from the ones the skeletal animation might have, using more than the skeletal mesh)
	vectorMat4              m_vectorUsedBoneCurrentPose;      //!< Vector with the bone information for the current antimation being played m_skeletalAnimation, for the time given m_skeletalAnimationTime, for those bones being used in the skeletal mesh (which can be a subset of the bones used in the skeletal animation)
	vectorFloat             m_vectorUsedBoneCurrentPoseFloat; //!< Vector with the same information as m_vectorUsedBoneCurrentPose but as a set of 16 floats per matrix for easier sending the data to GPU
	mat4                    m_skeletalMeshParentTransform;    //!< Transform to apply to all the skeletal mesh bones from higer hierarchy nodes which affect the skeletal mesh but have no bones
	mat4                    m_sceneLoadTransform;             //!< Transform applied to the vertices of the skeletal mesh at scene loading, needed to apply properly skinned mesh matrices to the mesh vertices
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _SKINNEDMESHRENDERCOMPONENT_H_
