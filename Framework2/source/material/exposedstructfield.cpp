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

// PROJECT INCLUDES
#include "../../include/material/exposedstructfield.h"

// NAMESPACE

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

ExposedStructField::ExposedStructField() :
	m_internalType(ResourceInternalType::RIT_SIZE)
	, m_data(nullptr)
	, m_structFieldResource(nullptr)
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

ExposedStructField::ExposedStructField(ResourceInternalType internalType, string&& structName, string&& structFieldName, void* data) :
	m_internalType(internalType)
	, m_data(data)
	, m_structFieldResource(nullptr)
	, m_structName(move(structName))
	, m_structFieldName(move(structFieldName))
{

}

/////////////////////////////////////////////////////////////////////////////////////////////
