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
#include "../../include/scene/scene.h"
#include "../../include/util/containerutilities.h"
#include "../../include/util/singleton.h"
#include "../../include/model/model.h"
#include "../../include/parameter/attributedefines.h"
#include "../../include/parameter/attributedata.h"
#include "../../include/camera/camera.h"
#include "../../include/camera/cameramanager.h"
#include "../../include/shader/shadermanager.h"
#include "../../include/core/coremanager.h"
#include "../../include/material/material.h"
#include "../../include/component/basecomponent.h"
#include "../../include/component/componentmanager.h"
#include "../../include/util/bufferverificationhelper.h"

// NAMESPACE
using namespace attributedefines;

// DEFINES

// STATIC MEMBER INITIALIZATION
string Scene::m_scenePath = "../data/scenes/bistro_v5_1/";
string Scene::m_sceneName = "exterior_scaled_3_tunnel_cover_cut.obj";
vectorString Scene::m_transparentKeywords = { "curtains", "foliage" };
vectorString Scene::m_animationDiscardKeywords = { "Bistro_Research_Exterior", "Lantern_Wind", "StringLight_Wind" };
vectorString Scene::m_modelDiscardKeywords = { };

/////////////////////////////////////////////////////////////////////////////////////////////

Scene::Scene() :
	m_deltaTime(.0f)
	, m_sceneCamera(nullptr)
	, m_staticSceneCompactedGeometry(nullptr)
	, m_isVectorNodeFrameUpdated(false)
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

