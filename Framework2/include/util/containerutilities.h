/*
Copyright 2016 Alejandro Cosin

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

// Taken from https://gist.github.com/aminzai/2706798

#ifndef _CONTAINERUTILITIES_H_
#define _CONTAINERUTILITIES_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/util/loopMacroDefines.h"
#include "../../include/commonnamespace.h"

// NAMESPACE
using namespace commonnamespace;

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

/** Deletes every single element in the pointer vector given as parameter and sets each element to nullptr
* @param vectorData [in] vector of pointer to elements to delete
* @return nothing */
template <class T> inline void deleteVectorInstances(vector<T*> &vectorData)
{
	forI(vectorData.size())
	{
		delete vectorData[i];
		vectorData[i] = nullptr;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

/** Adds element to vectorData if no present
* @param element    [in] element to add to vectorData if no present
* @param vectorData [in] vector of data to which add element if no present
* @return true if added, false otherwise */
template <class T> inline bool addIfNoPresent(T *element, vector<T*> &vectorData)
{
	if (find(vectorData.begin(), vectorData.end(), element) == vectorData.end())
	{
		vectorData.push_back(element);
		return true;
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////

/** Adds element to vectorData if no present
* @param element    [in] element to add to vectorData if no present
* @param vectorData [in] vector of data to which add element if no present
* @return true if added, false otherwise */
template <class T> inline bool addIfNoPresent(T element, vector<T> &vectorData)
{
	if (find(vectorData.begin(), vectorData.end(), element) == vectorData.end())
	{
		vectorData.push_back(element);
		return true;
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////

/** Adds element to vectorData if no present
* @param element    [in] element to add to vectorData if no present
* @param vectorData [in] vector of data to which add element if no present
* @return true if added, false otherwise */
template <class T> inline bool removeIfPresent(T element, vector<T> &vectorData)
{
	vector<T>::iterator it = find(vectorData.begin(), vectorData.end(), element);

	if (it != vectorData.end())
	{
		vectorData.erase(it);
		return true;
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////

/** Finds element index in vectorData if present
* @param element    [in] element to find in vectorData
* @param vectorData [in] vector of data in which to find
* @return index of element if found, nullptr otherwise */
template <class T> inline int findElementIndex(vector<T> &vectorData, T element)
{
	int numElements = int(vectorData.size());

	vector<T>::iterator it = find(vectorData.begin(), vectorData.end(), element);

	if (it == vectorData.end())
	{
		return -1;
	}

	return int(distance(vectorData.begin(), it));
}

/////////////////////////////////////////////////////////////////////////////////////////////

/** Finds element in vectorData if present
* @param element    [in] element to find in vectorData
* @param vectorData [in] vector of data in which to find
* @return index of element if found, nullptr otherwise */
template <class T> inline int findIndexByNameMethodPtr(vector<T> &vectorData, const string &&name)
{
	int numElements = int(vectorData.size());

	forI(uint(numElements))
	{
		if (vectorData[i]->getName() == name)
		{
			return i;
		}
	}

	return -1;
}

/////////////////////////////////////////////////////////////////////////////////////////////

/** Finds element in vectorData if present
* @param element    [in] element to find in vectorData
* @param vectorData [in] vector of data in which to find
* @return pointer to element if found, nullptr otherwise */
template <class T> inline T findByNameMethodPtr(vector<T> &vectorData, const string &&name)
{
	int numElements = vectorData.size();

	forI(numElements)
	{
		if (vectorData[i]->getName() == name)
		{
			return vectorData[i];
		}
	}

	return nullptr;
}

/////////////////////////////////////////////////////////////////////////////////////////////

/** Adds the pair <T, Q*>(key, value) element to mapData if the key value is not present
* @param key     [in] key value to test if present in mapData
* @param value   [in] value to add together with the key to mapData
* @param mapData [in] map to add the pair <T, Q*>(key, value) if key is not present
* @return true if added, false otherwise */
template <class T, class Q> inline bool addIfNoPresent(T &&key, Q value, map<T, Q> &mapData)
{
	auto it = mapData.find(key);

	if(it == mapData.end())
	{
		return mapData.insert(pair<T, Q>(key, value)).second;
	}
	
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////

/** Removes the element with key given as parameter from mapData if present
* @param key     [in] key value to test if present in mapData
* @param mapData [in] map to remove the pair <T, Q*>(key, value) if key is present
* @return true if removed, false otherwise */
template <class T, class Q> inline bool removeByKey(T &&key, map<T, Q> &mapData)
{
	auto it = mapData.find(key);

	if (it == mapData.end())
	{
		return false;
	}

	delete (it->second);

	return (mapData.erase(key) > 0);
}

/////////////////////////////////////////////////////////////////////////////////////////////

/** Returns the mapped value by the key value parameter if it does exist in mapData and nullptr otherwise
* @param key     [in] key value to test if present in mapData
* @param mapData [in] map to return the value mapped by the given key parameter
* @return the mapped value by the key value parameter if it does exist in mapData and nullptr otherwise */
template <class T, class Q> inline Q getByKey(T &&key, map<T, Q> &mapData)
{
	auto it = mapData.find(key);

	if (it == mapData.end())
	{
		return nullptr;
	}

	return it->second;
}

/////////////////////////////////////////////////////////////////////////////////////////////

/** Returns true if there's an element in mapData with key value equal to the one given as parameter and false otherwise
* @param key     [in] key value to test if present in mapData
* @param mapData [in] map to test if there's a key value equal to the key parameter
* @return true if there's an element in mapData with key value equal to the one given as parameter and false otherwise */
template <class T, class Q> inline bool existsByKey(T &&key, map<T, Q> &mapData)
{
	return (mapData.find(key) != mapData.end());
}

/////////////////////////////////////////////////////////////////////////////////////////////

/** Iterates over the elements present in mapData, callig delete of each mapped value and assigning its value to nullptr
* @param mapData [in] map to delete mapped values
* @return nothing */
template <class T, class Q> inline void deleteMapped(map<T, Q*> &mapData)
{
	forIT(mapData)
	{
		delete it->second;
		it->second = nullptr;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

/** Traverses vectorElement trying to find whether each element is a substring of the element parameter
* @param vectorElement [in] string vector to analyse
* @param element       [in] string to compare against each element in vectorElement
* @return true if added, false otherwise */
inline bool anyVectorElementisSubstring(const vector<string>& vectorElement, const string& element)
{
	forIT(vectorElement)
	{
		if (element.find(*it) != string::npos)
		{
			return true;
		}
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _CONTAINERUTILITIES_H_
