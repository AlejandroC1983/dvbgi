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
#include "../../include/pipeline/pipeline.h"
#include "../../include/core/coremanager.h"
#include "../../include/parameter/attributedefines.h"
#include "../../include/parameter/attributedata.h"

// NAMESPACE
using namespace attributedefines;

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

Pipeline::Pipeline() : GenericResource(move(string("")), move(string("Pipeline")), GenericResourceType::GRT_PIPELINE)
	, m_pipeline(VK_NULL_HANDLE)
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

Pipeline::Pipeline(string &&name) : GenericResource(move(name), move(string("Pipeline")), GenericResourceType::GRT_PIPELINE)
	, m_pipeline(VK_NULL_HANDLE)
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

Pipeline::~Pipeline()
{
	destroyResources();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Pipeline::setDefaultValues()
{
	m_pipelineData.setDefaultValues();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Pipeline::destroyResources()
{
	if (m_pipeline != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(coreM->getLogicalDevice(), m_pipeline, NULL);

		m_pipeline = VK_NULL_HANDLE;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Pipeline::setPipelineLayout(const VkPipelineLayout& pipelineLayout)
{
	m_pipelineData.m_pipelineInfo.layout = pipelineLayout;
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Pipeline::buildPipeline()
{
	if (vkCreateGraphicsPipelines(coreM->getLogicalDevice(), gpuPipelineM->getPipelineCache(), 1, &m_pipelineData.m_pipelineInfo, NULL, &m_pipeline) == VK_SUCCESS)
	{
		return true;
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Pipeline::setPipelineShaderStage(const vector<VkPipelineShaderStageCreateInfo>& arrayPipelineShaderStage)
{
	m_pipelineData.m_pipelineInfo.pStages    = arrayPipelineShaderStage.data();
	m_pipelineData.m_pipelineInfo.stageCount = (uint32_t)arrayPipelineShaderStage.size();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Pipeline::setBasePipelineIndex(int index)
{
	m_pipelineData.m_pipelineInfo.basePipelineIndex = index;
}

/////////////////////////////////////////////////////////////////////////////////////////////