Scene::~Scene()
{
	// Delete scene models
	deleteVectorInstances(m_vectorNode);
	m_vectorNode.clear();

	// Delete cameras
	deleteVectorInstances(m_vectorCamera);
	m_vectorCamera.clear();

	// Delete scene components
	forI(m_vectorSceneComponent.size())
	{
		componentM->removeElement(move(string(m_vectorSceneComponent[i]->getName())));
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

vec3 convRGB8ToVec(uint val)
{
	return vec3(float((val & 0x000000FF)), float((val & 0x0000FF00) >> 8U), float((val & 0x00FF0000) >> 16U)); // TODO: Review offset, might be wrong and needs to be fixed at vertex level
}

/////////////////////////////////////////////////////////////////////////////////////////////

uint convVecToRGB8(vec3 val)
{
	uint result = (uint(val.x) | (uint(val.y) << 8) | (uint(val.z) << 16));
	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Scene::init()
{
	gpuPipelineM->addRasterFlag(move(string("EMITTER_RADIANCE")), 50);
	gpuPipelineM->addRasterFlag(move(string("LIT_VOXEL_MIN_COORDINATE_X")), 0);
	gpuPipelineM->addRasterFlag(move(string("LIT_VOXEL_MIN_COORDINATE_Y")), 0);
	gpuPipelineM->addRasterFlag(move(string("LIT_VOXEL_MIN_COORDINATE_Z")), 0);
	gpuPipelineM->addRasterFlag(move(string("LIT_VOXEL_MAX_COORDINATE_X")), 127);
	gpuPipelineM->addRasterFlag(move(string("LIT_VOXEL_MAX_COORDINATE_Y")), 127);
	gpuPipelineM->addRasterFlag(move(string("LIT_VOXEL_MAX_COORDINATE_Z")), 127);
	gpuPipelineM->addRasterFlag(move(string("LIT_VOXEL_ADD_BOUNDARIES")), 1);
	gpuPipelineM->addRasterFlag(move(string("SCENE_VOXELIZATION_RESOLUTION")), 128);
	gpuPipelineM->addRasterFlag(move(string("IRRADIANCE_MULTIPLIER")), 14000); // This value is divided by 100000 in the shader
	gpuPipelineM->addRasterFlag(move(string("DYNAMIC_IRRADIANCE_MULTIPLIER")), 407000); // This value is divided by 100000 in the shader
	gpuPipelineM->addRasterFlag(move(string("FORM_FACTOR_VOXEL_TO_VOXEL_ADDED")), 1000); // This value is divided by 10 in the shader
	gpuPipelineM->addRasterFlag(move(string("DIRECT_IRRADIANCE_MULTIPLIER")), 700); // This value is divided by 1000 in the shader
	gpuPipelineM->addRasterFlag(move(string("MAX_IRRADIANCE_STATIC_TO_STATIC_VECTOR_LENGTH")), 10000); // This value is divided by 100 in the shader
	gpuPipelineM->addRasterFlag(move(string("MAX_IRRADIANCE_DYNAMIC_TO_STATIC_VECTOR_LENGTH")), 10000); // This value is divided by 100 in the shader
	gpuPipelineM->addRasterFlag(move(string("MAX_IRRADIANCE_ALL_TO_DYNAMIC_VECTOR_LENGTH")), 2000); // This value is divided by 100 in the shader
	gpuPipelineM->addRasterFlag(move(string("THRESHOLD_DISTANCE_VOXEL_MULTIPLIER")), 2);
	gpuPipelineM->addRasterFlag(move(string("ANALYSE_POSSIBLE_NEIGHBOUR_LIT_VOXEL")), 1);
	gpuPipelineM->addRasterFlag(move(string("DYNAMIC_VOXEL_RT_TMIN_MULTIPLIER")), 50); // This value is divided by 100 in the shader
	gpuPipelineM->addRasterFlag(move(string("ADD_EXTRA_VALUE_FORM_FACTOR")), 1); // If set, in the light bounce step, in the differentialAreaFormFactor call, some extra weight will be added in the denominator to avoid the form factor formula, which can reach really big values for close points, from reaching those high values
	gpuPipelineM->addRasterFlag(move(string("USE_FULL_NEIGHBOUR_SAMPLING")), 1);
	gpuPipelineM->addRasterFlag(move(string("MIN_SQUARED_DISTANCE_FORM_FACTOR_STATIC")), 100); // Value divided by 10.0 in the shader
	gpuPipelineM->addRasterFlag(move(string("MIN_SQUARED_DISTANCE_FORM_FACTOR_DYNAMIC")), 1); // Value divided by 10.0 in the shader

	// Scene raster settings

	gpuPipelineM->addRasterFlag(move(string("TILE_SIZE_TOTAL")), 8); // Size of the tiles for the padding, currently 2^3 = 8 voxels
	gpuPipelineM->addRasterFlag(move(string("TILE_SIDE")), 2); // Size of the tiles for the padding, currently 2^3 = 8 voxels
	gpuPipelineM->addRasterFlag(move(string("VERTEX_STRIDE")), gpuPipelineM->getVertexStrideBytes() / 4); // Vertex format stride
	gpuPipelineM->addRasterFlag(move(string("MAXIMUM_BONES_PER_SKELETAL_MESH")), MAXIMUM_BONES_PER_SKELETAL_MESH); // Vertex format stride
	gpuPipelineM->addRasterFlag(move(string("MAX_DYNAMIC_RT_RAY_LENGTH")), 10); // Max length of the rays traced in DynamicLightBounceVoxelIrradianceTechnique
	gpuPipelineM->addRasterFlag(move(string("COOL_DOWN_TIME_MILISECONDS")), 300); // Col down for temporal filtering in miliseconds
 
	shaderM->addGlobalHeaderSourceCode(move(string("#define TILE_SIZE_TOTAL " + to_string(gpuPipelineM->getRasterFlagValue(move(string("TILE_SIZE_TOTAL")))) + "\n")));
	shaderM->addGlobalHeaderSourceCode(move(string("#define TILE_SIDE " + to_string(gpuPipelineM->getRasterFlagValue(move(string("TILE_SIDE")))) + "\n")));

	shaderM->addGlobalHeaderSourceCode(move(string("/////////////////////////////////////////////////////////////\n")));
	shaderM->addGlobalHeaderSourceCode(move(string("// GLOBAL DEFINES\n")));
	shaderM->addGlobalHeaderSourceCode(move(string("#define NUM_ELEMENT_GATHERING_ARRAY 128\n")));
	shaderM->addGlobalHeaderSourceCode(move(string("#define NUM_ELEMENT_PER_OCTANT 10\n")));
	shaderM->addGlobalHeaderSourceCode(move(string("#define NUM_SPHERE_SAMPLE 1024\n")));
	shaderM->addGlobalHeaderSourceCode(move(string("#define NUM_SPHERICAL_COEFFICIENTS 9\n")));

	if (gpuPipelineM->getRasterFlagValue(move(string("LIT_VOXEL_TEST_VOXEL_TO_LIGHT_DIRECTION"))) == 1)
	{
		shaderM->addGlobalHeaderSourceCode(move(string("#define LIT_VOXEL_TEST_VOXEL_TO_LIGHT_DIRECTION 1\n")));
	}
	if (gpuPipelineM->getRasterFlagValue(move(string("AVOID_VOXEL_FACE_PENALTY"))) == 1)
	{
		shaderM->addGlobalHeaderSourceCode(move(string("#define AVOID_VOXEL_FACE_PENALTY 1\n")));
	}
	if (gpuPipelineM->getRasterFlagValue(move(string("LIT_VOXEL_ADD_BOUNDARIES"))) == 1)
	{
		shaderM->addGlobalHeaderSourceCode(move(string("#define LIT_VOXEL_MIN_COORDINATE_X " + to_string(gpuPipelineM->getRasterFlagValue(move(string("LIT_VOXEL_MIN_COORDINATE_X")))) + "\n")));
		shaderM->addGlobalHeaderSourceCode(move(string("#define LIT_VOXEL_MIN_COORDINATE_Y " + to_string(gpuPipelineM->getRasterFlagValue(move(string("LIT_VOXEL_MIN_COORDINATE_Y")))) + "\n")));
		shaderM->addGlobalHeaderSourceCode(move(string("#define LIT_VOXEL_MIN_COORDINATE_Z " + to_string(gpuPipelineM->getRasterFlagValue(move(string("LIT_VOXEL_MIN_COORDINATE_Z")))) + "\n")));
		shaderM->addGlobalHeaderSourceCode(move(string("#define LIT_VOXEL_MAX_COORDINATE_X " + to_string(gpuPipelineM->getRasterFlagValue(move(string("LIT_VOXEL_MAX_COORDINATE_X")))) + "\n")));
		shaderM->addGlobalHeaderSourceCode(move(string("#define LIT_VOXEL_MAX_COORDINATE_Y " + to_string(gpuPipelineM->getRasterFlagValue(move(string("LIT_VOXEL_MAX_COORDINATE_Y")))) + "\n")));
		shaderM->addGlobalHeaderSourceCode(move(string("#define LIT_VOXEL_MAX_COORDINATE_Z " + to_string(gpuPipelineM->getRasterFlagValue(move(string("LIT_VOXEL_MAX_COORDINATE_Z")))) + "\n")));
	}

	shaderM->addGlobalHeaderSourceCode(move(string("#define FIND_HASHED_POSITION_NUM_ITERATION 8\n")));
	shaderM->addGlobalHeaderSourceCode(move(string("#define FORM_FACTOR_VOXEL_TO_VOXEL_ADDED " + to_string(gpuPipelineM->getRasterFlagValue(move(string("FORM_FACTOR_VOXEL_TO_VOXEL_ADDED")))) + "\n")));
	shaderM->addGlobalHeaderSourceCode(move(string("#define IRRADIANCE_MULTIPLIER " + to_string(gpuPipelineM->getRasterFlagValue(move(string("IRRADIANCE_MULTIPLIER")))) + "\n")));
	shaderM->addGlobalHeaderSourceCode(move(string("#define DYNAMIC_IRRADIANCE_MULTIPLIER " + to_string(gpuPipelineM->getRasterFlagValue(move(string("DYNAMIC_IRRADIANCE_MULTIPLIER")))) + "\n")));
	shaderM->addGlobalHeaderSourceCode(move(string("#define DIRECT_IRRADIANCE_MULTIPLIER " + to_string(gpuPipelineM->getRasterFlagValue(move(string("DIRECT_IRRADIANCE_MULTIPLIER")))) + "\n")));
	shaderM->addGlobalHeaderSourceCode(move(string("#define IRRADIANCE_FIELD_GRADIENT_OFFSET 0.1\n")));
	shaderM->addGlobalHeaderSourceCode(move(string("#define VERTEX_STRIDE " + to_string(gpuPipelineM->getRasterFlagValue(move(string("VERTEX_STRIDE")))) + "\n")));
	shaderM->addGlobalHeaderSourceCode(move(string("#define MAXIMUM_BONES_PER_SKELETAL_MESH " + to_string(gpuPipelineM->getRasterFlagValue(move(string("MAXIMUM_BONES_PER_SKELETAL_MESH")))) + "\n")));

	if (gpuPipelineM->getRasterFlagValue(move(string("LIT_VOXEL_CORNER_POINT_MULTIPLIER"))) > 0)
	{
		shaderM->addGlobalHeaderSourceCode(move(string("#define LIT_VOXEL_CORNER_POINT_MULTIPLIER " + to_string(gpuPipelineM->getRasterFlagValue(move(string("LIT_VOXEL_CORNER_POINT_MULTIPLIER")))) + "\n")));
	}

	if (gpuPipelineM->getRasterFlagValue(move(string("LIT_VOXEL_MID_EDGE_POINT_MULTIPLIER"))) > 0)
	{
		shaderM->addGlobalHeaderSourceCode(move(string("#define LIT_VOXEL_MID_EDGE_POINT_MULTIPLIER " + to_string(gpuPipelineM->getRasterFlagValue(move(string("LIT_VOXEL_MID_EDGE_POINT_MULTIPLIER")))) + "\n")));
	}

	if (gpuPipelineM->getRasterFlagValue(move(string("LIMIT_VOXEL_FACE_MAX_IRRADIANCE"))) == 1)
	{
		shaderM->addGlobalHeaderSourceCode(move(string("#define LIMIT_VOXEL_FACE_MAX_IRRADIANCE 1\n")));
	}

	if (gpuPipelineM->getRasterFlagValue(move(string("MAX_IRRADIANCE_STATIC_TO_STATIC_VECTOR_LENGTH"))) >= 0)
	{
		shaderM->addGlobalHeaderSourceCode(move(string("#define MAX_IRRADIANCE_STATIC_TO_STATIC_VECTOR_LENGTH " + to_string(gpuPipelineM->getRasterFlagValue(move(string("MAX_IRRADIANCE_STATIC_TO_STATIC_VECTOR_LENGTH")))) + "\n")));
	}

	if (gpuPipelineM->getRasterFlagValue(move(string("MAX_IRRADIANCE_DYNAMIC_TO_STATIC_VECTOR_LENGTH"))) >= 0)
	{
		shaderM->addGlobalHeaderSourceCode(move(string("#define MAX_IRRADIANCE_DYNAMIC_TO_STATIC_VECTOR_LENGTH " + to_string(gpuPipelineM->getRasterFlagValue(move(string("MAX_IRRADIANCE_DYNAMIC_TO_STATIC_VECTOR_LENGTH")))) + "\n")));
	}

	if (gpuPipelineM->getRasterFlagValue(move(string("MAX_IRRADIANCE_ALL_TO_DYNAMIC_VECTOR_LENGTH"))) >= 0)
	{
		shaderM->addGlobalHeaderSourceCode(move(string("#define MAX_IRRADIANCE_ALL_TO_DYNAMIC_VECTOR_LENGTH " + to_string(gpuPipelineM->getRasterFlagValue(move(string("MAX_IRRADIANCE_ALL_TO_DYNAMIC_VECTOR_LENGTH")))) + "\n")));
	}

	if (gpuPipelineM->getRasterFlagValue(move(string("THRESHOLD_DISTANCE_VOXEL_MULTIPLIER"))) >= 0)
	{
		shaderM->addGlobalHeaderSourceCode(move(string("#define THRESHOLD_DISTANCE_VOXEL_MULTIPLIER " + to_string(gpuPipelineM->getRasterFlagValue(move(string("THRESHOLD_DISTANCE_VOXEL_MULTIPLIER")))) + "\n")));
	}

	if (gpuPipelineM->getRasterFlagValue(move(string("ANALYSE_POSSIBLE_NEIGHBOUR_LIT_VOXEL"))) > 0)
	{
		shaderM->addGlobalHeaderSourceCode(move(string("#define ANALYSE_POSSIBLE_NEIGHBOUR_LIT_VOXEL " + to_string(gpuPipelineM->getRasterFlagValue(move(string("ANALYSE_POSSIBLE_NEIGHBOUR_LIT_VOXEL")))) + "\n")));
	}

	if (gpuPipelineM->getRasterFlagValue(move(string("DYNAMIC_VOXEL_USE_MEAN_DIRECTION_IRRADIANCE"))) > 0)
	{
		shaderM->addGlobalHeaderSourceCode(move(string("#define DYNAMIC_VOXEL_USE_MEAN_DIRECTION_IRRADIANCE " + to_string(gpuPipelineM->getRasterFlagValue(move(string("DYNAMIC_VOXEL_USE_MEAN_DIRECTION_IRRADIANCE")))) + "\n")));
	}

	if (gpuPipelineM->getRasterFlagValue(move(string("DYNAMIC_VOXEL_RT_TMIN_MULTIPLIER"))) > 0)
	{
		shaderM->addGlobalHeaderSourceCode(move(string("#define DYNAMIC_VOXEL_RT_TMIN_MULTIPLIER " + to_string(gpuPipelineM->getRasterFlagValue(move(string("DYNAMIC_VOXEL_RT_TMIN_MULTIPLIER")))) + "\n")));
	}

	if (gpuPipelineM->getRasterFlagValue(move(string("USE_TRIANGULATED_INFO_AND_FULL_NEIGHBOUR_SAMPLING"))) > 0)
	{
		shaderM->addGlobalHeaderSourceCode(move(string("#define USE_TRIANGULATED_INFO_AND_FULL_NEIGHBOUR_SAMPLING " + to_string(gpuPipelineM->getRasterFlagValue(move(string("USE_TRIANGULATED_INFO_AND_FULL_NEIGHBOUR_SAMPLING")))) + "\n")));
	}

	if (gpuPipelineM->getRasterFlagValue(move(string("ADD_EXTRA_VALUE_FORM_FACTOR"))) >= 0)
	{
		shaderM->addGlobalHeaderSourceCode(move(string("#define ADD_EXTRA_VALUE_FORM_FACTOR " + to_string(gpuPipelineM->getRasterFlagValue(move(string("ADD_EXTRA_VALUE_FORM_FACTOR")))) + "\n")));
	}

	if (gpuPipelineM->getRasterFlagValue(move(string("ADD_EXTRA_TRIANGULATED_POINTS"))) >= 0)
	{
		shaderM->addGlobalHeaderSourceCode(move(string("#define ADD_EXTRA_TRIANGULATED_POINTS " + to_string(gpuPipelineM->getRasterFlagValue(move(string("ADD_EXTRA_TRIANGULATED_POINTS")))) + "\n")));
	}

	if (gpuPipelineM->getRasterFlagValue(move(string("USE_FULL_NEIGHBOUR_SAMPLING"))) >= 0)
	{
		shaderM->addGlobalHeaderSourceCode(move(string("#define USE_FULL_NEIGHBOUR_SAMPLING " + to_string(gpuPipelineM->getRasterFlagValue(move(string("USE_FULL_NEIGHBOUR_SAMPLING")))) + "\n")));
	}

	if (gpuPipelineM->getRasterFlagValue(move(string("MAX_DYNAMIC_RT_RAY_LENGTH"))) > 0)
	{
		shaderM->addGlobalHeaderSourceCode(move(string("#define MAX_DYNAMIC_RT_RAY_LENGTH " + to_string(gpuPipelineM->getRasterFlagValue(move(string("MAX_DYNAMIC_RT_RAY_LENGTH")))) + "\n")));
	}

	if (gpuPipelineM->getRasterFlagValue(move(string("MIN_SQUARED_DISTANCE_FORM_FACTOR_STATIC"))) > 0)
	{
		shaderM->addGlobalHeaderSourceCode(move(string("#define MIN_SQUARED_DISTANCE_FORM_FACTOR_STATIC " + to_string(gpuPipelineM->getRasterFlagValue(move(string("MIN_SQUARED_DISTANCE_FORM_FACTOR_STATIC")))) + "\n")));
	}

	if (gpuPipelineM->getRasterFlagValue(move(string("MIN_SQUARED_DISTANCE_FORM_FACTOR_DYNAMIC"))) > 0)
	{
		shaderM->addGlobalHeaderSourceCode(move(string("#define MIN_SQUARED_DISTANCE_FORM_FACTOR_DYNAMIC " + to_string(gpuPipelineM->getRasterFlagValue(move(string("MIN_SQUARED_DISTANCE_FORM_FACTOR_DYNAMIC")))) + "\n")));
	}

	shaderM->addGlobalHeaderSourceCode(move(string("/////////////////////////////////////////////////////////////\n\n")));

	gpuPipelineM->preSceneLoadResources();

	Model* model = new Model(m_scenePath, m_sceneName, true, false, move(mat4(1.0f)), true);
	model->addMeshesToScene();
	delete model;
	
	int numElement = 10;
	
	vectorVec3 vectorPosition(numElement);
	vectorQuat vectorRotation(numElement);
	vectorVec3 vectorScaling(numElement);
	vectorFloat vectorTime(numElement);
	float step = 1.0f / float(numElement);
	
	model = new Model("../data/scenes/Stanford_dragon_manual/", "dragon_uv_manual_add_2.obj", true, false, move(mat4(1.0f)), false);
	vec3 startPoint = vec3( -5.05085, 6.75, 12.316);
	vec3 endPoint   = vec3( 16.7492,  6.75, 18.0161);

	// For testing for this case
	//vec3 startPoint = vec3(8.70f, 6.05, 15.926);
	//vec3 endPoint   = vec3(8.71f, 6.05, 15.926);

	quat rotation   = quat(vec3(0.0f, -0.2f * pi<float>(), 0.0f));
	
	forI(numElement)
	{
		float offset      = float(i) / numElement;
		vectorPosition[i] = startPoint * (1.0f - offset) + offset * endPoint;
		vectorRotation[i] = rotation;
		vectorScaling[i]  = vec3(9.0f);
		vectorTime[i]     = float(i);
	}
	
	Node* node = model->refVecMesh()[0];
	DynamicComponent* component = node->refDynamicComponent();

	component->setKeyInformation(move(vectorPosition), move(vectorRotation), move(vectorScaling), move(vectorTime));
	component->setDuration(float(numElement - 1));
	component->setTicksPerSecond(1.0f);
	node->updateNodeType();
	node->initializeComponents();
	model->addMeshesToScene();

	delete model;
	
	// Force update of aabb to have the data ready for the instance of SceneVoxelizationTechnique
	update();

	m_sceneCamera = cameraM->getElement(move(string("maincamera")));

	if (m_sceneCamera == nullptr)
	{
		m_sceneCamera = cameraM->buildCamera("maincamera",
											 CameraType::CT_FIRST_PERSON,
											 vec3(-29.7679996f, 15.6282482f, -13.8280325f),
											 m_aabb.getCenter(),
											 vec3(0.0f, 1.0, 0.0f),
											 ZNEAR,
											 ZFAR,
											 glm::pi<float>() * 0.25f);
	}

	Camera* sceneCamera = cameraM->buildCamera(move(string("emitter")),
		CameraType::CT_FIRST_PERSON,
		vec3(-73.0466995f, 87.3924103f, 77.4063263f),
		vec3(0.0f, 1.0f, 0.0f),
		vec3(0.0f, 1.0f, 0.0f),
		ZNEAR,
		ZFAR,
		glm::pi<float>() * 0.25f);

	cameraM->setAsMainCamera(m_sceneCamera);
	cameraM->loadCameraRecordingData();

	BaseComponent* baseComponent = componentM->buildComponent(move(string("SceneElementControlComponent")), GenericResourceType::GRT_SCENEELEMENTCONTROLCOMPONENT);
	baseComponent->init();
	m_vectorSceneComponent.push_back(baseComponent);

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Scene::update()
{
	m_vectorNodeFrameUpdated.clear();
	m_vectorNodeIndexFrameUpdated.clear();

	forI(m_vectorNode.size())
	{
		if (m_vectorNode[i]->prepare(m_deltaTime))
		{
			m_vectorNodeFrameUpdated.push_back(m_vectorNode[i]);
			m_vectorNodeIndexFrameUpdated.push_back(i);
		}
	}

	if (m_aabb.getDirty())
	{
		updateBB();
		m_aabb.setDirty(false);
	}

	cameraM->updateMovementState(m_deltaTime, 0.5f);

	if (m_vectorNodeFrameUpdated.size() > 0)
	{
		m_isVectorNodeFrameUpdated = true;
		m_signalSceneDirtyNotification.emit();
	}
	else
	{
		m_isVectorNodeFrameUpdated = false;
	}

	m_signalSceneDirtyNotification.emit();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Scene::shutdown()
{
	this->~Scene();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Scene::addModel(Node* node)
{
	bool result = addIfNoPresent(move(string(node->getName())), node, m_mapStringNode);

	BBox3D box = node->getBBox();

	assert(result);

	if (!result)
	{
		cout << "ERROR: in Scene::addModel, there's another node with the same name." << endl;
		return;
	}

	m_vectorNode.push_back(node);

	if (!node->refAffectSceneBB())
	{
		return;
	}

	if (m_aabb.getMin() == m_aabb.getMax())
	{
		m_aabb = node->getBBox(); // first bb
	}
	else
	{
		m_aabb.extend(node->getBBox());
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Scene::updateBB()
{
	m_aabb = BBox3D::computeFromNodeVector(m_vectorNode);
}

/////////////////////////////////////////////////////////////////////////////////////////////

Node* Scene::refElementByName(string&& name)
{
	return getByKey(move(name), m_mapStringNode);
}

/////////////////////////////////////////////////////////////////////////////////////////////

const Node* Scene::getElementByName(string&& name)
{
	return getByKey(move(name), m_mapStringNode);
}

/////////////////////////////////////////////////////////////////////////////////////////////

int Scene::getElementIndexByName(string&& name)
{
	return findIndexByNameMethodPtr(m_vectorNode, move(name));
}

/////////////////////////////////////////////////////////////////////////////////////////////

int Scene::getElementIndex(Node* node)
{
	return findElementIndex(m_vectorNode, node);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Scene::sortByMaterial(vectorNodePtr& vectorNode)
{
	sort(vectorNode.begin(), vectorNode.end(), 
		[](const Node* node1, const Node* node2)
		{
			const Material* material1 = node1->getRenderComponent()->getMaterial();
			const Material* material2 = node2->getRenderComponent()->getMaterial();
			assert(material1 != nullptr);
			assert(material2 != nullptr);
			return (material1->getHashedName() < material2->getHashedName());
		});
}

/////////////////////////////////////////////////////////////////////////////////////////////

vectorNodePtr Scene::getByMeshType(eMeshType meshType)
{
	vectorNodePtr vectorResult;
	uint maxIndex = uint(m_vectorNode.size());

	forI(maxIndex)
	{
		if (m_vectorNode[i]->refRenderComponent()->getMeshType() & meshType)
		{
			vectorResult.push_back(m_vectorNode[i]);
		}
	}

	return vectorResult;
}

/////////////////////////////////////////////////////////////////////////////////////////////

vectorNodePtr Scene::getByNodeType(eNodeType nodeType)
{
	vectorNodePtr vectorResult;
	uint maxIndex = uint(m_vectorNode.size());

	forI(maxIndex)
	{
		if (m_vectorNode[i]->getNodeType() & nodeType)
		{
			vectorResult.push_back(m_vectorNode[i]);
		}
	}

	return vectorResult;
}

/////////////////////////////////////////////////////////////////////////////////////////////

vectorNodePtr Scene::getByComponentType(GenericResourceType componentType)
{
	vectorNodePtr vectorResult;
	uint maxIndex = uint(m_vectorNode.size());

	switch (componentType)
	{
		case resourceenum::GenericResourceType::GRT_RENDERCOMPONENT:
		{
			forI(maxIndex)
			{
				if ((m_vectorNode[i]->getRenderComponent() != nullptr) && (m_vectorNode[i]->getRenderComponent()->getResourceType() == GenericResourceType::GRT_RENDERCOMPONENT))
				{
					vectorResult.push_back(m_vectorNode[i]);
				}
			}

			break;
		}
		case resourceenum::GenericResourceType::GRT_SKINNEDMESHRENDERCOMPONENT:
		{
			forI(maxIndex)
			{
				if ((m_vectorNode[i]->getRenderComponent() != nullptr) && (m_vectorNode[i]->getRenderComponent()->getResourceType() == GenericResourceType::GRT_SKINNEDMESHRENDERCOMPONENT))
				{
					vectorResult.push_back(m_vectorNode[i]);
				}
			}

			break;
		}
		case resourceenum::GenericResourceType::GRT_DYNAMICCOMPONENT:
		{
			forI(maxIndex)
			{
				if (m_vectorNode[i]->getDynamicComponent() != nullptr)
				{
					vectorResult.push_back(m_vectorNode[i]);
				}
			}

			break;
		}
		default:
		{
			forI(maxIndex)
			{
				vectorBaseComponentPtr& vectorComponent = m_vectorNode[i]->refVectorComponent();
				int numElement = int(vectorComponent.size());
				forJ(numElement)
				{
					if (vectorComponent[j]->getResourceType() == componentType)
					{
						vectorResult.push_back(m_vectorNode[i]);
						break;
					}
				}
			}
			break;
		}
	}

	return vectorResult;
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Scene::addCamera(Camera* cameraParameter)
{
	return addIfNoPresent(cameraParameter, m_vectorCamera);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Scene::addWholeSceneOffset(vec3 offset)
{
	uint maxIndex = uint(m_vectorNode.size());

	forI(maxIndex)
	{
		m_vectorNode[i]->refTransform().addTraslation(offset);
		m_vectorNode[i]->refTransform().setDirty(false);

		BBox3D& box = m_vectorNode[i]->refBBox();
		box.setMin(box.getMin() + offset);
		box.setMax(box.getMax() + offset);
		box.setCenter(box.getCenter() + offset);
		box.setDirty(false);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Scene::stopSceneAnimations()
{
	DynamicComponent* dynamicComponent;

	forIT(m_vectorNode)
	{
		dynamicComponent = (*it)->refDynamicComponent();

		if ((dynamicComponent != nullptr) && dynamicComponent->getKeyframeInformationSet())
		{
			dynamicComponent->setPlay(false);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
