/*
Copyright 2017 Alejandro Cosin

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
#include "../../include/util/lightingverificationhelper.h"
#include "../../include/util/loopmacrodefines.h"
#include "../../include/util/vulkanstructinitializer.h"

// NAMESPACE

// DEFINES

// STATIC MEMBER INITIALIZATION
vectorVectorFloat LightingVerificationHelper::m_vectorFirstPass;
vectorVectorFloat LightingVerificationHelper::m_vectorSecondPass;
bool              LightingVerificationHelper::m_firstPassDataSet        = false;
bool              LightingVerificationHelper::m_secondPassDataSet       = false;
uint              LightingVerificationHelper::m_outputTexturePlotWidth  = 640;
uint              LightingVerificationHelper::m_outputTexturePlotHeight = 480;

/////////////////////////////////////////////////////////////////////////////////////////////

void LightingVerificationHelper::saveDataForPlot(const vectorVectorFloat& vectorData,
		                                         float                    multiplier,
												 float                    minValue,
												 float                    maxValue,
												 float                    zRangeMin,
												 float                    zRangeMax,
		                                         const string&            filePath,
		                                         const string&            fileName)
{
	// First, make the .dat file with the information from the texture
	ofstream file;
	file.open((filePath + fileName + ".dat").c_str());

	const uint numRow = uint(vectorData.size());

	if (numRow == 0)
	{
		cout << "ERROR: no information in vectorData in LightingVerificationHelper::saveDataForPlot" << endl;
	}

	uint numColumn;
	forI(numRow)
	{
		numColumn = uint(vectorData[i].size());
		forJ(numColumn)
		{
			file << i << "     " << j << "     " << setprecision(10) << vectorData[i][j] * multiplier << endl;
		}
	}

	file.close();

	// Second, make the .p file with the commands for gnuplot
	file.open((filePath + fileName + ".p").c_str());
	file << "set terminal pngcairo" << endl;
	file << "set output '" << fileName << ".png'" << endl;
	file << "set dgrid3d " << numRow << ", " << numColumn << ", 1" << endl;
	file << "set dummy u, v" << endl;
	file << "set pm3d implicit at s" << endl;
	file << "set hidden3d" << endl;
	file << "set key inside right top vertical Right noreverse enhanced autotitles box linetype - 1 linewidth 1.000" << endl;
	file << "set style data points" << endl;
	file << "set xrange [0:" << numRow - 1 << "]" << endl;
	file << "set yrange [0:" << numColumn - 1 << "]" << endl;
	file << "set zrange [" << zRangeMin << ":" << zRangeMax << "]" << endl;

	file << "set palette defined (";
	// Generate the color gradient values equally distributed
	string vecColor[10] = {
	"#67000d",
	"#a50f15",
	"#cb181d",
	"#ef3b2c",
	"#fb6a4a",
	"#fc9272",
	"#fcbba1",
	"#fee0d2",
	"#fff5f0",
	"white" };

	float span = (maxValue - minValue) * multiplier;
	float step = span / 10.0f;
	vectorFloat vecData;
	vecData.resize(10);
	forI(10)
	{
		vecData[i] = minValue + float(i) * step;
		file << setprecision(10) << vecData[i] << " " << "\"" << vecColor[i] << "\"";

		if (i < 9)
		{
			file << ", ";
		}
	}
	file << ")" << endl;
	file << "set nokey" << endl;
	file << "splot \"" << fileName << ".dat\"" << endl;
	file.close();

	// And last step, add the corresponding command line to data/plots/generateplots
	file.open((string("../data/plots/generateplots.sh")).c_str(), ios::app);
	file << "start ..\\\\..\\\\util\\\\gnuplot\\\\bin\\\\gnuplot.exe " << fileName << ".p" << endl;
	file.close();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void LightingVerificationHelper::getMinAndMaxValues(const vectorVectorFloat& vectorData, float& min, float& max)
{
	const uint numRow = uint(vectorData.size());

	if (numRow == 0)
	{
		cout << "ERROR: no information in vectorData in LightingVerificationHelper::getMinAndMaxValues" << endl;
	}

	min =         FLT_MAX;
	max = -1.0f * FLT_MAX;

	uint numColumn;
	forI(numRow)
	{
		numColumn = uint(vectorData[i].size());
		forJ(numColumn)
		{
			min = glm::min(min, vectorData[i][j]);
			max = glm::max(max, vectorData[i][j]);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
