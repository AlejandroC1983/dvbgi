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

#ifndef _CAMERAMANAGER_H_
#define _CAMERAMANAGER_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/util/singleton.h"
#include "../../include/util/managertemplate.h"
#include "../../include/camera/camera.h"
#include "../headers.h"

// CLASS FORWARDING
class Camera;

// NAMESPACE

// DEFINES
#define cameraM s_pCameraManager->instance()

struct RecordedCamera
{
	vec3 m_position;   //!< Position of the camera
	vec3 m_lookAt;     //!< Target point for the camera
	vec3 m_up;         //!< Up vector for the camera
	vec3 m_right;      //!< Right direction of the camera according to its position and orientation
	mat4 m_view;       //!< View matrix
	mat4 m_projection; //!< Projection matrix

};

/////////////////////////////////////////////////////////////////////////////////////////////

class CameraManager: public ManagerTemplate<Camera>, public Singleton<CameraManager>
{
	friend class CoreManager;

public:
	/** Constructor
	* @return nothing */
	CameraManager();

	/** Destructor
	* @return nothing */
	virtual ~CameraManager();

	/** Builds a new camera, a pointer to the camera is returned, nullptr is returned if any errors while building it
	* @param instanceName [in] name of the new camera instance (the m_sName member variable)
	* @param cameraType   [in] camera type
	* @param vPos         [in] position of the camera
	* @param vLookAt      [in] where is the camera looking at
	* @param up           [in] up vector
	* @param zNear        [in] near clip plane distance
	* @param zFar         [in] far clip plane distance
	* @param fov          [in] field of view (radians)
	* @return a pointer to the built texture, nullptr otherwise */	
	Camera* buildCamera(string &&instanceName,
						CameraType cameraType,
						vec3 vPos,
						vec3 vLookAt,
						vec3 up,
						float zNear,
						float zFar,
						float fov);

	/** Destroys all elements in the manager
	* @return nothing */
	void destroyResources();

	/** Will set the camera given as parameter as the one being controlled by the user
	* @param camera [in] camera to set as main camera
	* @return nothing */
	void setAsMainCamera(Camera* camera);

	/** For each camera updates position, allowing a smooth movment independent from the framerate
	* @param deltaTime    [in] Elapsed time since last call to the funtion
	* @param offsetFactor [in] Offset factor to scale the movement speed
	* @return nothing */
	void updateMovementState(float deltaTime, float offsetFactor);

	/** Look for a file in data\scenes\temp with the same name as the scene loaded and, if it exists, loads its content into m_vectorRecordedCamera
	* @return nothing */
	void loadCameraRecordingData();

	/** Look for a file in data\scenes\temp with the same name as the scene loaded and write its content into m_vectorRecordedCamera, if it does not
	* exist then the file wil be made
	* @return nothing */
	void writeCameraRecordingData();

	/** Take all the information to fill a RecordedCamera struct and put it in the file with name the scene name in data\scenes\temp
	* @param position   [in] Position of the camera
	* @param lookAt     [in] Target point for the camera
	* @param up         [in] Up vector for the camera
	* @param right      [in] Right direction of the camera according to its position and orientation
	* @param view       [in] View matrix
	* @param projection [in] Projection matrix
	* @return nothing */
	void takeCameraRecording(vec3 position, vec3 lookAt, vec3 up, vec3 right, const mat4& view, const mat4& projection);

	REF_PTR(Camera, m_mainCamera, MainCamera)
	GET_PTR(Camera, m_mainCamera, MainCamera)
	GET(bool, m_operatingCamera, OperatingCamera)
	REF_PTR(Camera, m_cameraOperated, CameraOperated)
	GETCOPY_SET(bool, m_cyclingRecorderCameras, CyclingRecorderCameras)
	GETCOPY_SET(int, m_recorderCameraIndex, RecorderCameraIndex)

protected:
	/** Slot for the keyboard signal when pressing the S key to change the main camera for the next one in m_mapElement
	* @return nothing */
	void useNextCamera();

	/** Slot for the keyboard signal when pressing the D key to change the camera being controlled for the next one
	* in m_mapElement
	* @return nothing */
	void operateNextCamera();

	/** Will clean all signals from the keyboard and mouse events slots used
	* @return nothing */
	void cleanAllCameraSignal();

	/** Assign the signal slots used for camera controls to the camera given as parameter
	* @param camera [in] camera to assign slots
	* @return nothing */
	void assignSlotsToCamera(Camera* camera);

	/** Slot for receiving signal from the input manager when the key with number 9 (not numpad) is pressed, to add the matrix information
	* of the current camera to the array of camera datas reocrded
	* @return nothing */
	void slot9Pressed();

	/** Slot for receiving signal from the input manager when the key with number 0 (not numpad) is pressed, to start cycling through the vector
	* of recorded cameras for the current scene (which are stored in a file in data\scenes\temp with the name of the scene, and loaded at scene start).
	* @return nothing */
	void slot0Pressed();

	Camera*                m_mainCamera;             //!< Current main camera being used
	Camera*                m_cameraOperated;         //!< Camera being operated (if any)
	bool                   m_operatingCamera;        //!< True if a camera not currently the one in the viewport is being operated
	bool                   m_cyclingRecorderCameras; //!< To know whether the current camera is cycling through recorded cameras
	int                    m_recorderCameraIndex;    //!< To know what recorded camera index to show
	vector<RecordedCamera> m_vectorRecordedCamera;   //!< Vector with the recorded camera data
	static int             m_nextCameraOperateIndex; //!< Static variable to iterate over the elements of m_mapElement

};

static CameraManager* s_pCameraManager;

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _CAMERAMANAGER_H_
