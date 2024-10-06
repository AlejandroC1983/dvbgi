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

#ifndef _SCENE_H_
#define _SCENE_H_

// GLOBAL INCLUDES
#include "../../external/nano-signal-slot/nano_signal_slot.hpp"

// PROJECT INCLUDES
#include "../../include/node/node.h"
#include "../../include/util/singleton.h"
#include "../../include/commonnamespace.h"
#include "../../include/util/loopmacrodefines.h"
#include "../../include/component/rendercomponent.h"
#include "../../include/component/skinnedmeshrendercomponent.h"
#include "../../include/component/dynamiccomponent.h"

// CLASS FORWARDING
class RasterTechnique;
class Camera;

// NAMESPACE
using namespace commonnamespace;

// DEFINES
#define sceneM s_pSceneSingleton->instance()
typedef Nano::Signal<void()> SignalSceneDirtyNotification;

/** Struct used to store, for each scene element in vectorNodePtr, the position and bounding sphere radius */
struct InstanceData
{
	vec3 m_position; // Position of the scene elemenr
	float m_radius;  // Radius of the bounding sphere for the scene element
};

typedef vector<InstanceData> vectorInstanceData;

/////////////////////////////////////////////////////////////////////////////////////////////

class Scene: public Singleton<Scene>
{
public:
	/** Default constructor
	* @return nothing */
	Scene();

	/** Default destructor
	* @return nothing */
	~Scene();

	/** Initialization of the scene (here, the scene elements and materials needed are loaded)
	* @return nothing */
    bool init();

	/** Update all scene elements and camera
	* @return nothing */
    void update();

	/** Free all resources holded and managed by this class
	* @return nothing */
    void shutdown();

	/** Performs all the required computations when adding a model to the scene
	* @param node [in] node to add to the scene */
	void addModel(Node *node);

	/** When a node is moved in the scene, the bb of the scene is updated
	* @return nothing */
	void updateBB();

	/** Returns a pointer to a scene element if any has the name
	* @param name [in] name of the node to search in m_mapStringNode
	* @return pointer to the found node or nulltpr otherwise */
	Node* refElementByName(string&& name);

	/** Returns a const pointer to a scene element if any has the name 
	* @param name [in] name of the node to search in m_mapStringNode 
	* @return const pointer to the found node or nulltpr otherwise */
	const Node* getElementByName(string&& name);

	/** Returns the index of the scene element with name given by the name parameter in the m_vModel vector
	* @param name [in] name of the node to search in m_mapStringNode
	* @return index of the found element or -1 otherwise */
	int getElementIndexByName(string&& name);

	/** Returns the index of the node given by the node parameter in the m_vModel vector
	* @param node [in] node to search in m_mapStringNode
	* @return index of the found node or -1 otherwise */
	int getElementIndex(Node* node);

	/** Takes the vector of pointers to Node instances vectorNode and sorts it by material hashed name value
	* @param vectorNode [inout] vector to sort
	* @return nothing */
	static void sortByMaterial(vectorNodePtr& vectorNode);

	/** Returns a vector with the scene elements with the flags given by meshType parameter
	* @param meshType [in] type of mesh, flags used to retieve elements
	* @return vector with the scene elements with the flags given by meshType parameter */
	vectorNodePtr getByMeshType(eMeshType meshType);

	/** Returns a vector with the scene elements with the flags given by nodeType parameter
	* @param nodeType [in] type of node
	* @return vector with the scene elements with the flags given by nodeType parameter */
	vectorNodePtr getByNodeType(eNodeType nodeType);

	/** Returns a vector with the scene elements with at least one component of the type given by the componentType parameter
	* @param componentType [in] type of component to lok for
	* @return vector with the scene elements with at least one component of the type given by the componentType parameter */
	vectorNodePtr getByComponentType(GenericResourceType componentType);

	/** Add cameraParameter to m_vectorCamera if not already added
	param cameraParameter [in] light to add to the scene if not already added
	* @return true if added successfully, false otherwise */
	bool addCamera(Camera* cameraParameter);

	/** Adds to the translation of each scene element value given by the offset parameter
	* @param offset [in] offset to add to the whole scene
	* @return nothing */
	void addWholeSceneOffset(vec3 offset);

	/** Stop the animation of all scene elements
	* @return nothing */
	void stopSceneAnimations();
	
	GET_SET(float, m_deltaTime, DeltaTime)
	REF_SET(BBox3D, m_aabb, Box)
	GET(string, m_sceneName, SceneName)
	GET(vectorString, m_transparentKeywords, TransparentKeywords)
	GET(vectorNodePtr, m_vectorNode, VectorNode)
	REF(SignalSceneDirtyNotification, m_signalSceneDirtyNotification, SignalSceneDirtyNotification)
	GET(vectorString, m_animationDiscardKeywords, AnimationDiscardKeywords)
	GET(vectorNodePtr, m_vectorNodeFrameUpdated, VectorNodeFrameUpdated)
	GET(vectorInt, m_vectorNodeIndexFrameUpdated, VectorNodeIndexFrameUpdated)
	GETCOPY(bool, m_isVectorNodeFrameUpdated, IsVectorNodeFrameUpdated)

protected:
	float		                 m_deltaTime;                    //!< Delta time, the time between the last frame rendered and this frame
	BBox3D		                 m_aabb;                         //!< Scene bb, updated whenever an element is added in addModel
	vectorNodePtr                m_vectorNode;                   //!< Nodes in the scene
	mapStringNode                m_mapStringNode;                //!< Map with the key mapped value pair <string, node pointer> to locate quicker nodes by name
	Camera*                      m_sceneCamera;                  //!< Scene camera
	static string                m_scenePath;                    //!< Path to the scene to load
	static string                m_sceneName;                    //!< Name of the scene file to load
	static vectorString          m_transparentKeywords;          //!< Vector with strings to be used to identify transparent materials in the material name (pending a more ellaborated way to identify those materials from .obj scenes, which need a discard operation depending on alpha test value
	static vectorString          m_animationDiscardKeywords;     //!< Vector with strings to be used to identify animations to discard
	static vectorString          m_modelDiscardKeywords;         //!< Vector with strings to be used to discard specifi scene elements when loading the scene
	vectorCameraPtr              m_vectorCamera;                 //!< Vector with all the scene cameras (emitters also have their cameras here)
	vectorNodePtr                m_vectorNodeFrameUpdated;       //!< Vector with the elements from m_vectorNode which had a dirty transform when the call to Scene::update was done for the last time
	vectorInt                    m_vectorNodeIndexFrameUpdated;  //!< Vector with the indices of the elements from m_vectorNode which had a dirty transform when the call to Scene::update was done for the last time
	Node*                        m_staticSceneCompactedGeometry; //!< Node with the static scene geometry, used for singla draw call render passes
	SignalSceneDirtyNotification m_signalSceneDirtyNotification; //!< Scene dirty notification
	vectorBaseComponentPtr       m_vectorSceneComponent;         //!< Vector with scene components used to alter the behaviour of the scene or the scene elements
	bool                         m_isVectorNodeFrameUpdated;     //!< Flag to know whether there are any scene elements with a transform to be updated, beased on whether m_vectorNodeFrameUpdated has a least one element after the call to Scene::update
};

static Scene *s_pSceneSingleton;

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _SCENE_H_
