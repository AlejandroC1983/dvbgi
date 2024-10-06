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

#ifndef _PIPELINE_H_
#define _PIPELINE_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../headers.h"
#include "../../include/util/genericresource.h"
#include "../../include/pipeline/pipelinedata.h"

// CLASS FORWARDING

// NAMESPACE

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

class Pipeline : public GenericResource
{
	friend class PipelineManager;

public:
	/** Default constructor
	* @return nothing */
	Pipeline();

	/** Parameter constructor
	* @param [in] shader's name
	* @return nothing */
	Pipeline(string &&name);

	/** Destructor
	* @return nothing */
	virtual ~Pipeline();

	/** Sets default values to the member variables
	* @return nothing */
	void setDefaultValues();

	/** Destroys the resources allocated by this pipeline, if any
	* @return nothing */
	void destroyResources();

public:
	/** Sets the pipeline layout used in this pipeline before its built
	* @return nothing */
	void setPipelineLayout(const VkPipelineLayout& pipelineLayout);

	/** Builds the pipeline with the values present in the member variables, result is stored in m_pipeline
	* @return true if the pipeline was built successfully and false otherwise */
	bool buildPipeline();

	/** Sets the value of VkGraphicsPipelineCreateInfo::pStages
	* @param arrayPipelineShaderStage [in] value data to set
	* @return nothing */
	void setPipelineShaderStage(const vector<VkPipelineShaderStageCreateInfo>& arrayPipelineShaderStage);

	/** Sets the value of m_pipelineInfo.basePipelineIndex
	* @param index [in] value to assign to m_pipelineInfo.basePipelineIndex
	* @return nothing */
	void setBasePipelineIndex(int index);

	GET(VkPipeline, m_pipeline, Pipeline)
	REF(VkPipeline, m_pipeline, Pipeline)
	GET_SET(PipelineData, m_pipelineData, PipelineData)
	REF(PipelineData, m_pipelineData, PipelineData)

protected:
	PipelineData m_pipelineData; //!< Contains the information that will be used to build this pipeline
	VkPipeline   m_pipeline;     //!< Finally built pipeline
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _PIPELINE_H_
