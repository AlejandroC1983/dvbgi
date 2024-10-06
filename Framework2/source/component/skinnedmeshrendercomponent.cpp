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

// PROJECT INCLUDES
#include "../../include/headers.h"
#include "../../include/util/loopmacrodefines.h"
#include "../../include/buffer/buffer.h"
#include "../../include/buffer/buffermanager.h"
#include "../../include/component/skinnedmeshrendercomponent.h"

// NAMESPACE

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

SkinnedMeshRenderComponent::SkinnedMeshRenderComponent(string&& name, string&& className, GenericResourceType resourceType) : RenderComponent(move(name), move(className), GenericResourceType::GRT_SKINNEDMESHRENDERCOMPONENT)
	, m_skeletalAnimation(nullptr)
	, m_skeletalAnimationTime(0.0f)
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

SkinnedMeshRenderComponent::~SkinnedMeshRenderComponent()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

void SkinnedMeshRenderComponent::updateBones(float dt)
{
	if (m_skeletalAnimation == nullptr)
	{
		return;
	}

	m_skeletalAnimationTime += dt;
	m_skeletalAnimationTime  = fmod(m_skeletalAnimationTime, m_skeletalAnimation->getDuration());

	map<string, vector<SkeletalAnimationKey>>& mapBoneVectorPositionKey = m_skeletalAnimation->refMapBoneVectorPositionKey();

	mat4 parentTransform;

	mat4 skeletalMeshParentTransformInverse = inverse(m_skeletalMeshParentTransform);

	forI(m_vectorBoneInformation.size())
	{
		BoneInformation& boneInformation = m_vectorBoneInformation[i];
		string& boneName                 = boneInformation.m_name;

		mat4 currentTransform = mat4(1.0f);

		if (mapBoneVectorPositionKey.find(boneName) != mapBoneVectorPositionKey.end())
		{
			int index0                       = int(floorf(m_skeletalAnimationTime / m_skeletalAnimation->getTimeDifference()));
			int index1                       = int(ceilf( m_skeletalAnimationTime / m_skeletalAnimation->getTimeDifference()));
			index1                           = clamp(index1, index0, m_skeletalAnimation->getNumKeys() - 1);
			vec3 position0                   = mapBoneVectorPositionKey[boneName][index0].m_position;
			vec3 position1                   = mapBoneVectorPositionKey[boneName][index1].m_position;
			vec3 scale0                      = mapBoneVectorPositionKey[boneName][index0].m_scale;
			vec3 scale1                      = mapBoneVectorPositionKey[boneName][index1].m_scale;
			quat rotation0                   = mapBoneVectorPositionKey[boneName][index0].m_rotation;
			quat rotation1                   = mapBoneVectorPositionKey[boneName][index1].m_rotation;

			float interpolationValue         = fmodf(m_skeletalAnimationTime, m_skeletalAnimation->getTimeDifference());
			vec3 positionInterpolated        = (1.0f - interpolationValue) * position0 + interpolationValue * position1;
			vec3 scaleInterpolated           = (1.0f - interpolationValue) * scale0    + interpolationValue * scale1;
			quat rotationInterpolated        = glm::slerp(rotation0, rotation1, interpolationValue);

			mat4 temp = mat4(1.0f);
			mat4 translation                 = glm::translate(mat4(1.0f), positionInterpolated);
			mat4 scale                       = glm::scale(mat4(1.0f), scaleInterpolated);
			mat4 rotation                    = mat4(rotationInterpolated);

			currentTransform                 = translation * rotation * scale;
		}
	
		if (i == 0)
		{
			currentTransform               = m_skeletalMeshParentTransform * currentTransform;
			m_vectorCurrentPoseNoOffset[i] = m_skeletalMeshParentTransform;
		}

		if (boneInformation.m_parentBoneIndex.size() > 0)
		{
			int parentIndex = boneInformation.m_parentBoneIndex[0];
			m_vectorCurrentPoseNoOffset[i] = m_vectorCurrentPoseNoOffset[parentIndex] * currentTransform;
		}
		
		int parentIndex        = (boneInformation.m_parentBoneIndex.size() > 0) ? boneInformation.m_parentBoneIndex[0] : -1;
		mat4 parentTransform   = (parentIndex == -1) ? mat4(1.0f) : m_vectorCurrentPoseNoOffset[parentIndex];
		m_vectorCurrentPose[i] = skeletalMeshParentTransformInverse * parentTransform * currentTransform * boneInformation.m_offsetMatrix;

		if (m_mapUsedBoneNameIndex.find(boneName) != m_mapUsedBoneNameIndex.end())
		{
			int indexStore = m_mapUsedBoneNameIndex[boneName];
			m_vectorUsedBoneCurrentPose[indexStore] = m_vectorCurrentPose[i];
			memcpy(m_vectorUsedBoneCurrentPoseFloat.data() + indexStore * 16, value_ptr(m_vectorCurrentPose[i]), 16 * sizeof(float));
		}		
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void SkinnedMeshRenderComponent::updatePoseVertexData()
{
	forI(m_vertices.size())
	{
		vec3 vertex         = m_vertices[i];
		vec4 vertex4D       = vec4(vertex.x, vertex.y, vertex.z, 1.0);

		mat4 loadTransform  = mat4(inverse(m_sceneLoadTransform));
		vertex4D            = loadTransform * vertex4D;

		mat4 m0 = m_vectorUsedBoneCurrentPose[m_boneID[i].x];
		mat4 m1 = m_vectorUsedBoneCurrentPose[m_boneID[i].y];
		mat4 m2 = m_vectorUsedBoneCurrentPose[m_boneID[i].z];
		mat4 m3 = m_vectorUsedBoneCurrentPose[m_boneID[i].w];

		mat4 transformBone  = mat4(m_vectorUsedBoneCurrentPose[m_boneID[i].x]) * double(m_boneWeight[i].x) +
			                  mat4(m_vectorUsedBoneCurrentPose[m_boneID[i].y]) * double(m_boneWeight[i].y) +
			                  mat4(m_vectorUsedBoneCurrentPose[m_boneID[i].z]) * double(m_boneWeight[i].z) +
			                  mat4(m_vectorUsedBoneCurrentPose[m_boneID[i].w]) * double(m_boneWeight[i].w);

		vec4 vertexFinal    = m_sceneLoadTransform * transformBone * vertex4D;
		m_vertices[i]		= vec3(vertexFinal.x, vertexFinal.y, vertexFinal.z);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
