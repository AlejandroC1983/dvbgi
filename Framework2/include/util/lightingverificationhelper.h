/*
Copyright 2014 Alejandro Cosin

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

#ifndef _LIGHTINGVERIFICATIONHELPER_H_
#define _LIGHTINGVERIFICATIONHELPER_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../headers.h"
#include "../../include/util/getsetmacros.h"

// CLASS FORWARDING

// NAMESPACE
using namespace commonnamespace;

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

/** This is a helper class to verify the lighting information of the texture space expanding planes */
class LightingVerificationHelper
{
public:
	/** Will save to path given by parameter, with file name given by parameter, the information present in the
	* vectorData parameter for plotting. For scaling purposes, the information in each element of vectorData
	* will be multiplied by the multiplier parameter
	* @param vectorData [in] vector with the texture information
	* @param multiplier [in] value that will multiply each element in vectorData
	* @param minValue   [in] min value range
	* @param maxValue   [in] max value range
	* @param zRangeMin  [in] z range min value
	* @param zRangeMax  [in] z range max value
	* @param filePath   [in] path for the outputted file
	* @param fileName   [in] name for the outputted file
	* @return nothing */
	static void saveDataForPlot(const vectorVectorFloat& vectorData,
		                        float                    multiplier,
								float                    minValue,
								float                    maxValue,
								float                    zRangeMin,
								float                    zRangeMax,
		                        const string&            filePath,
		                        const string&            fileName);

	/** Computes the minimum and maximum values of the texture information present in vectorData
	* @param vectorData [in]    vector with the texture information
	* @param min        [inout] minimum value in vectorData
	* @param max        [inout] maximum value in vectorData
	*@return nothing */
	static void getMinAndMaxValues(const vectorVectorFloat& vectorData, float& min, float& max);

	static vectorVectorFloat m_vectorFirstPass;         //!< Vector with the first lighting texture information to compare
	static vectorVectorFloat m_vectorSecondPass;        //!< Vector with the second lighting texture information to compare
	static bool              m_firstPassDataSet;        //!< True if the information of the first pass to compare (m_vectorFirstPass) has been set
	static bool              m_secondPassDataSet;       //!< True if the information of the second pass to compare (m_vectorSecondPass) has been set
	static uint              m_outputTexturePlotWidth;  //!< If outputting data to generate plots, width of the texture to generate
	static uint              m_outputTexturePlotHeight; //!< If outputting data to generate plots, height of the texture to generate
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _LIGHTINGVERIFICATIONHELPER_H_
