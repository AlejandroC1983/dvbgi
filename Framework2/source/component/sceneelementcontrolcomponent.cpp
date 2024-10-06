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
#include "../../include/component/sceneelementcontrolcomponent.h"
#include "../../include/core/input.h"
#include "../../include/scene/scene.h"

// NAMESPACE

// DEFINES
const float elementOffset = 0.1f;

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

SceneElementControlComponent::SceneElementControlComponent(string&& name) : BaseComponent(move(name), move(string("SceneElementControlComponent")), GenericResourceType::GRT_SCENEELEMENTCONTROLCOMPONENT)
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

SceneElementControlComponent::~SceneElementControlComponent()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

void SceneElementControlComponent::init()
{
	inputM->refEventSinglePressSignalSlot().addKeyDownSignal(KeyCode::KEY_CODE_M);
	SignalVoid* signalAdd = inputM->refEventSinglePressSignalSlot().refKeyDownSignalByKey(KeyCode::KEY_CODE_M);
	signalAdd->connect<SceneElementControlComponent, &SceneElementControlComponent::slotMKeyPressed>(this);

	inputM->refEventSinglePressSignalSlot().addKeyDownSignal(KeyCode::KEY_CODE_KP_4);
	signalAdd = inputM->refEventSinglePressSignalSlot().refKeyDownSignalByKey(KeyCode::KEY_CODE_KP_4);
	signalAdd->connect<SceneElementControlComponent, &SceneElementControlComponent::slotKeypad4KeyPressed>(this);

	inputM->refEventSinglePressSignalSlot().addKeyDownSignal(KeyCode::KEY_CODE_KP_6);
	signalAdd = inputM->refEventSinglePressSignalSlot().refKeyDownSignalByKey(KeyCode::KEY_CODE_KP_6);
	signalAdd->connect<SceneElementControlComponent, &SceneElementControlComponent::slotKeypad6KeyPressed>(this);

	inputM->refEventSinglePressSignalSlot().addKeyDownSignal(KeyCode::KEY_CODE_KP_8);
	signalAdd = inputM->refEventSinglePressSignalSlot().refKeyDownSignalByKey(KeyCode::KEY_CODE_KP_8);
	signalAdd->connect<SceneElementControlComponent, &SceneElementControlComponent::slotKeypad8KeyPressed>(this);

	inputM->refEventSinglePressSignalSlot().addKeyDownSignal(KeyCode::KEY_CODE_KP_2);
	signalAdd = inputM->refEventSinglePressSignalSlot().refKeyDownSignalByKey(KeyCode::KEY_CODE_KP_2);
	signalAdd->connect<SceneElementControlComponent, &SceneElementControlComponent::slotKeypad2KeyPressed>(this);

	inputM->refEventSinglePressSignalSlot().addKeyDownSignal(KeyCode::KEY_CODE_KP_7);
	signalAdd = inputM->refEventSinglePressSignalSlot().refKeyDownSignalByKey(KeyCode::KEY_CODE_KP_7);
	signalAdd->connect<SceneElementControlComponent, &SceneElementControlComponent::slotKeypad7KeyPressed>(this);

	inputM->refEventSinglePressSignalSlot().addKeyDownSignal(KeyCode::KEY_CODE_KP_9);
	signalAdd = inputM->refEventSinglePressSignalSlot().refKeyDownSignalByKey(KeyCode::KEY_CODE_KP_9);
	signalAdd->connect<SceneElementControlComponent, &SceneElementControlComponent::slotKeypad9KeyPressed>(this);

	m_vectorDynamicNode = sceneM->getByNodeType(eNodeType::E_NT_DYNAMIC_ELEMENT);
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool SceneElementControlComponent::update(float delta, Transform& transform)
{
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void SceneElementControlComponent::slotMKeyPressed()
{
	// Stop all animations from all scene elements
	sceneM->stopSceneAnimations();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void SceneElementControlComponent::slotKeypad4KeyPressed()
{
	forI(m_vectorDynamicNode.size())
	{
		m_vectorDynamicNode[i]->refTransform().addTraslation(vec3(-1.0f * elementOffset, 0.0f, 0.0f));
		vec3 traslation = m_vectorDynamicNode[i]->refTransform().getTraslation();
		cout << "New position for dynamic element (" << traslation.x << ", " << traslation.y << ", " << traslation.z << ")" << endl;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void SceneElementControlComponent::slotKeypad6KeyPressed()
{
	forI(m_vectorDynamicNode.size())
	{
		m_vectorDynamicNode[i]->refTransform().addTraslation(vec3(elementOffset, 0.0f, 0.0f));
		vec3 traslation = m_vectorDynamicNode[i]->refTransform().getTraslation();
		cout << "New position for dynamic element (" << traslation.x << ", " << traslation.y << ", " << traslation.z << ")" << endl;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void SceneElementControlComponent::slotKeypad8KeyPressed()
{
	forI(m_vectorDynamicNode.size())
	{
		m_vectorDynamicNode[i]->refTransform().addTraslation(vec3(0.0f, 0.0f, -1.0f * elementOffset));
		vec3 traslation = m_vectorDynamicNode[i]->refTransform().getTraslation();
		cout << "New position for dynamic element (" << traslation.x << ", " << traslation.y << ", " << traslation.z << ")" << endl;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void SceneElementControlComponent::slotKeypad2KeyPressed()
{
	forI(m_vectorDynamicNode.size())
	{
		m_vectorDynamicNode[i]->refTransform().addTraslation(vec3(0.0f, 0.0f, elementOffset));
		vec3 traslation = m_vectorDynamicNode[i]->refTransform().getTraslation();
		cout << "New position for dynamic element (" << traslation.x << ", " << traslation.y << ", " << traslation.z << ")" << endl;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void SceneElementControlComponent::slotKeypad7KeyPressed()
{
	forI(m_vectorDynamicNode.size())
	{
		m_vectorDynamicNode[i]->refTransform().addTraslation(vec3(0.0f, -1.0f * elementOffset, 0.0f));
		vec3 traslation = m_vectorDynamicNode[i]->refTransform().getTraslation();
		cout << "New position for dynamic element (" << traslation.x << ", " << traslation.y << ", " << traslation.z << ")" << endl;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void SceneElementControlComponent::slotKeypad9KeyPressed()
{
	forI(m_vectorDynamicNode.size())
	{
		m_vectorDynamicNode[i]->refTransform().addTraslation(vec3(0.0f, elementOffset, 0.0f));
		vec3 traslation = m_vectorDynamicNode[i]->refTransform().getTraslation();
		cout << "New position for dynamic element (" << traslation.x << ", " << traslation.y << ", " << traslation.z << ")" << endl;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
