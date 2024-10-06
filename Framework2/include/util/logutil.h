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

#ifndef _LOGUTIL_H_
#define _LOGUTIL_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/headers.h"

// CLASS FORWARDING

// NAMESPACE

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

class LogUtil
{
public:
	/** Prints information in a tabulated way for the resources with name and size given as parameter in mapData
	* @param mapData [in] buffer data to print
	* @return nothing */
	static void printInformationTabulated(const map<string, int>& mapData);
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _LOGUTIL_H_
