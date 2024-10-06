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

// GLOBAL INCLUDES
#include <cctype>
#include <experimental/filesystem>
#include "../../external/glm/glm/gtc/type_ptr.hpp"

// PROJECT INCLUDES
#include "../../include/model/model.h"
#include "../../include/node/node.h"
#include "../../include/scene/scene.h"
#include "../../include/util/mathutil.h"
#include "../../include/util/loopMacroDefines.h"
#include "../../include/texture/texturemanager.h"
#include "../../include/texture/texture.h"
#include "../../include/parameter/attributedefines.h"
#include "../../include/parameter/attributedata.h"
#include "../../include/material/materialcolortexture.h"
#include "../../include/camera/camera.h"
#include "../../include/camera/cameramanager.h"
#include "../../include/component/basecomponent.h"
#include "../../include/component/dynamiccomponent.h"
#include "../../include/util/containerutilities.h"
#include "../../include/component/rendercomponent.h"
#include "../../include/skeletalanimation/skeletalanimationmanager.h"
#include "../../include/skeletalanimation/skeletalanimation.h"
#include "../../include/model/modelenum.h"
#include "../../include/component/skinnedmeshrendercomponent.h"

// NAMESPACE
using namespace attributedefines;
using namespace modelenum;

// DEFINES

// STATIC MEMBER INITIALIZATION
int Model::m_modelCounter             = 0;
int Model::m_skeletalAnimationCounter = 0;

/////////////////////////////////////////////////////////////////////////////////////////////

Model::Model():
	  m_path("")
	, m_name("")
	, m_XYZToMinusXZY(false)
	, m_makeMergedGeometryNode(false)
	, m_initializeComponents(true)
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

