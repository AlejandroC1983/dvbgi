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
#include "../../include/node/node.h"
#include "../../include/scene/scene.h"
#include "../../include/util/loopmacrodefines.h"
#include "../../include/component/componentmanager.h"
#include "../../include/component/rendercomponent.h"
#include "../../include/component/dynamiccomponent.h"
#include "../../include/component/skinnedmeshrendercomponent.h"

// NAMESPACE

// DEFINES

// STATIC MEMBER INITIALIZATION
float Node::s_instanceCounter = 0.0f;

/////////////////////////////////////////////////////////////////////////////////////////////

Node::Node(string&& name, string&& assetName, string&& className): GenericResource(move(name), move(className), GenericResourceType::GRT_NODE)
	, m_followingPath(false)
	, m_affectSceneBB(true)
	, m_instanceCounter(s_instanceCounter)
	, m_renderComponent(nullptr)
	, m_dynamicComponent(nullptr)
	, m_nodeType(eNodeType::E_NT_STATIC_ELEMENT)
	, m_assetName(move(assetName))
{
	s_instanceCounter += 1.0f;
}

/////////////////////////////////////////////////////////////////////////////////////////////

Node::Node(string&&          name,
		   string&&          assetName,
		   string&&          className,
	       const vectorUint &indices,
		   const vectorVec3 &vertices,
		   const vectorVec2 &texCoord,
		   const vectorVec3 &normals,
		   const vectorVec3 &tangents) : GenericResource(move(string(name)), move(className), GenericResourceType::GRT_NODE)
	, m_followingPath(false)
	, m_affectSceneBB(true)
	, m_instanceCounter(s_instanceCounter)
	, m_renderComponent(nullptr)
	, m_dynamicComponent(nullptr)
	, m_nodeType(eNodeType::E_NT_STATIC_ELEMENT)
	, m_assetName(move(assetName))
{
	s_instanceCounter += 1.0f;

	m_renderComponent  = static_cast<RenderComponent*>(componentM->buildComponent(move(string(name + "_RenderComponent")), GenericResourceType::GRT_RENDERCOMPONENT));
	m_dynamicComponent = static_cast<DynamicComponent*>(componentM->buildComponent(move(string(name + "_DynamicComponent")), GenericResourceType::GRT_DYNAMICCOMPONENT));

	m_renderComponent->m_indices  = indices;
	m_renderComponent->m_vertices = vertices;
	m_renderComponent->m_normals  = normals;
	m_renderComponent->m_texCoord = texCoord;
	m_renderComponent->m_tangents = tangents;
}

/////////////////////////////////////////////////////////////////////////////////////////////

Node::Node(string&& name, 
		   string&& assetName,
		   string&& className, 
		   vectorUint&& indices, 
		   vectorVec3&& vertices, 
		   vectorVec2&& texCoord, 
		   vectorVec3&& normals, 
		   vectorVec3&& tangents, 
		   vectorVec4&& boneWeight, 
		   vectorUVec4&& boneID,
	       map<string, int>&& mapUsedBoneNameIndex,
	       vector<BoneInformation>&& vectorBoneInformation,
		   /*vectorInt&& vectorUpdateOrder,*/
		   mat4&& skeletalMeshParentTransform,
		   mat4&& sceneLoadTransform) : GenericResource(move(string(name)), move(className), GenericResourceType::GRT_NODE)
	, m_followingPath(false)
	, m_affectSceneBB(true)
	, m_instanceCounter(s_instanceCounter)
	, m_renderComponent(nullptr)
	, m_dynamicComponent(nullptr)
	, m_nodeType(eNodeType::E_NT_SKINNEDMESH_ELEMENT)
	, m_assetName(move(assetName))
{
	s_instanceCounter += 1.0f;

	m_renderComponent  = static_cast<RenderComponent*>(componentM->buildComponent(move(string(name + "_SkinnedMeshRenderComponent")), GenericResourceType::GRT_SKINNEDMESHRENDERCOMPONENT));
	m_dynamicComponent = static_cast<DynamicComponent*>(componentM->buildComponent(move(string(name + "_DynamicComponent")), GenericResourceType::GRT_DYNAMICCOMPONENT));

	m_renderComponent->setNodeType(m_nodeType);

	SkinnedMeshRenderComponent* skinnedMeshRenderComponent    = static_cast<SkinnedMeshRenderComponent*>(m_renderComponent);
	skinnedMeshRenderComponent->m_indices                     = move(indices);
	skinnedMeshRenderComponent->m_vertices                    = move(vertices);
	skinnedMeshRenderComponent->m_normals                     = move(normals);
	skinnedMeshRenderComponent->m_texCoord                    = move(texCoord);
	skinnedMeshRenderComponent->m_tangents                    = move(tangents);
	skinnedMeshRenderComponent->m_boneWeight                  = move(boneWeight);
	skinnedMeshRenderComponent->m_boneID                      = move(boneID);
	skinnedMeshRenderComponent->m_mapUsedBoneNameIndex        = move(mapUsedBoneNameIndex);
	skinnedMeshRenderComponent->m_vectorBoneInformation       = move(vectorBoneInformation);
	skinnedMeshRenderComponent->m_skeletalMeshParentTransform = move(skeletalMeshParentTransform);
	skinnedMeshRenderComponent->m_sceneLoadTransform          = move(sceneLoadTransform);

	skinnedMeshRenderComponent->m_vectorCurrentPose.resize(skinnedMeshRenderComponent->m_vectorBoneInformation.size());
	skinnedMeshRenderComponent->m_vectorCurrentPoseNoOffset.resize(skinnedMeshRenderComponent->m_vectorBoneInformation.size());
	skinnedMeshRenderComponent->m_vectorUsedBoneCurrentPose.resize(skinnedMeshRenderComponent->m_mapUsedBoneNameIndex.size());
	skinnedMeshRenderComponent->m_vectorUsedBoneCurrentPoseFloat.resize(skinnedMeshRenderComponent->m_mapUsedBoneNameIndex.size() * 16);
	
}

