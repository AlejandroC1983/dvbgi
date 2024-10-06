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

#ifndef _MODEL_H_
#define _MODEL_H_

// GLOBAL INCLUDES
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// PROJECT INCLUDES
#include "../../include/headers.h"
#include "../../include/util/getsetmacros.h"
#include "../../include/geometry/bbox.h"
#include "../../include/material/materialenum.h"
#include "../../include/component/componentenum.h"

// CLASS FORWARDING
class Node;

// NAMESPACE
using namespace materialenum;
using namespace componentenum;

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

/** Simple model class to load models form the assimp library, each mesh is added to the scene as a node
*   the scene is node focused, being the model class a simple container at this moment, so the destruction of
*   scene nodes is carried out by the scene itself and not by each model. Information about node hierarchy is
*   pending. */

class Model
{
public:
	/** Default constructor
	* @return nothing */
	Model();

	/** Parameter constructor: loads a model given by the path parameter, adding each mesh in the model as
	* a single node to the scene
	* @param path                   [in] path to the scene or model to load
	* @param name                   [in] scene or model file to load
	* @param XYZToMinusXZY          [in] when loading models in a format like 3DS max, where the axis are different, the coordinates of the verices and vectors are switched to match OpenGL standard
	* @param makeMergedGeometryNode [in] if true, information to build a node with all the merged geometry of the model will be stored. This node can be retrieved by calling getCompactedGeometryNode
	* @param sceneTransform         [in] transform to apply to all vertices / normals / tangents for all geometry loaded, use identity matrix for no changes
	* @param initializeComponents   [in] Flag to know whether to initialize the components of the nodes built after loading the scene
	* @return nothing */
	Model(const string &path, const string &name, bool XYZToMinusXZY, bool makeMergedGeometryNode, mat4&& sceneTransform, bool initializeComponents);

	/** Destructor
	* @return nothing */
	~Model();

	/** Adds all the meshes present in m_vecMesh to the scene
	* @return nothing */
	void addMeshesToScene();

	/** Sets the rotation given as parameter to all nodes in m_vecMesh
	* @param rotation [in] rotation to apply
	* @return nothing */
	void setRotationToNodes(quat rotation);

	/** Sets the translation given as parameter to all nodes in m_vecMesh
	* @param translation [in] translation to apply
	* @return nothing */
	void setTranslationToNodes(vec3 translation);

	/** Adds the translation given as parameter to all nodes in m_vecMesh
	* @param translation [in] translation to apply
	* @return nothing */
	void addTranslationToNodes(vec3 translation);

	/** Sets the scaling given as parameter to all nodes in m_vecMesh
	* @param scaling [in] scaling to apply
	* @return nothing */
	void setScalingToNodes(vec3 scaling);

	/** If m_makeMergedGeometryNode is true, a node with the compacted geometry of the model will be generated and
	* returned as parameter
	* @param nodeName [in] name to assign to the node
	* @return node generated with the compacted goemetry of the model */
	Node* getCompactedGeometryNode(string&& nodeName);

	/** Helper function to process animation data related to a scene node
	* @param animation [in] pointer to the animation data
	* @param sceneNode [in] pointer to the node to which assign the antimation data
	* @return nothing */
	static void processAnimationData(aiAnimation* animation, Node* sceneNode);

	/** Helper function to perform breadth first traversal on the tree given by baseNode and all its children
	* @param baseNode         [in] base node where to start the tree traversal
	* @param vectorTempResult [in] temporal vector used to build the final result
	* @return nothing */
	//static void breadthFirstTraversal(aiNode* baseNode, vector<aiNode*>& vectorTempResult);
	static void inorderTreeTraversal(aiNode* baseNode, vector<aiNode*>& vectorTempResult);

	/** Helper function to cache for each element in vectorTempResult, all the node names in the node hierarchy until the root node
	* @param nodeVector                  [in] vector with the nodes to track hierarchy information
	* @param mapUsedBoneNameIndex        [in] map with key -> value "bone name" -> "bone index"
	* @param vectorBoneInformation       [in] vector with each BoneInformation struct where to store the hierarchy information (BoneInformation::m_parentBone)
	* @param vectorUpdateOrder           [in] vector with the index order to follow in vectorBoneInformation to update skeletal mesh bone transform
	* @param skeletalMeshParentTransform [in] transform to apply to all the skeletal mesh bones from higer hierarchy nodes which affect the skeletal mesh but have no bones
	* @return nothing */
	static void fillNodeParentInformation(const vector<aiNode*>&         nodeVector, 
										        vector<BoneInformation>& vectorBoneInformation);

