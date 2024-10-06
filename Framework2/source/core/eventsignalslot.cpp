/*
Copyright 2018 Alejandro Cosin

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
#include <windowsx.h>

// PROJECT INCLUDES
#include "../../include/core/eventsignalslot.h"
#include "../../include/util/containerutilities.h"

// NAMESPACE

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

EventSignalSlot::~EventSignalSlot()
{
	deleteMapped(m_mapKeyDown);
	deleteMapped(m_mapKeyUp);
}

/////////////////////////////////////////////////////////////////////////////////////////////

SignalVoid* EventSignalSlot::refKeyDownSignalByKey(KeyCode key)
{
	return refKeyUpSignalByKey(m_mapKeyDown, key);
}

/////////////////////////////////////////////////////////////////////////////////////////////

SignalVoid* EventSignalSlot::refKeyUpSignalByKey(KeyCode key)
{
	return refKeyUpSignalByKey(m_mapKeyUp, key);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void EventSignalSlot::addKeyDownSignal(KeyCode key)
{
	addKeySignal(m_mapKeyDown, key);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void EventSignalSlot::addKeyUpSignal(KeyCode key)
{
	addKeySignal(m_mapKeyUp, key);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void EventSignalSlot::emitSignalForDownKey(KeyCode key)
{
	emitKeySignal(m_mapKeyDown, key);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void EventSignalSlot::emitSignalForUpKey(KeyCode key)
{
	emitKeySignal(m_mapKeyUp, key);
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool EventSignalSlot::isKeyDownRegistered(KeyCode key)
{
	return (m_mapKeyDown.find(static_cast<int>(key)) != m_mapKeyDown.end());
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool EventSignalSlot::isKeyUpRegistered(KeyCode key)
{
	return (m_mapKeyUp.find(static_cast<int>(key)) != m_mapKeyUp.end());
}

/////////////////////////////////////////////////////////////////////////////////////////////

SignalVoid* EventSignalSlot::refKeyUpSignalByKey(map<int, SignalVoid*>& mapKey, KeyCode key)
{
	map<int, SignalVoid*>::iterator it = mapKey.find(static_cast<int>(key));

	if (it != mapKey.end())
	{
		return it->second;
	}

	return nullptr;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void EventSignalSlot::addKeySignal(map<int, SignalVoid*>& mapKey, KeyCode key)
{
	map<int, SignalVoid*>::iterator it = mapKey.find(static_cast<int>(key));

	if (it != mapKey.end())
	{
		cout << "ERROR: trying to add again a key value " << static_cast<int>(key) << " in EventSignalSlot::addKeySignal" << endl;
	}

	SignalVoid* signalVoidToInsert = new SignalVoid;
	mapKey.insert(pair<int, SignalVoid*>(static_cast<int>(key), signalVoidToInsert));
}

/////////////////////////////////////////////////////////////////////////////////////////////

void EventSignalSlot::emitKeySignal(map<int, SignalVoid*>& mapKey, KeyCode key)
{
	map<int, SignalVoid*>::iterator it = mapKey.find(static_cast<int>(key));

	if (it == mapKey.end())
	{
		return;
	}

	it->second->emit();
}

/////////////////////////////////////////////////////////////////////////////////////////////
