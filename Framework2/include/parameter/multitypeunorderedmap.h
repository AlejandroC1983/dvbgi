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

#ifndef _MULTITYPEUNORDEREDMAP_H_
#define _MULTITYPEUNORDEREDMAP_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/parameter/multitypeimplementation.h"
#include "../../include/commonnamespace.h"

// CLASS FORWARDING

// NAMESPACE
using namespace commonnamespace;

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

/** MultiTypeUnorderedMap class, allows storing elements of different types as unique_ptr instances and retrive them
* using the proper templated parameters and the shased name of the element to be retrived. This milty type container
* is done considering it will contain instances of the templatized struct AttributeData, which has an m_hashedName member class */

class MultiTypeUnorderedMap
{
public:
	/** Casts the value unorderedMap_[hashed] to the obne given as template parameter
	* @param hashed [in] value to return in unorderedMap_
	* @return casted version of unorderedMap_[hashed] element */
	bool elementExists(unsigned int hashed)
	{
		return (unorderedMap_.find(hashed) != unorderedMap_.end());
	}

	/** Makes a new element in unorderedMap_ taking the AttributeData::m_hashedName of the element parameter and the element
	* parameter to build the pair to be inserted in unorderedMap_
	* @param element [in] element to insert in unorderedMap_
	* @return true if the element was inserted in unorderedMap_ and false otherwise */
	template <typename T> bool newElement(T element)
	{
		bool result = makeNewElement(element);

		if (!result)
		{
			return false;
		}

		unorderedMapCast<T>(element->m_hashedName).assignData(element);
		return true;
	}

	/** Removes from unorderedMap_ the element with key hashedName if any
	* @param hashedName [in] element's hashed name to remove from unorderedMap_
	* @return true if the element was removed successfully and false otherwise */
	bool removeElement(unsigned int hashedName)
	{
		return (unorderedMap_.erase(hashedName) > 0);
	}

	/** Retrieves from unorderedMap_ and casts the element with hashedName given as parameter
	* @param hashedName [in] element's hashed name to retrieve from unorderedMap_ and cast
	* @return the casted element if found, nullptr otherwise */
	template <typename T> T getElement(unsigned int hashedName)
	{
		if (elementExists(hashedName))
		{
			return unorderedMapCast<T>(hashedName).getData();
		}
		return nullptr;
	}

	/** Retrieves from unorderedMap_ and casts the element with hashedName given as parameter
	* @param hashedName [in] element's hashed name to retrieve from unorderedMap_ and cast
	* @return the casted element if found, nullptr otherwise */
	template <typename T> const T getElement(unsigned int hashedName) const
	{
		if (elementExists(hashedName))
		{
			return unorderedMapCast<T>(hashedName).getData();
		}
		return nullptr;
	}

protected:
	/** Makes a new element in unorderedMap_
	* @param element [in] element to add to unorderedMap_
	* @return true if the new element was made successfully and false otherwise */
	template <typename T> bool makeNewElement(T element)
	{
		unordered_map<unsigned int, unique_ptr<MultiTypeBase>>::iterator it = unorderedMap_.find(element->m_hashedName);
		if (it == unorderedMap_.end())
		{
			unorderedMap_.insert(make_pair<unsigned int, unique_ptr<MultiTypeBase>>(move(unsigned int(element->m_hashedName)), make_unique<MultiTypeImplementation<T>>()));
			return true;
		}
		return false;
	}

	/** Casts the value unorderedMap_[hashed] to the obne given as template parameter
	* @param hashed [in] value to return in unorderedMap_
	* @return casted version of unorderedMap_[hashed] element */
	template <typename T> MultiTypeImplementation<T>& unorderedMapCast(unsigned int hashed)
	{
		return dynamic_cast <MultiTypeImplementation<T>&>(*unorderedMap_[hashed]);
	}

	//!< Used to store the (hashed name, unique_ptr) elements for the multi type unordered map container
	mutable unordered_map <unsigned int, unique_ptr <MultiTypeBase>> unorderedMap_;
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _MULTITYPEUNORDEREDMAP_H_