	REF(vectorNodePtr, m_vecMesh, VecMesh)
	REF(string, m_path, Path)
	REF(string, m_name, Name)
	GET(BBox3D, m_aabb, AABB)

protected:
	/** Loads the model given by the path variable, returns true if everything went ok and false otherwise
	* param path [in] path to the model to load
	* @return true if everything went ok and false otherwise */
	bool loadModel(string path);

	/** Recursive function used to process all the nodes and therefore children of the model loaded with
	* the assimp library
	* @param node		  [in] assimp node to process
	* @param scene		  [in] assimp scene which contains the model that contains the node
	* @param accTransform [in] accumulated transform for the nodes and children meshes of the model
	* @return nothing */
	void processNode(aiNode* node, const aiScene* scene, const mat4 accTransform);

	/** Generates a new node added to the scene from each mesh given by each node of the assimp loaded model
	* #param node      [in] assimp node containing the mesh to get data from and generate a new scene node
	* #param mesh      [in] assimp mesh to get data from and generate a new scene node
	* #param scene     [in] assimp scene which contains the model that contains the node
	* #param transform [in] transform info for this node
	* @return new node added to the scene from each mesh given by each node of the assimp loaded model */
	Node *processMesh(aiNode* node, aiMesh* mesh, const aiScene* scene, const mat4&transform);

	/** Generates a new node added to the scene from each skeletal mesh given by each node of the assimp loaded model
	* #param node         [in] assimp node containing the mesh to get data from and generate a new scene node
	* #param skeletalMesh [in] assimp skeletal mesh to get data from and generate a new scene node
	* #param scene        [in] assimp scene which contains the model that contains the node
	* #param transform    [in] transform info for this node
	* @return new node added to the scene from each skeletal mesh given by each node of the assimp loaded model */
	Node* processSkeletalMesh(aiNode* node, aiMesh* skeletalMesh, const aiScene* scene, const mat4& transform);

	/** Loops through all the nodes starting from the root node of the scene, and returns true if all of the meshes have tangent and
	* bitangent data, and false otherwise
	* @param scene [in] assimp scene
	* @return and returns true if all of the meshes have tangent and bitangent data, and false otherwise */
	bool hasTangentAndBitangent(const aiScene* scene);

	/** Loops through all the meshes starting from the node given as parameter, returns true if all of them have tangent and
	* bitangent data, and false otherwise
	* @param scene  [in] assimp scene
	* @param node   [in] current node
	* @param result [in] result to compare with after each mesh is tested
	* @return nothing */
	void hasTangentAndBitangent(const aiScene* scene, const aiNode* node, bool &result);

	/** Loops through all the nodes starting from the root node of the scene, and returns true if all of the meshes have normal
	* data, and false otherwise
	* @param scene [in] assimp scene
	* @return returns true if all of the meshes have normal data, and false otherwise */
	bool hasNormal(const aiScene* scene);

	/** Loops through all the meshes starting from the node given as parameter, returns true if all of them have normal
	* data, and false otherwise
	* @param scene  [in] assimp scene
	* @param node   [in] current node
	* @param result [in] result to compare with after each mesh is tested
	* @return nothing */
	void hasNormal(const aiScene* scene, const aiNode* node, bool &result);

	/** Loops through all the loaded material information from const aiScene* scene and makes new materials. The reflectance texture of
	* the material is given by the AI_MATKEY_TEXTURE_DIFFUSE(0) enum in aiMaterial::Get. The normal texture is assumed to exist, and
	* have the same name, but with the "_N" suffix. For instance, for "Name.png", the normal texture is assumed to be called "Name_N.png"
	* NOTE: since only .ktx files can be loaded, a small hck to replace the file extension is done (the texture file extensions are changed
	* to "ktx"), assuming the corresponding file also exists to be loaded.
	* NOTE: all textures loaded are assumed to be RGBA8 in .ktx format right now.
	* @param scene [in] assimp scene
	* @return nothing */
	void loadMaterials(const aiScene* scene);

	/** Helper method: Takes the texture file name recovered, textureName, and returns the corresponding reflectance and normal texture
	* names for loading
	* @param textureName [in] texture name to generate the two resulting ones
	* @param reflectance [in] final reflectance texture name
	* @param normal      [in] final normal texture name */
	static void getTextureNames(const string& textureName, string& reflectance, string& normal);

