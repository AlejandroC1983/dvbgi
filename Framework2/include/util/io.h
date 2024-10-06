/*
Copyright 2020 Alejandro Cosin

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

#ifndef _IO_H_
#define _IO_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/headers.h"

// CLASS FORWARDING

// NAMESPACE

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

class InputOutput
{
public:
	/** Read file given path as parameter
	* @param filePath [in] Path of the file to read
	* @param fileSize [in] Size of the file read
	* @return pointer to file content */
	static void* readFile(const char *filePath, size_t *fileSize);
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _IO_H_
