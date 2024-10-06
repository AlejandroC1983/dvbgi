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

#ifndef _SCENEELEMENTCONTROLCOMPONENT_H_
#define _SCENEELEMENTCONTROLCOMPONENT_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/math/transform.h"
#include "../../include/component/basecomponent.h"

// CLASS FORWARDING

// NAMESPACE

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

class SceneElementControlComponent : public BaseComponent
{
	friend class ComponentManager;

protected:
	/** Default constructor
	* @return nothing */
	//SceneElementControlComponent();

	/** Paremeter constructor
	* @param name [in] component name
	* @return nothing */
	SceneElementControlComponent(string&& name);

public:
	/** Default destructor
	* @return nothing */
	virtual ~SceneElementControlComponent();

	/** Initialize the resource
	* @return nothing */
	virtual void init();

	/** Update the status of the current animation
	* @param delta [in]    elapsed time since the last frame, in ms
	* @param delta [inout] transform of the node owning this component, it will be modified if there are any changes
	* @return true if transform was modified, false, otherwise */
	bool update(float delta, Transform& transform);

protected:
	/** Callback for when the user pressed key M to stop all scene element animations
	* @ return nothing */
	void slotMKeyPressed();

	/** Callback for when the user pressed key keypad 4
	* @ return nothing */
	void slotKeypad4KeyPressed();

	/** Callback for when the user pressed key keypad 6
	* @ return nothing */
	void slotKeypad6KeyPressed();

	/** Callback for when the user pressed key keypad 8
	* @ return nothing */
	void slotKeypad8KeyPressed();

	/** Callback for when the user pressed key keypad 2
	* @ return nothing */
	void slotKeypad2KeyPressed();

	/** Callback for when the user pressed key keypad 7
	* @ return nothing */
	void slotKeypad7KeyPressed();

	/** Callback for when the user pressed key keypad 9
	* @ return nothing */
	void slotKeypad9KeyPressed();

	vectorNodePtr m_vectorDynamicNode; //!< Vector with pointers to the scene dynamic scene nodes
};		   							  

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _SCENEELEMENTCONTROLCOMPONENT_H_