/////////////////////////////////////////////////////////////////////////////////////////////

Node::~Node()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Node::initialize(const vec3& vPosition, const quat& rotation, const vec3& scale, Material* material, Material* materialInstanced, eMeshType eMeshType, bool initializeComponents)
{
	GenericResourceType resourceType       = m_renderComponent->getResourceType();
	m_renderComponent->m_material	       = material;
	m_renderComponent->m_materialInstanced = materialInstanced;
	m_renderComponent->m_meshType          = eMeshType;
	m_transform                            = Transform(vPosition, rotation, scale);
	m_affectSceneBB                        = (eMeshType == eMeshType::E_MT_RENDER_MODEL);

	// TODO: Review code, refactor pending

	if (resourceType != GenericResourceType::GRT_SKINNEDMESHRENDERCOMPONENT)
	{
		centerToModelSpaceAndUpdateTransform();
	}

	m_renderComponent->setInstanceCounter(s_instanceCounter);

	if (m_dynamicComponent->getKeyframeInformationSet())
	{
		m_nodeType      = eNodeType::E_NT_DYNAMIC_ELEMENT;
		m_affectSceneBB = false; // Dynamic scene elements will not affect the scene's bounding box
	}

	if (m_renderComponent->getResourceType() == GenericResourceType::GRT_SKINNEDMESHRENDERCOMPONENT)
	{
		m_nodeType      = eNodeType::E_NT_SKINNEDMESH_ELEMENT;
		m_affectSceneBB = false; // Dynamic scene elements will not affect the scene's bounding box
	}

	m_renderComponent->setNodeType(m_nodeType);

	if (initializeComponents)
	{
		m_renderComponent->init();
		m_dynamicComponent->init();
	}

    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Node::initializeComponents()
{
	m_renderComponent->init();
	m_dynamicComponent->init();
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Node::prepare(const float &fDt)
{
	bool returnValue = false;

	if (m_dynamicComponent->getKeyframeInformationSet() && (fDt > 0.0f))
	{
		m_dynamicComponent->update(fDt, m_transform);
	}

	// Update bounding box if the model matrix changed
	if (m_transform.getDirty())
	{
		returnValue = true;
		m_aabb.setCenter(m_transform.getTraslation());

		// TODO: Pending to update the size of the BB as well, only the traslation is at the moment
		m_transform.setDirty(false);
		if (m_affectSceneBB)
		{
			sceneM->refBox().setDirty(true);
		}
	}

	return returnValue;
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Node::testPointInside(const vec3 &vP)
{
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Node::computeBB()
{
	m_aabb.computeFromModel(this);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Node::setBBox(BBox3D* pBBox)
{
	if (!pBBox)
	{
		return;
	}

	m_aabb.setMin(pBBox->getMin());
	m_aabb.setMax(pBBox->getMax());
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Node::updateNodeType()
{
	if (m_dynamicComponent->getKeyframeInformationSet())
	{
		m_nodeType      = eNodeType::E_NT_DYNAMIC_ELEMENT;
		m_affectSceneBB = false;
		m_dynamicComponent->init();
		m_renderComponent->setNodeType(m_nodeType);
		return;
	}

	if (m_renderComponent->getResourceType() == GenericResourceType::GRT_SKINNEDMESHRENDERCOMPONENT)
	{
		m_nodeType      = eNodeType::E_NT_SKINNEDMESH_ELEMENT;
		m_affectSceneBB = false;
		m_renderComponent->setNodeType(m_nodeType);
		return;
	}

	m_nodeType      = eNodeType::E_NT_STATIC_ELEMENT;
	m_affectSceneBB = true;
	m_renderComponent->setNodeType(m_nodeType);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Node::centerToModelSpaceAndUpdateTransform()
{
	// TODO: Refactor
	m_aabb.computeFromVertList(m_renderComponent->m_vertices, this);
	vec3 aabbCenter = m_aabb.getCenter();
	m_transform.setTraslation(aabbCenter);

	forI(m_renderComponent->m_vertices.size())
	{
		m_renderComponent->m_vertices[i] -= aabbCenter;
	}

	m_transform.setDirty(false);
	if (m_affectSceneBB)
	{
		sceneM->refBox().setDirty(true);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
