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

#ifndef _PIPELINEDATA_H_
#define _PIPELINEDATA_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/util/getsetmacros.h"
#include "../headers.h"

// CLASS FORWARDING

// NAMESPACE

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

class PipelineData
{
	friend class Pipeline;

public:
	/** Default constructor
	* @return nothing */
	PipelineData();

	/** Sets default values to the member variables
	* @return nothing */
	void setDefaultValues();

	/** Sets as nullptr the field m_pipelineInfo::pDepthStencilState
	* @return nothing */
	void setNullDepthStencilState();

	/** Setter for m_pipelineInfo::renderPass
	* @param renderPass [in] value to set to m_pipelineInfo::renderPass
	* @return nothing */
	void setRenderPass(VkRenderPass renderPass);

	/** Update the m_arrayColorBlendAttachmentStateInfo array with the new data, and the corresponding fields in the color
	* blend state and pipeline create info structures
	* @param arrayData [in] new pipeline color blend attachment information
	* @return nothing */
	void updateArrayColorBlendAttachmentStateInfo(vector<VkPipelineColorBlendAttachmentState>& arrayData);

	/** Updates the values in m_colorBlendStateInfo::blendConstants with the new data, and the corresponding fields in the color
	* @param blendConstants [in] new blend constants value
	* @return nothing */
	void updateColorBlendStateBlendConstants(vec4 blendConstants);

	/** Update m_vertexInputStateInfo and the corresponding fields in m_pipelineInfo
	* @param vertexInputStateInfo [in] new pipeline vertex input state information
	* @return nothing */
	void updateVertexInputStateInfo(VkPipelineVertexInputStateCreateInfo& vertexInputStateInfo);

	/** Update m_rasterStateInfo and the corresponding fields in m_pipelineInfo
	* @param rasterStateInfo [in] new pipeline raster state information
	* @return nothing */
	void updateRasterStateInfo(VkPipelineRasterizationStateCreateInfo& rasterStateInfo);

	/** Update m_colorBlendStateInfo and the corresponding fields in m_pipelineInfo
	* @param colorBlendStateInfo [in] new color blend state information
	* @param arrayData           [in] color blend attachment information
	* @return nothing */
	void updateColorBlendStateInfo(VkPipelineColorBlendStateCreateInfo&         colorBlendStateInfo,
		                           vector<VkPipelineColorBlendAttachmentState>& arrayData);

	/** Update m_depthStencilStateInfo and the corresponding fields in m_pipelineInfo
	* @param depthStencilStateInfo [in] new depth stencil state information
	* @return nothing */
	void updateDepthStencilStateInfo(VkPipelineDepthStencilStateCreateInfo& depthStencilStateInfo);

	/** Update m_vertexInputStateInfo and the corresponding fields in m_pipelineInfo
	* @param inputAssemblyStateCreateInfo [in] new input assembly state create information
	* @return nothing */
	void updateinputAssemblyStateCreateInfo(VkPipelineInputAssemblyStateCreateInfo& inputAssemblyStateCreateInfo);

	/** Update m_dynamicState and the corresponding fields in m_pipelineInfo
	* @param vectorDynamicState [in] vector with the new input dynamic state create info flags
	* @return nothing */
	void updateDynamicStateCreateInfo(vector<VkDynamicState> vectorDynamicState);

	GET_SET(VkPipelineDynamicStateCreateInfo, m_dynamicState, DynamicState)
	GET_SET(VkPipelineVertexInputStateCreateInfo, m_vertexInputStateInfo, VertexInputStateInfo)
	GET_SET(VkPipelineInputAssemblyStateCreateInfo, m_inputAssemblyInfo, InputAssemblyInfo)
	GET_SET(VkPipelineRasterizationStateCreateInfo, m_rasterStateInfo, RasterStateInfo)
	GET(vector<VkPipelineColorBlendAttachmentState>, m_arrayColorBlendAttachmentStateInfo, ArrayColorBlendAttachmentStateInfo)
	GET_SET(VkPipelineColorBlendStateCreateInfo, m_colorBlendStateInfo, ColorBlendStateInfo)
	GET_SET(VkPipelineViewportStateCreateInfo, m_viewportStateInfo, ViewportStateInfo)
	GET_SET(VkPipelineDepthStencilStateCreateInfo, m_depthStencilStateInfo, DepthStencilStateInfo)
	GET_SET(VkPipelineMultisampleStateCreateInfo, m_multiSampleStateInfo, MultiSampleStateInfo)
	GET_SET(VkGraphicsPipelineCreateInfo, m_pipelineInfo, PipelineInfo)

protected:
	VkDynamicState                              m_dynamicStateEnables[9];             //!< Dynamic state information for the pipeline, viewport and scissor
	VkPipelineDynamicStateCreateInfo            m_dynamicState;                       //!< Information strcut for the dynamic state information for the pipeline
	VkPipelineVertexInputStateCreateInfo        m_vertexInputStateInfo;               //!< Information about the format and input of vertex for the vertex shaders
	VkPipelineInputAssemblyStateCreateInfo      m_inputAssemblyInfo;                  //!< Input topology used (triangles)
	VkPipelineRasterizationStateCreateInfo      m_rasterStateInfo;                    //!< Information about raster state (polygon fill, clockwiseness, depth bias,...)
	vector<VkPipelineColorBlendAttachmentState> m_arrayColorBlendAttachmentStateInfo; //!< Color blend information
	VkPipelineColorBlendStateCreateInfo         m_colorBlendStateInfo;                //!< More color blend information for the pipeline
	VkPipelineViewportStateCreateInfo           m_viewportStateInfo;                  //!< Number of viewports used
	VkPipelineDepthStencilStateCreateInfo       m_depthStencilStateInfo;              //!< Depth and stencil settings
	VkPipelineMultisampleStateCreateInfo        m_multiSampleStateInfo;               //!< Multisample settings
	VkGraphicsPipelineCreateInfo                m_pipelineInfo;                       //!< Pipeline building struct that holds most of the previous structs
};

/////////////////////////////////////////////////////////////////////////////////////////////

// INLINE METHODS

inline void PipelineData::setRenderPass(VkRenderPass renderPass)
{
	m_pipelineInfo.renderPass = renderPass;
}

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _PIPELINEDATA_H_