	/** Helper method: Returns the material texture type (opaque / alpha tested / alpha blended) using keywords present in the material texture,
	* pending to implement a proper way to do it.
	* @param textureName [in] texture name to generate the two resulting ones
	* @return The material texture type for the texture name provided */
	static MaterialSurfaceType getMaterialTextureType(const string& textureName);

	/** Helper method: Takes the texture file name recovered, textureName, and returns the corresponding reflectance and normal texture
	* names for loading
	* @param vectorIndex    [in] index information to add to the merged geometry node
	* @param vectorVertex   [in] vertex information to add to the merged geometry node
	* @param vectorTexCoord [in] texture cordinates information to add to the merged geometry node
	* @param vectorNormal   [in] normal information to add to the merged geometry node
	* @param vectorTangent  [in] tangent information to add to the merged geometry node
	* @return nothing */
	void addMergedGeometryNodeInformation(const vectorUint& vectorIndex,
										  const vectorVec3& vectorVertex,
										  const vectorVec2& vectorTexCoord,
										  const vectorVec3& vectorNormal,
										  const vectorVec3& vectorTangent);

	/** Look in the aiCamera array in the scene to look for a camera with name given as parameter. If found, then 
	* retrieve information from that camera (position, up, lookAt, zNear, zFar)
	* @param scene      [in] assimp scene to look for the camera
	* @param cameraName [in] camera name
	* @param position   [in] position
	* @param lookAt     [in] look at direction
	* @param up         [in] up direction
	* @param zNear      [in] clip plane near distance
	* @param zFar       [in] clip plane far distance
	* @return true if the camera was found, false otherwise */
	static bool getCameraInformation(const aiScene* scene, string&& cameraName, vec3& position, vec3& lookAt, vec3& up, float& zNear, float& zFar);

	/** Utility function to know whether a scene has no skinned meshes and at least one animation for any of the static meshes present
	* @param scene [in] assimp scene to perform the test
	* @return true if scene has no skinned meshes and at least one animation for any of the static meshes present, false otherwise */
	static bool sceneHasOnlyStaticMeshAnimations(const aiScene* scene);

protected:
	vectorNodePtr             m_vecMesh;                  //!< vector with the meshes of this model
	string		              m_path;                     //!< path to the scene or model
	string		              m_name;                     //!< file name of the scene or model
	bool		              m_XYZToMinusXZY;            //!< When loading models in a format like 3DS max, where the axis are different, the coordinates of the verices and vectors are switched to match OpenGL standard
	BBox3D                    m_aabb;                     //!< Model aabb
	bool                      m_makeMergedGeometryNode;   //!< if true, a node with all the geometry of all nodes of the model will be built
	vectorUint                m_mergedIndex;              //!< temp vector used to store information for the merged geometry node (if m_makeMergedGeometryNode is true)
	vectorVec3                m_mergedVertex;             //!< temp vector used to store information for the merged geometry node (if m_makeMergedGeometryNode is true)
	vectorVec2                m_mergedTexCoord;           //!< temp vector used to store information for the merged geometry node (if m_makeMergedGeometryNode is true)
	vectorVec3                m_mergedNormal;             //!< temp vector used to store information for the merged geometry node (if m_makeMergedGeometryNode is true)
	vectorVec3                m_mergedTangent;            //!< temp vector used to store information for the merged geometry node (if m_makeMergedGeometryNode is true)
	vectorString              m_vectorEmitterName;        //!< Temp vector used to store emitter names (emitter information is not available in the aiLight structure, storing the name of all emitters to obtain its transform data when processing scene nodes)
	vectorString              m_vectorCameraName;         //!< Temp vector used to store camera names (camera information is not available in the aiCamera structure, storing the name of all cameras to obtain its transform data when processing scene nodes)
	map<string, aiAnimation*> m_mapNodeNameAnimation;     //!< Temp map used to store names of the nodes that have animation related information and the pointer to the animation data
	mat4                      m_sceneTransform;           //!< Transform to apply to all vertices / normals / tangents for all geometry loaded, use identity matrix for no changes
	bool                      m_initializeComponents;     //!< Flag to know whether to initialize the components of the nodes built after loading the scene
	static int                m_modelCounter;             //!< Simple counter to name added models to the scene
	static int                m_skeletalAnimationCounter; //!< Simple counter to name added skeletal animations to the scene
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _MODEL_H_
