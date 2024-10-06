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
#include "../../include/util/logutil.h"
#include "../../include/util/loopMacroDefines.h"

// NAMESPACE

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

void LogUtil::printInformationTabulated(const map<string, int>& mapData)
{
	int longestName = 0;

	forIT(mapData)
	{
		longestName = max(longestName, int(it->first.size()));
	}

	longestName += 5;
	int sizeByteOffset = longestName + 20;
	int sizeKBOffset = sizeByteOffset + 20;

	int i;
	string line;
	int mappingSize;
	uint totalSize = 0;
	int currentSize;

	line = "Resource name";
	currentSize = int(line.size());

	for (i = currentSize; i < longestName; ++i)
	{
		line += " ";
	}

	line += "Size(bytes)";
	currentSize = int(line.size());

	for (i = currentSize; i < sizeByteOffset; ++i)
	{
		line += " ";
	}
	line += "Size(KB)";
	currentSize = int(line.size());

	for (i = currentSize; i < sizeKBOffset; ++i)
	{
		line += " ";
	}

	line += "Size(MB)";
	currentSize = int(line.size());

	string lineTemp;
	for (i = 0; i < (currentSize + 5); ++i)
	{
		lineTemp += "-";
	}

	cout << endl;
	cout << line << endl;
	cout << lineTemp << endl;

	forIT(mapData)
	{
		line = it->first;
		currentSize = int(it->first.size());

		for (i = currentSize; i < longestName; ++i)
		{
			line += " ";
		}

		mappingSize = int(it->second);
		totalSize += uint(mappingSize);
		line += to_string(mappingSize);
		currentSize = int(line.size());

		for (i = currentSize; i < sizeByteOffset; ++i)
		{
			line += " ";
		}

		line += to_string(float(mappingSize) / 1024.0f);
		currentSize = int(line.size());

		for (i = currentSize; i < sizeKBOffset; ++i)
		{
			line += " ";
		}

		line += to_string(float(mappingSize) / (1024.0f * 1024.0f));
		currentSize = int(line.size());

		cout << line << endl;
	}

	line = "Total";
	currentSize = 5;

	for (i = currentSize; i < longestName; ++i)
	{
		line += " ";
	}

	line += to_string(totalSize);
	currentSize = int(line.size());

	for (i = currentSize; i < sizeByteOffset; ++i)
	{
		line += " ";
	}

	line += to_string(float(totalSize) / 1024.0f);
	currentSize = int(line.size());

	for (i = currentSize; i < sizeKBOffset; ++i)
	{
		line += " ";
	}

	line += to_string(float(totalSize) / (1024.0f * 1024.0f));
	currentSize = int(line.size());

	cout << line << endl;

	cout << lineTemp << endl;
}

/////////////////////////////////////////////////////////////////////////////////////////////
