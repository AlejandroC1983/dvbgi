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

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/texture/irradiancetexture.h"

// NAMESPACE

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

IrradianceTexture::IrradianceTexture()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

IrradianceTexture::IrradianceTexture(string&& path)
{
	ifstream file(path, ios::in | ios::binary);

	file.read((char*)(&m_width),  sizeof(uint));
	file.read((char*)(&m_height), sizeof(uint));
	file.read((char*)(&m_depth),  sizeof(uint));

	uint i;
	m_vectorDistance.resize(m_depth);
	for (i = 0; i < m_depth; ++i)
	{
		file.read((char*)(&m_vectorDistance[i]), sizeof(float));
	}

	m_size = 0;
	m_vectorTextureSize.resize(m_depth);
	for (i = 0; i < m_depth; ++i)
	{
		file.read((char*)(&m_vectorTextureSize[i]), sizeof(uint));
		m_size += m_vectorTextureSize[i];
	}

	m_vectorTextureData.resize(m_depth);
	for (i = 0; i < m_depth; ++i)
	{
		m_vectorTextureData[i].resize(m_vectorTextureSize[i]);
		file.read((char*)(m_vectorTextureData[i].data()), sizeof(float) * m_vectorTextureSize[i]);
	}

	for (i = 0; i < m_depth; ++i)
	{
		m_vectorTextureFullData.insert(m_vectorTextureFullData.end(), m_vectorTextureData[i].begin(), m_vectorTextureData[i].end());
	}

	m_size *= sizeof(float);

	file.close();
}

/////////////////////////////////////////////////////////////////////////////////////////////
