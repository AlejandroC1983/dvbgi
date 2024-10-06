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

#ifndef _NODE_H_
#define _NODE_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/headers.h"
#include "../../include/util/genericresource.h"
#include "../../include/geometry/bbox.h"
#include "../../include/math/transform.h"
#include "../../include/component/componentenum.h"

// CLASS FORWARDING
class Material;
class Buffer;
class RenderComponent;
class DynamicComponent;

// NAMESPACE
using namespace componentenum;

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

class Node : public GenericResource
{
public:
	/** Parameter constructor
	* @param name      [in] name of the node
	* @param assetName [in] asset name
	* @param className [in] name of the class (for children classes)
	* @return nothing */
	Node(string&& name, string&& assetName, string&& className);

	/** Parameter constructor to fil all the per-vertex data of the node
	* @param name      [in] name of the node
	* @param assetName [in] asset name
	* @param className [in] name of the class (for children classes)
	* @param indices   [in] indices of the vertices conforming this vector
	* @param vertices  [in] vertex positions
	* @param texCoord  [in] texture coordinates
	* @param normals   [in] normal data
	* @param tangents  [in] tangent data
	* @return nothing */
	Node(string&& name, 
		 string&& assetName,
		 string&& className, 
		 const vectorUint& indices, 
		 const vectorVec3& vertices, 
		 const vectorVec2& texCoord,
		 const vectorVec3& normals, 
		 const vectorVec3& tangents);

	/** Parameter constructor to fil all the per-vertex data of the node
	* @param name                        [in] name of the node
	* @param assetName                   [in] asset name
	* @param className                   [in] name of the class (for children classes)
	* @param indices                     [in] indices of the vertices conforming this vector
	* @param vertices                    [in] vertex positions
	* @param texCoord                    [in] texture coordinates
	* @param normals                     [in] normal data
	* @param tangents                    [in] tangent data
	* @param boneWeight                  [in] vector with the bone weight information per vertex, up to MAXIMUM_BONES_PER_VERTEX bones per vertex
	* @param boneID                      [in] vector with the bone ID information per vertex, up to MAXIMUM_BONES_PER_VERTEX bones per vertex
	* @param mapBoneNameIndex            [in] map with information bone name -> bone index
	* @param vectorBoneInformation       [in] vector with information for each bone
	* @param vectorUpdateOrder           [in] vector with the indices in vectorBoneInformation to perform the bone updates
	* @param skeletalMeshParentTransform [in] transform to apply to all the skeletal mesh bones from higer hierarchy nodes which affect the skeletal mesh but have no bones
	* @param sceneLoadTransform          [in] transform applied to the vertices of the skeletal mesh at scene loading, needed to apply properly skinned mesh matrices to the mesh vertices
	* @return nothing */
	Node(string&& name, 
		 string&& assetName,
		 string&& className, 
		 vectorUint&& indices, 
		 vectorVec3&& vertices, 
		 vectorVec2&& texCoord, 
		 vectorVec3&& normals, 
		 vectorVec3&& tangents, 
		 vectorVec4&& boneWeight, 
		 vectorUVec4&& boneID, 
		 map<string, int>&& mapBoneNameIndex,
		 vector<BoneInformation>&& vectorBoneInformation,
		 /*vectorInt&& vectorUpdateOrder,*/
		 mat4&& skeletalMeshParentTransform,
		 mat4&& sceneLoadTransform);

	/** Default destructor
	* @return nothing */
	virtual ~Node();

	/** Initialize all the properties of this node: position, rotation, material and other parameter
	* @param vPosition            [in] node position
	* @param rotation             [in] node rotation
	* @param scale                [in] node scale
	* @param material             [in] material to use for this node for scene (main purpose) rasterization
	* @param materialInstanced    [in] material to use for this node for instanced rasterization
	* @param eMeshType            [in] node mesh type
	* @param initializeComponents [in] flag to know whether to initialize the components present in the node. By default it should be true unless changes are going to be be done to the node, like for instance turning a static node into a dynamic one at initialization time
	* @return true if initialization went ok and false otherwise */
	virtual bool initialize(const vec3& vPosition, const quat& rotation, const vec3& scale, Material* material, Material* materialInstanced, eMeshType eMeshType, bool initializeComponents);

	/** Initializes the components of the node
	* @return nothing */
	virtual void initializeComponents();

	/** Called before rasterizing the node, all the code that needs to be updated is executed here
	* @param fDt [in] elapsed time from last call
	* @return true if the node's transform was modified in the call, false otherwise */
	virtual bool prepare(const float &fDt);

	/** Tests whether vP is indide the model or not
	* param vP [in] point to test if it is inside the model geometry or not
	* @return nothing */
	virtual bool testPointInside(const vec3 &vP);

	/** Computes the bounding box of the model
	* @return nothing */
	virtual void computeBB();

	/** Compute the AABB of this model and center it in the origin of coordinates, updating it's transform to be properly
	* represented in world space
	* @return nothing */
	void centerToModelSpaceAndUpdateTransform();

	GET(BBox3D, m_aabb, BBox)
	REF(BBox3D, m_aabb, BBox)
	GET_SET(bool, m_followingPath, FollowingPath)
	REF_SET(Transform, m_transform, Transform)
	REF_SET(bool, m_affectSceneBB, AffectSceneBB)
	GETCOPY(float, m_instanceCounter, InstanceCounter)
	GET_PTR(RenderComponent, m_renderComponent, RenderComponent)
	REF_PTR(RenderComponent, m_renderComponent, RenderComponent)
	GET_PTR(DynamicComponent, m_dynamicComponent, DynamicComponent)
	REF_PTR(DynamicComponent, m_dynamicComponent, DynamicComponent)
	GETCOPY(eNodeType, m_nodeType, NodeType)
	REF(vectorBaseComponentPtr, m_vectorComponent, VectorComponent)
	GET(string, m_assetName, AssetName)

	/** Getter of m_transform.getMat()
	* @return m_transform.getMat() */
    mat4 getModelMat() const;

	/** Setter of m_aabb
	* @return nothing */
	void setBBox(BBox3D* pBBox);

	/** Update m_nodeType type based on the node's components and the information on them
	* @return nothing */
	void updateNodeType();

protected:
	BBox3D		           m_aabb;                 //!< Bounding box of the mesh
	bool		           m_followingPath;        //!< True if th model follows a path given
	Transform              m_transform;            //!< Node's transform
	bool		           m_affectSceneBB;        //!< If true, this node will be taken into account for the scene bb calculations whenever it is added to the scene or modified its bb (translated, rotated or scaled), true by default
	float                  m_instanceCounter;      //!< Current value of s_instanceCounter in Node constructor before its increased by one
	static float           s_instanceCounter;      //!< Static variable to know the number of nodes instanced and have per-element vertex information in shaders, float value is currnently used since per-vertex data is currently composed of 32-bit float values
	RenderComponent*       m_renderComponent;      //!< Pointer to the render component of this node
	DynamicComponent*      m_dynamicComponent;     //!< Pointer to the dynamic component of this node
	vectorBaseComponentPtr m_vectorComponent;      //!< Vector with components different from the RenderComponent and the Dynamic component this node might have to implement other features / functionalities
	eNodeType              m_nodeType;             //!< Enum describing the type of node (static / dynamic)
	string                 m_assetName;            //!< For debug purposes, name of the original asset this node represents in the scene
};

/////////////////////////////////////////////////////////////////////////////////////////////

// INLINE METHODS

/////////////////////////////////////////////////////////////////////////////////////////////

inline mat4 Node::getModelMat() const
{
	return m_transform.getMat();
}

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _NODE_H_