Model::Model(const string &path, const string &name, bool XYZToMinusXZY, bool makeMergedGeometryNode, mat4&& sceneTransform, bool initializeComponents):
	  m_path(path)
	, m_name(name)
	, m_XYZToMinusXZY(XYZToMinusXZY)
	, m_makeMergedGeometryNode(makeMergedGeometryNode)
	, m_sceneTransform(move(sceneTransform))
	, m_initializeComponents(initializeComponents)
{
	bool result = loadModel(path + name);

	if (!result)
	{
		cout << "ERROR loading model with path " << path << endl;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

Model::~Model()
{
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Model::loadModel(string path)
{
	Assimp::Importer import;
	const aiScene* scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_FlipWindingOrder | aiProcess_CalcTangentSpace);

	if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		cout << "ERROR::ASSIMP::" << import.GetErrorString() << endl;
		return false;
	}

	if (!hasNormal(scene))
	{
		scene = import.ApplyPostProcessing(aiProcess_GenNormals);
	}

	if (!hasTangentAndBitangent(scene))
	{
		scene = import.ApplyPostProcessing(aiProcess_CalcTangentSpace);
	}

	scene = import.ApplyPostProcessing(aiProcess_ImproveCacheLocality);

	loadMaterials(scene);

	if (scene->HasLights())
	{
		uint numLight = scene->mNumLights;

		forI(numLight)
		{
 			m_vectorEmitterName.push_back(string(scene->mLights[i]->mName.C_Str()));
		}
	}

	if (scene->HasCameras())
	{
		uint numCamera = scene->mNumCameras;
		forI(numCamera)
		{
			m_vectorCameraName.push_back(string(scene->mCameras[i]->mName.C_Str()));
		}
	}

	// TODO: Add a test in case the scene has no meshes with bones, to distinguish between animations for static meshes and the ones for skinned meshes
	if(sceneHasOnlyStaticMeshAnimations(scene))
	{
		uint numAnimation = scene->mNumAnimations;
		forI(numAnimation)
		{
			aiAnimation* tempAnimation = scene->mAnimations[i];
			if (scene->mAnimations[i]->mChannels == nullptr)
			{
				cout << "ERROR: NO ANIMATION CHANNELS FOR ANIMATION AT INDEX " << i << endl;
				continue;
			}

			string animationNodeName = string(scene->mAnimations[i]->mChannels[0]->mNodeName.C_Str());
			map<string, aiAnimation*>::iterator it = m_mapNodeNameAnimation.find(animationNodeName);

			if (it != m_mapNodeNameAnimation.end())
			{
				cout << "ERROR: TRYING TO ADD ANIMATION FOR NODE " << scene->mAnimations[i]->mChannels[0]->mNodeName.C_Str() << " WHICH ALREADY EXISTS" << endl;

				// If an animation node with the same name is found (there might be several copies of the same animation) pick the one with
				// a higher amount of keyframes
				if (it->second->mChannels[0]->mNumPositionKeys < tempAnimation->mChannels[0]->mNumPositionKeys)
				{
					cout << "WARNING: ANIMATION FOR NODE " << scene->mAnimations[i]->mChannels[0]->mNodeName.C_Str() << " WAS REPLACED SINCE ANOTHER WITH BIGGER AMOUNT OF KEYFRAMES WITH SAME NAME WAS FOUND" << endl;
					it->second = tempAnimation;
				}

				continue;
			}

			
			addIfNoPresent(move(animationNodeName), tempAnimation, m_mapNodeNameAnimation);
		}
	}

	processNode(scene->mRootNode, scene, m_sceneTransform);
	m_aabb = BBox3D::computeFromNodeVector(m_vecMesh);

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Model::processNode(aiNode* node, const aiScene* scene, const mat4 accTransform)
{
	mat4 transform = mat4AssimpCast(node->mTransformation) * accTransform;

	vectorString::iterator it = find(m_vectorEmitterName.begin(), m_vectorEmitterName.end(), string(node->mName.C_Str()));

	if (it != m_vectorEmitterName.end())
	{
		//mat4 nodeMatrix = MathUtil::assimpAIMatrix4x4ToGLM(&transform);
		mat4 nodeMatrix = transform;

		glm::vec3 scale;
		glm::quat rotation;
		glm::vec3 translation;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(nodeMatrix, scale, rotation, translation, skew, perspective);

		const mat4 inverted      = inverse(nodeMatrix);
		const vec3 lookAt        = normalize(glm::vec3(inverted[2]));
		vec4 lookAtTransformed4D = nodeMatrix * vec4(lookAt, 0.0f);
		vec3 lookAtTransformed   = vec3(lookAtTransformed4D.x, lookAtTransformed4D.y, lookAtTransformed4D.z);

		Camera* sceneCamera = cameraM->buildCamera(move(string(node->mName.C_Str())),
			CameraType::CT_FIRST_PERSON,
			translation,
			lookAtTransformed,
			vec3(0.0f, 1.0f, 0.0f),
			ZNEAR,
			ZFAR,
			glm::pi<float>() * 0.25f);
	}

	it = find(m_vectorCameraName.begin(), m_vectorCameraName.end(), string(node->mName.C_Str()));

	if (it != m_vectorCameraName.end())
	{
		float zFar;
		float zNear;

		vec3 up;
		vec3 position;
		vec3 lookAt;
		
		bool result = getCameraInformation(scene, move(string(node->mName.C_Str())), position, lookAt, up, zNear, zFar);

		if(result)
		{
			//mat4 nodeMatrix          = MathUtil::assimpAIMatrix4x4ToGLM(&transform);
			mat4 nodeMatrix          = transform;
			mat4 inverted            = inverse(nodeMatrix);
			vec3 lookAt              = normalize(glm::vec3(inverted[2]));
			vec4 lookAtTransformed4D = nodeMatrix * vec4(lookAt, 0.0f);
			vec3 lookAtTransformed   = vec3(lookAtTransformed4D.x, lookAtTransformed4D.y, lookAtTransformed4D.z);
			vec4 upTransformed4D     = nodeMatrix * vec4(up, 0.0f);
			vec3 upTransformed       = vec3(upTransformed4D.x, upTransformed4D.y, upTransformed4D.z);

			Camera* sceneCamera = cameraM->buildCamera(move(string(node->mName.C_Str())),
													   CameraType::CT_FIRST_PERSON,
													   vec3(-13.8768f, 8.181f, 8.78848f),
													   lookAtTransformed,
													   upTransformed,
													   zNear,
													   zFar,
													   glm::pi<float>() * 0.25f); // Assimp import from .fbx won't load correct fov value https://github.com/assimp/assimp/issues/245
		}
	}

	// Process all the node's meshes (if any)
	aiMesh* mesh = nullptr;
	forI(node->mNumMeshes)
	{
		mesh = scene->mMeshes[node->mMeshes[i]];

		if (mesh->HasBones())
		{
			Node* temp = processSkeletalMesh(node, mesh, scene, transform);
			m_vecMesh.push_back(temp);
		}
		else
		{
			if (mesh->HasTextureCoords(0)) // Avoid those meshes without texture coordinates
			{
				Node* temp = processMesh(node, mesh, scene, transform);
				m_vecMesh.push_back(temp);
			}
			else
			{
				cout << "ERROR: mesh " << mesh->mName.C_Str() << " has no texture coordinates" << endl;
			}
		}
	}

	// Then do the same for each of its children
	forI(node->mNumChildren)
	{
		processNode(node->mChildren[i], scene, transform);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Model::hasTangentAndBitangent(const aiScene* scene)
{
	bool result = true;
	hasTangentAndBitangent(scene, scene->mRootNode, result);
	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Model::hasTangentAndBitangent(const aiScene* scene, const aiNode* node, bool &result)
{
	// Loop through each mesh of this node and test if the node has tangent and bitangent data
	aiMesh* mesh = nullptr;
	forI(node->mNumMeshes)
	{
		mesh = scene->mMeshes[node->mMeshes[i]];
		if (!mesh->HasTangentsAndBitangents())
		{
			result = false;
			return;
		}
	}

	// If all mesh have tangent and bitangent data, try the children
	for (uint i = 0; i < node->mNumChildren && result; ++i)
	{
		hasTangentAndBitangent(scene, node->mChildren[i], result);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Model::hasNormal(const aiScene* scene)
{
	bool result = true;
	hasNormal(scene, scene->mRootNode, result);
	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Model::hasNormal(const aiScene* scene, const aiNode* node, bool &result)
{
	// Loop through each mesh of this node and test if the node has normal data
	aiMesh* mesh = nullptr;
	forI(node->mNumMeshes)
	{
		mesh = scene->mMeshes[node->mMeshes[i]];
		if (!mesh->HasNormals())
		{
			result = false;
			return;
		}
	}

	// If all mesh have tangent and bitangent data, try the children
	for (uint i = 0; i < node->mNumChildren && result; ++i)
	{
		hasNormal(scene, node->mChildren[i], result);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Model::loadMaterials(const aiScene* scene)
{
	aiMaterial* material;
	aiString materialName;
	aiString reflectanceTextureName;

	int numReflectance;
	string materialString;
	size_t found;
	string reflectanceTextureFinalName;
	string normalTextureFinalName;
	Texture* reflectance;
	Texture* normal;

	forI(scene->mNumMaterials)
	{
		material = scene->mMaterials[i];
		material->Get(AI_MATKEY_NAME, materialName);

		materialString = materialName.C_Str();

		std::transform(materialString.begin(), materialString.end(), materialString.begin(), ::tolower);
		found = materialString.find(string("emitter"));

		if (found != string::npos)
		{
			materialM->buildMaterial(move(string("MaterialPlainColor")), move(string(materialName.C_Str())), nullptr);
		}
		else
		{
			numReflectance                            = material->GetTextureCount(aiTextureType_DIFFUSE);
			bool overwriteReflectanceTextureFinalName = true;

			if (numReflectance == 0)
			{
				cout << "WARNING: no reflectance texture information for material " << materialName.C_Str() << " at Model::loadMaterials, trying to find reflectance and normal .ktx textures from material names" << endl;

				// Try to find reflectance and normal textures based on the material's name
				reflectanceTextureFinalName   = materialString;
				string reflectanceTexturePath = m_path + reflectanceTextureFinalName + ".dds";
				string normalTexturePath      = m_path + reflectanceTextureFinalName + "_N.dds";

				if (!std::experimental::filesystem::exists(reflectanceTexturePath))
				{
					cout << "ERROR: unable to find reflectance texture file based on material name for material " << materialName.C_Str() << " at Model::loadMaterials" << endl;
					continue;
				}

				if (!std::experimental::filesystem::exists(normalTexturePath))
				{
					cout << "ERROR: unable to find normal texture file based on material name for material " << materialName.C_Str() << " at Model::loadMaterials" << endl;
					continue;
				}

				overwriteReflectanceTextureFinalName = false;
			}

			if (overwriteReflectanceTextureFinalName)
			{
				material->Get(AI_MATKEY_TEXTURE_DIFFUSE(0), reflectanceTextureName);
				reflectanceTextureFinalName = reflectanceTextureName.C_Str();
			}

			if (reflectanceTextureFinalName == "")
			{
				cout << "ERROR: no reflectance texture name for material " << materialName.C_Str() << " at Model::loadMaterials" << endl;
			}

			assert(reflectanceTextureFinalName != "");

			getTextureNames(reflectanceTextureFinalName, reflectanceTextureFinalName, normalTextureFinalName);

			MaterialSurfaceType surfaceType = getMaterialTextureType(reflectanceTextureFinalName);

			reflectance = textureM->build2DTextureFromFile(
				move(string(reflectanceTextureFinalName)),
				move(m_path + reflectanceTextureFinalName),
				VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_FORMAT_BC7_SRGB_BLOCK);

			normal = textureM->build2DTextureFromFile(
				move(string(normalTextureFinalName)),
				move(m_path + normalTextureFinalName),
				VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_FORMAT_BC5_UNORM_BLOCK);

			MultiTypeUnorderedMap *attributeMaterial = new MultiTypeUnorderedMap();
			attributeMaterial->newElement<AttributeData<string>*>(new AttributeData<string>(string(g_reflectanceTextureResourceName), string(reflectance->getName())));
			attributeMaterial->newElement<AttributeData<string>*>(new AttributeData<string>(string(g_normalTextureResourceName), string(normal->getName())));

			if (surfaceType != MaterialSurfaceType::MST_OPAQUE)
			{
				attributeMaterial->newElement<AttributeData<MaterialSurfaceType>*>(new AttributeData<MaterialSurfaceType>(string(g_materialSurfaceType), move(surfaceType)));
			}

			materialM->buildMaterial(move(string("MaterialColorTexture")), move(string(materialName.C_Str())), attributeMaterial);
			materialM->buildMaterial(move(string("MaterialIndirectColorTexture")), move(string(materialName.C_Str()) + "Instanced"), attributeMaterial);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Model::getTextureNames(const string& textureName, string& reflectance, string& normal)
{
	if (textureName.find("_BaseColor") != string::npos)
	{
		int indexLastUnderScore = int(textureName.find_last_of('_'));
		string newTextureName = textureName.substr(0, indexLastUnderScore);
		reflectance = "/Textures/" + newTextureName + ".dds";
		normal = "/Textures/" + newTextureName + "_N.dds";
	}
	else
	{
		string temp = textureName.substr(0, textureName.find_last_of('.'));
		reflectance = temp + ".dds";
		normal = temp + "_N.dds";
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

MaterialSurfaceType Model::getMaterialTextureType(const string& textureName)
{
	const vectorString& transparentKeywords = sceneM->getTransparentKeywords();

	forI(transparentKeywords.size())
	{
		auto it = std::search(textureName.begin(), textureName.end(), transparentKeywords[i].begin(), transparentKeywords[i].end(), [](char char1, char char2) { return std::toupper(char1) == std::toupper(char2); });
		if (it != textureName.end())
		{
			cout << "TEXTURE " << textureName << " will be alpha tested " << endl;
			return MaterialSurfaceType::MST_ALPHATESTED;
		}
	}

	return MaterialSurfaceType::MST_OPAQUE;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Model::addMergedGeometryNodeInformation(const vectorUint& vectorIndex,
											 const vectorVec3& vectorVertex,
											 const vectorVec2& vectorTexCoord,
											 const vectorVec3& vectorNormal,
											 const vectorVec3& vectorTangent)
{
	uint offset      = uint(m_mergedVertex.size());
	m_mergedVertex.insert(  m_mergedVertex.end(),   vectorVertex.begin(),   vectorVertex.end());
	m_mergedNormal.insert(  m_mergedNormal.end(),   vectorNormal.begin(),   vectorNormal.end());
	m_mergedTexCoord.insert(m_mergedTexCoord.end(), vectorTexCoord.begin(), vectorTexCoord.end());
	m_mergedTangent.insert( m_mergedTangent.end(),  vectorTangent.begin(),  vectorTangent.end());
	uint startIndex  = uint(m_mergedIndex.size());
	m_mergedIndex.insert(m_mergedIndex.end(),       vectorIndex.begin(),    vectorIndex.end());
	uint endIndex    = uint(m_mergedIndex.size());

	if (startIndex > 0)
	{
		forIFrom(startIndex, endIndex)
		{
			m_mergedIndex[i] += offset;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Model::getCameraInformation(const aiScene* scene, string&& cameraName, vec3& position, vec3& lookAt, vec3& up, float& zNear, float& zFar)
{
	aiCamera* camera;
	string cameraNameTemp;

	uint numCamera = scene->mNumCameras;

	forI(numCamera)
	{
		camera         = scene->mCameras[i];
		cameraNameTemp = string(camera->mName.C_Str());

		if (cameraNameTemp == cameraName)
		{
			position = vec3(camera->mPosition.x, camera->mPosition.y, camera->mPosition.z);
			lookAt   = vec3(camera->mLookAt.x, camera->mLookAt.y, camera->mLookAt.z);
			up       = vec3(camera->mUp.x, camera->mUp.y, camera->mUp.z);
			zNear    = camera->mClipPlaneNear;
			zFar     = camera->mClipPlaneFar;
			return true;
		}
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Model::sceneHasOnlyStaticMeshAnimations(const aiScene* scene)
{
	forI(scene->mNumMeshes)
	{
		if (scene->mMeshes[i]->HasBones())
		{
			return false;
		}
	}

	if (scene->HasAnimations())
	{
		return true;
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Model::processAnimationData(aiAnimation* animation, Node* sceneNode)
{
	float duration       = animation->mDuration;
	float ticksPerSecond = animation->mTicksPerSecond;
	uint numChannels     = animation->mNumChannels;

	if (numChannels != 1)
	{
		// Only a single animation channel is supported
		cout << "ERROR: MORE THAN ONE ANIMATION CHANNEL IN " << animation->mName.C_Str() << " ANIMATION" << endl;
		return;
	}

	aiNodeAnim* nodeAnim = animation->mChannels[0];

	uint numPositionKeys = nodeAnim->mNumPositionKeys;
	uint numRotationKeys = nodeAnim->mNumRotationKeys;
	uint numScalingKeys  = nodeAnim->mNumScalingKeys;

	if ((numPositionKeys != numRotationKeys) || (numPositionKeys != numScalingKeys))
	{
		cout << "ERROR: ANIMATION " << animation->mName.C_Str() << " HAS DATA WITH DIFFERENT VALUES FOR POSITION (" << numPositionKeys << "), ROTATION (" << numRotationKeys << ") OR SCALING (" << numScalingKeys << ")" << endl;
		return;
	}

	uint numKey = numPositionKeys;

	vectorVec3 vectorPositionKey(numKey);
	vectorQuat vectorRotationKey(numKey);
	vectorVec3 vectorScalingKey(numKey);
	vectorFloat vectorKey(numKey);

	// NOTE: Assuming the values in mPositionKeys::mTime match the ones in mRotationKeys::mTime and mScalingKeys::mTime

	forJ(numKey)
	{
		aiVectorKey positionKey = nodeAnim->mPositionKeys[j];
		aiQuatKey rotationKey   = nodeAnim->mRotationKeys[j];
		aiVectorKey scalingKey  = nodeAnim->mScalingKeys[j];
		
		vectorPositionKey[j]    = vec3(positionKey.mValue.x, positionKey.mValue.y, positionKey.mValue.z);
		vectorRotationKey[j]    = quat(rotationKey.mValue.x, rotationKey.mValue.y, rotationKey.mValue.z, rotationKey.mValue.w);
		vectorScalingKey[j]     = vec3(scalingKey.mValue.x, scalingKey.mValue.y, scalingKey.mValue.z);
		vectorKey[j]            = positionKey.mTime;
	}

	DynamicComponent* component = sceneNode->refDynamicComponent();
	component->setKeyInformation(move(vectorPositionKey), move(vectorRotationKey), move(vectorScalingKey), move(vectorKey));
	component->setDuration(int(vectorPositionKey.size() - 1));
	component->setTicksPerSecond(ticksPerSecond);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Model::inorderTreeTraversal(aiNode* baseNode, vector<aiNode*>& vectorTempResult)
{
	vectorTempResult.push_back(baseNode);

	forJ(baseNode->mNumChildren)
	{
		inorderTreeTraversal(baseNode->mChildren[j], vectorTempResult);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Model::fillNodeParentInformation(const vector<aiNode*>&         nodeVector, 
										    vector<BoneInformation>& vectorBoneInformation)
{
	BoneInformation* boneInformation = nullptr;

	forIT(nodeVector)
	{
		aiNode* node    = (*it);
		string nodeName = string(node->mName.C_Str());
		boneInformation = nullptr;

		forI(vectorBoneInformation.size())
		{
			if (vectorBoneInformation[i].m_name == nodeName)
			{
				boneInformation = &vectorBoneInformation[i];
				break;
			}
		}

		if (boneInformation == nullptr)
		{
			cout << "ERROR no element found in Model::fillNodeParentInformation" << endl;
		}
		else
		{
			while (node->mParent != nullptr)
			{
				node             = node->mParent;
				string& nodeName = string(node->mName.C_Str());
				boneInformation->m_parentBone.push_back(nodeName);

				forJ(nodeVector.size())
				{
					aiNode* nodeTemp = nodeVector[j];
					string& nodeTempName = string(nodeTemp->mName.data);

					if (nodeTempName == nodeName)
					{
						boneInformation->m_parentBoneIndex.push_back(j);
						break;
					}
				}
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

Node *Model::processMesh(aiNode* node, aiMesh* mesh, const aiScene* scene, const mat4& transform)
{
	vectorUint indices;
	vectorVec3 vertices;
	vectorVec2 texCoord;
	vectorVec3 normals;
	vectorVec3 tangents;

	vec3 vector3;
	vec2 vector2;

	forI(mesh->mNumVertices)
	{
		vector3 = vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
		vertices.push_back(vector3);

		if (mesh->HasTextureCoords(0)) // only first texture coordinate channel covered now
		{
			vector2 = vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
			texCoord.push_back(vector2);
		}

		if (mesh->HasNormals())
		{
			vector3 = vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
			normals.push_back(vector3);
		}

		if (mesh->HasTangentsAndBitangents())
		{
			vector3 = vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
			tangents.push_back(vector3);
		}
	}

	// Process indices
	aiFace face;
	forI(mesh->mNumFaces)
	{
		face = mesh->mFaces[i];
		forJ(face.mNumIndices)
		{
			indices.push_back(face.mIndices[j]);
		}
	}

	if (indices.size() > 65530)
	{
		// SHOW THE BOUNDIG BOX
		cout << "THERE ARE " << indices.size() << " INDICES IN " << node->mName.C_Str() << endl;
	}

	// If the model comes from a file with coordinate system as 3DS max ("z" up, "x" front and "y" right), then coordinates must be
	// switched (as well as traslation, scale and rotations)
	if (m_XYZToMinusXZY)
	{

	}

	vec3 scale;
	quat rotation;
	vec3 translation;
	vec3 skew;
	vec4 perspective;
	glm::decompose(transform, scale, rotation, translation, skew, perspective);

	// https://stackoverflow.com/questions/17918033/glm-decompose-mat4-into-translation-and-rotation
	// Keep in mind that the resulting quaternion in not correct.It returns its conjugate!
	// To fix this add this to your code :

	rotation = glm::conjugate(rotation);
	mat3 normalMatrix = glm::mat3_cast(rotation);
	
	forI(vertices.size())
	{
		vec3 v3D    = vertices[i];
		vec4 v4D    = transform * vec4(v3D, 1.0f);
		vertices[i] = vec3(v4D.x, v4D.y, v4D.z);

		normals[i]  = normalMatrix * normals[i];
		tangents[i] = normalMatrix * tangents[i];
	}

	string name = to_string(m_modelCounter);
	m_modelCounter++;

	aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
	aiString materialName;
	material->Get(AI_MATKEY_NAME, materialName);
	Material* materialInstance = materialM->getElement(move(string(materialName.C_Str())));
	Material* materialInstanceInstanced = materialM->getElement(move(string(materialName.C_Str()) + "Instanced"));

	if (materialInstance == nullptr)
	{
		cout << "ERROR: material " << materialName.C_Str() << " not found for model " << name << " with asset name " << mesh->mName.C_Str() << ", using DefaultMaterial" << endl;
		materialInstance = materialM->getElement(move(string("DefaultMaterial")));
	}

	if (materialInstanceInstanced == nullptr)
	{
		cout << "ERROR: material " << materialName.C_Str() << "Instanced not found for model " << name << " with asset name " << mesh->mName.C_Str() << ", using DefaultMaterialInstanced" << endl;
		materialInstanceInstanced = materialM->getElement(move(string("DefaultMaterialInstanced")));
	}

	// For the only dynamic scene element, the vertices 18, 19 20 are a triangle (those indices are used together for a triangle) and they are
	// coplanar in the z plane, but the normal direction seems to be (0, -1, 0) which is wrong
	Node* sceneNode = new Node(move(string(name)), move(string(mesh->mName.C_Str())), move(string("Node")), indices, vertices, texCoord, normals, tangents);

	const vector<string>& vectorAnimationDiscardKeywords = sceneM->getAnimationDiscardKeywords();

	string nodeName = string(node->mName.C_Str());
	if ((!anyVectorElementisSubstring(vectorAnimationDiscardKeywords, nodeName)) && (m_mapNodeNameAnimation.find(nodeName) != m_mapNodeNameAnimation.end()))
	{
		// There is animation data to be added for this node, proccess it and give it to the dynamic component of the node
		processAnimationData(m_mapNodeNameAnimation[nodeName], sceneNode);
	}

	// Dynamic scene elements will not be part of the scene merged geometry node
	if (m_makeMergedGeometryNode && !sceneNode->getDynamicComponent()->getKeyframeInformationSet())
	{
		addMergedGeometryNodeInformation(indices, vertices, texCoord, normals, tangents);
	}

	sceneNode->initialize(vec3(0.0f), quat(vec3(.0f)), vec3(1.0f), materialInstance, materialInstanceInstanced, eMeshType::E_MT_RENDER_MODEL, m_initializeComponents);

	return sceneNode;
}

/////////////////////////////////////////////////////////////////////////////////////////////

Node* Model::processSkeletalMesh(aiNode* node, aiMesh* skeletalMesh, const aiScene* scene, const mat4& transform)
{
	// TODO: Refactor with Model::processMesh
	uint numBones = skeletalMesh->mNumBones;

	vector<BoneInformation> vectorBoneInformation;
	map<string, int> mapUsedBoneNameIndex;

	uint numberBones = 0;

	vectorUVec4 vectorVertexBoneID(skeletalMesh->mNumVertices); // Vector with the bone IDs affecting each vertex
	vectorVec4 vectorVertexBoneWeight(skeletalMesh->mNumVertices); // Vector with the bone weights affecting each vertex

	assert(numBones < MAXIMUM_BONES_PER_SKELETAL_MESH);

	string boneName;
	uint boneIndex;
	forI(numBones)
	{
		boneName = skeletalMesh->mBones[i]->mName.data;

		// Find the bone index, caching each new one found
		if(mapUsedBoneNameIndex.find(boneName) == mapUsedBoneNameIndex.end())
		{
			boneIndex = numberBones;
			mapUsedBoneNameIndex.insert(make_pair(boneName, boneIndex));
			numberBones++;
		}
		else
		{
			boneIndex = mapUsedBoneNameIndex[boneName];
		}

		// Assign, for each vertex, the bone contributions this bone being proccessed has
		uint numWeights = skeletalMesh->mBones[i]->mNumWeights;
		forJ(numWeights)
		{
			uint vertexID = skeletalMesh->mBones[i]->mWeights[j].mVertexId;

			bool filledInformation = false;
			for(int k = 0; ((k < MAXIMUM_BONES_PER_VERTEX) && !filledInformation); ++k)
			{
				// Find an empty slot in the current vertex's weight and bone ID information
				if (vectorVertexBoneWeight[vertexID][k] == 0.0f)
				{
					vectorVertexBoneID[vertexID][k]     = boneIndex;
					vectorVertexBoneWeight[vertexID][k] = skeletalMesh->mBones[i]->mWeights[j].mWeight;
					filledInformation                   = true;
				}
			}
		}
	}

	// Put in the vector below all the nodes used in the skinned mesh following a breadth-first traversal
	vector<aiNode*> vectorTempResult;
	inorderTreeTraversal(scene->mRootNode, vectorTempResult);

	forI(vectorTempResult.size())
	{
		aiNode* node = vectorTempResult[i];

		mat4 offsetMatrix;
		string boneName = string(node->mName.data);

		if (mapUsedBoneNameIndex.find(boneName) != mapUsedBoneNameIndex.end())
		{
			int usedBoneIndex = mapUsedBoneNameIndex[boneName];
			offsetMatrix      = mat4AssimpCast(skeletalMesh->mBones[usedBoneIndex]->mOffsetMatrix);
		}
		vectorBoneInformation.push_back(BoneInformation(move(string(boneName)), i, offsetMatrix));
	}

	fillNodeParentInformation(vectorTempResult, vectorBoneInformation);

	// TODO: Pending to implement a more general case where more than one animation is loaded
	string animationName = scene->mAnimations[0]->mName.C_Str();

	if (animationName == "")
	{
		animationName = "animation_" + to_string(m_skeletalAnimationCounter);
		m_skeletalAnimationCounter++;
	}

	SkeletalAnimation* skeletalAnimation = skeletalAnimationM->buildAnimation(move(animationName), scene->mAnimations[0]);

	// Now, to know for each bone what matrices it has hierarchical dependency for updating its corresponding matrix, for each element in 
	// vectorTempResult trace back to the root node all the nodes and store their name and index from mapNameBoneInformation

	// Build Vertex buffer information
	vectorUint indices;
	vectorVec3 vertices;
	vectorVec2 texCoord;
	vectorVec3 normals;
	vectorVec3 tangents;
	vectorVec4 boneWeight;
	vectorUVec4 boneID;

	mat3 normalMatrix = mat3(transpose(inverse(transform)));
	vec4 vertex;
	vec4 transformedVertex;
	vec3 normal;
	vec3 tangent;
	vec3 transformedNormal;
	vec3 transformedTangent;
	
	forI(skeletalMesh->mNumVertices)
	{
		normal             = vec3AssimpCast(skeletalMesh->mNormals[i]);
		tangent            = vec3AssimpCast(skeletalMesh->mTangents[i]);
		vertex             = vec4(vec3AssimpCast(skeletalMesh->mVertices[i]), 1.0);
		transformedVertex  = transform * vertex;
		transformedNormal  = normal;
		transformedTangent = tangent;

		vertices.push_back(vec3(transformedVertex.x, transformedVertex.y, transformedVertex.z));
		normals.push_back(transformedNormal);
		tangents.push_back(transformedTangent);
		vec2 temp = vec2(skeletalMesh->mTextureCoords[0][i].x, skeletalMesh->mTextureCoords[0][i].y);
		texCoord.push_back(temp);
		boneWeight.push_back(vectorVertexBoneWeight[i]);
		boneID.push_back(vectorVertexBoneID[i]);
	}

	forI(skeletalMesh->mNumFaces)
	{
		indices.push_back(skeletalMesh->mFaces[i].mIndices[0]);
		indices.push_back(skeletalMesh->mFaces[i].mIndices[1]);
		indices.push_back(skeletalMesh->mFaces[i].mIndices[2]);
	}

	string name = to_string(m_modelCounter);
	m_modelCounter++;

	aiMaterial* material = scene->mMaterials[skeletalMesh->mMaterialIndex];
	aiString materialName;
	material->Get(AI_MATKEY_NAME, materialName);
	Material* materialInstance = materialM->getElement(move(string(materialName.C_Str())));
	Material* materialInstanceInstanced = materialM->getElement(move(string(materialName.C_Str()) + "Instanced"));

	if (materialInstance == nullptr)
	{
		cout << "ERROR: material " << materialName.C_Str() << " not found for model " << name << ", using DefaultMaterial" << endl;
		materialInstance = materialM->getElement(move(string("DefaultMaterial")));
	}

	if (materialInstanceInstanced == nullptr)
	{
		cout << "ERROR: material " << materialName.C_Str() << "Instanced not found for model " << name << ", using DefaultMaterialInstanced" << endl;
		materialInstanceInstanced = materialM->getElement(move(string("DefaultMaterialInstanced")));
	}

	mat4 skeletalMeshParentTransform = mat4AssimpCast(vectorTempResult[0]->mTransformation);

	Node* sceneNode = new Node(move(string(name)), 
							   move(string(skeletalMesh->mName.C_Str())),
		                       move(string("Node")), 
		                       move(indices),
		                       move(vertices), 
		                       move(texCoord), 
		                       move(normals), 
		                       move(tangents), 
		                       move(boneWeight), 
		                       move(boneID), 
		                       move(mapUsedBoneNameIndex),
		                       move(vectorBoneInformation),
							   move(skeletalMeshParentTransform),
		                       move(mat4(transform)));

	const vector<string>& vectorAnimationDiscardKeywords = sceneM->getAnimationDiscardKeywords();

	string nodeName = string(node->mName.C_Str());
	if ((!anyVectorElementisSubstring(vectorAnimationDiscardKeywords, nodeName)) && (m_mapNodeNameAnimation.find(nodeName) != m_mapNodeNameAnimation.end()))
	{
		// There is animation data to be added for this node, proccess it and give it to the dynamic component of the node
		processAnimationData(m_mapNodeNameAnimation[nodeName], sceneNode);
	}

	SkinnedMeshRenderComponent* skinnedMeshRenderComponent = static_cast<SkinnedMeshRenderComponent*>(sceneNode->refRenderComponent());

	skinnedMeshRenderComponent->setSkeletalAnimation(skeletalAnimation);

	sceneNode->initialize(vec3(0.0f), quat(vec3(.0f)), vec3(1.0f), materialInstance, materialInstanceInstanced, eMeshType::E_MT_RENDER_MODEL, m_initializeComponents);

	return sceneNode;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Model::addMeshesToScene()
{
	forIT(m_vecMesh)
	{
		sceneM->addModel(*it);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Model::setRotationToNodes(quat rotation)
{
	forIT(m_vecMesh)
	{
		(*it)->refTransform().setRotation(rotation);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Model::setTranslationToNodes(vec3 translation)
{
	forIT(m_vecMesh)
	{
		(*it)->refTransform().setTraslation(translation);

		// TODO: This is not how it shoould be done, pending proper implementation
		if ((*it)->getNodeType() == eNodeType::E_NT_DYNAMIC_ELEMENT)
		{
			DynamicComponent* component   = (*it)->refDynamicComponent();
			vectorVec3& vectorPositionKey = component->refVectorPositionKey();
			forJT(vectorPositionKey)
			{
				(*jt) += translation;
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Model::addTranslationToNodes(vec3 translation)
{
	forIT(m_vecMesh)
	{
		(*it)->refTransform().addTraslation(translation);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Model::setScalingToNodes(vec3 scaling)
{
	forIT(m_vecMesh)
	{
		(*it)->refTransform().setScaling(scaling);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

Node* Model::getCompactedGeometryNode(string&& nodeName)
{
	Node* mergedGeometryNode = nullptr;
	if (m_makeMergedGeometryNode)
	{
		for(int i = 0; i < m_mergedIndex.size(); ++i)
		{
			vec3 temp = m_mergedVertex[m_mergedIndex[i]];
		}
		mergedGeometryNode = new Node(move(nodeName), move(string("CompactedGeometryNode")), move(string("Node")), m_mergedIndex, m_mergedVertex, m_mergedTexCoord, m_mergedNormal, m_mergedTangent);
		mergedGeometryNode->initialize(vec3(0.0f), quat(vec3(0.0f)), vec3(1.0f), materialM->getElement(move(string("DefaultMaterial"))), materialM->getElement(move(string("DefaultMaterialInstanced"))), eMeshType::E_MT_RENDER_SHADOW, m_initializeComponents);
	}

	return mergedGeometryNode;
}

/////////////////////////////////////////////////////////////////////////////////////////////
