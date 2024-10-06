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
#include "../../include/renderpass/renderpassmanager.h"
#include "../../include/renderpass/renderpass.h"
#include "../../include/parameter/attributedefines.h"
#include "../../include/core/coremanager.h"

// NAMESPACE
using namespace attributedefines;

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

RenderPassManager::RenderPassManager()
{
	m_managerName = g_renderPassManager;
}

/////////////////////////////////////////////////////////////////////////////////////////////

RenderPassManager::~RenderPassManager()
{
	destroyResources();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void RenderPassManager::destroyResources()
{
	forIT(m_mapElement)
	{
		delete it->second;
		it->second = nullptr;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

RenderPass* RenderPassManager::buildRenderPass(string&& instanceName, MultiTypeUnorderedMap* attributeData)
{
	if (existsElement(move(instanceName)))
	{
		return getElement(move(instanceName));
	}

	RenderPass* renderPass = new RenderPass(move(string(instanceName)));
	renderPass->m_parameterData = attributeData;
	renderPass->updateRenderPassParameters();
	renderPass->buildRenderPass();

	addElement(move(string(instanceName)), renderPass);
	renderPass->m_name = move(instanceName);
	renderPass->m_ready = true;

	coreM->setObjectName(uint64_t(renderPass->m_renderPass), VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT, renderPass->getName());

	return renderPass;
}

/////////////////////////////////////////////////////////////////////////////////////////////
