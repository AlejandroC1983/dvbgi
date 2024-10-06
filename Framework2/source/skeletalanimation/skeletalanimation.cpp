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
#include "../../include/skeletalanimation/skeletalanimation.h"
#include "../../include/util/loopmacrodefines.h"

// NAMESPACE

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

// TODO initialize properly
SkeletalAnimation::SkeletalAnimation(string &&name) : GenericResource(move(name), move(string("SkeletalAnimation")), GenericResourceType::GRT_SKELETALANIMATION)
	, m_duration(0.0f)
	, m_ticksPerSecond(0.0f)
	, m_numChannels(0)
	, m_numKeys(0)
	, m_timeDifference(0)
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

SkeletalAnimation::~SkeletalAnimation()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

int getNumSpacesAdded(float value)
{
	if (value < 0.0f)
	{
		if (value > -10.0f)
		{
			return 2;
		}
		else if (value > -100.0f)
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		if (value < 10.0f)
		{
			return 3;
		}
		else if (value < 100.0f)
		{
			return 2;
		}
		else
		{
			return 1;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void SkeletalAnimation::printKeyPositionData()
{
	cout << std::setprecision(4) << std::fixed;

	cout << "------------------------------------------------------------------------------------------------------------" << endl;
	cout << "Animation: " << m_name << endl;
	cout << "\t duration: " << m_duration << endl;
	cout << "\t ticksPerSecond: " << m_ticksPerSecond << endl;
	cout << "\t numChannels: " << m_numChannels << endl;
	cout << "\t numKeys: " << m_numKeys << endl;

	int maxLength = 0;
	int currentLength;
	for (auto element : m_mapBoneVectorPositionKey)
	{
		currentLength = element.first.length();

		if (currentLength > maxLength)
		{
			maxLength = currentLength;
		}
	}

	for (auto element : m_mapBoneVectorPositionKey)
	{
		cout << "\t\t Information for bone " << std::setw(maxLength) << element.first << " ";

		int numSpace;
		vec3 positionInitial         = element.second.size() > 0 ? element.second[0].m_position : vec3(0.0);
		float maxDistanceBetweenKeys = 0.0f;

		forI(element.second.size())
		{
			vec3 value         = element.second[i].m_position;
			vec3 previousValue = (i > 0) ? element.second[i - 1].m_position : vec3(0.0);

			if (distance(positionInitial, value) > maxDistanceBetweenKeys)
			{
				maxDistanceBetweenKeys = distance(positionInitial, value);
			}

			cout << "Position " << i << " (";

			numSpace = getNumSpacesAdded(value.x);

			forJ(numSpace)
			{
				cout << " ";
			}
			
			cout << value.x << ",";
			
			numSpace = getNumSpacesAdded(value.y);

			forJ(numSpace)
			{
				cout << " ";
			}

			cout << value.y << ",";
			
			numSpace = getNumSpacesAdded(value.z);

			forJ(numSpace)
			{
				cout << " ";
			}

			cout << value.z << ") ";

			if (i > 0)
			{
				float distanceWithPeviousValue = distance(value, previousValue);

				numSpace = getNumSpacesAdded(distanceWithPeviousValue);

				cout << " Distance with previous value ";

				forJ(numSpace)
				{
					cout << " ";
				}

				cout << distanceWithPeviousValue << " | ";
			}
		}

		cout << " Maximum distance between keys: ";

		numSpace = getNumSpacesAdded(maxDistanceBetweenKeys);

		forJ(numSpace)
		{
			cout << " ";
		}

		cout << maxDistanceBetweenKeys;

		cout << endl;
	}
	cout << "------------------------------------------------------------------------------------------------------------" << endl;

	cout << std::defaultfloat;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void SkeletalAnimation::duplicateKeyInformation(string&& baseBone, int numberCopies)
{
	map<string, vector<SkeletalAnimationKey>>::iterator it = m_mapBoneVectorPositionKey.find(baseBone);

	if (it == m_mapBoneVectorPositionKey.end())
	{
		cout << "ERROR: No bone " << baseBone << " found in SkeletalAnimation::duplicateKeyInformation" << endl;
		return;
	}

	vector<SkeletalAnimationKey>& vectorData = it->second;
	const int numKey                         = vectorData.size();
	vec3 firstKeyPosition                    = vectorData[0].m_position;
	vec3 lastKeyPosition                     = vectorData[numKey - 1].m_position;
	vec3 vectorFirstToLast                   = lastKeyPosition - firstKeyPosition;
	vec3 meanOffset                          = vectorFirstToLast / float(numKey);
	
	// Duplicate all keys for all bones, for those bone key tracks different from the base bone, duplication will be enough
	forIT(m_mapBoneVectorPositionKey)
	{
		(*it).second.resize(numKey * (numberCopies + 1));
		forJ(numKey * numberCopies)
		{
			(*it).second[j + numKey] = (*it).second[j];
		}
	}

	// For the bone equal to the baseBone, duplicate information adding the correct offset to the position field, following the same operations in m_vectorTime
	m_vectorTime.resize(numKey * (numberCopies + 1));
	forIFrom(numKey, numKey + (numKey * numberCopies))
	{
		m_vectorTime[i] = m_vectorTime[i - 1] + m_timeDifference;
	}

	// The offset from the last key until the first key of the new set of duplicated keys is the mean offset (as there is no real data to use)

	// In the new set of duplicated keys, the offset from the second until the last key is the original offset plus vectorFirstToLast
	forI(numberCopies)
	{
		it->second[numKey + (i * numKey)].m_position = it->second[numKey + (i * numKey) - 1].m_position + meanOffset;

		forJFrom(1, numKey)
		{
			vec3 positionPrevious              = it->second[j - 1].m_position;
			vec3 positionCurrent               = it->second[j + 0].m_position;
			vec3 offset                        = positionCurrent - positionPrevious;
			int indexWrite                     = j + numKey + (numKey * i);
			it->second[indexWrite].m_position += vectorFirstToLast * float(i + 1) + meanOffset * float(i) + offset;
		}
	}

	m_numKeys  *= (1 + numberCopies);
	m_duration *= 1.0f + float(numberCopies);
}

/////////////////////////////////////////////////////////////////////////////////////////////
