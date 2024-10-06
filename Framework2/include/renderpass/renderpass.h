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

#ifndef _RENDERPASS_H_
#define _RENDERPASS_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/util/genericresource.h"

// CLASS FORWARDING

// NAMESPACE

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

class RenderPass : public GenericResource
{
	friend class RenderPassManager;

protected:
	/** Parameter constructor
	* @param [in] shader's name
	* @return nothing */
	RenderPass(string &&name);

	/** Destructor
	* @return nothing */
	virtual ~RenderPass();

	/** Updates the member variables with the parameters present in parent member variable m_parameterData
	* @return nothing */
	void updateRenderPassParameters();

	/** Builds the render pass m_renderPass member variable with the data present in the class member variables 
	* @return nothing */
	void buildRenderPass();

public:
	GET(VkRenderPass, m_renderPass, RenderPass)
	REF_RETURN_PTR(VkRenderPass, m_renderPass, RenderPass)

protected:
	VkAttachmentReference           m_depthReference;                  //!< Attachment reference data for the depth attachment in the render pass
	VkPipelineBindPoint             m_pipelineBindPoint;               //!< Pipeline bind point enum
	VkRenderPass                    m_renderPass;                      //!< Vulkan render pass built element handler
	vector<VkFormat>                m_vectorAttachmentFormat;          //!< Vector with the format of each one of the render pass attachments
	vector<VkSampleCountFlagBits>   m_vectorAttachmentSamplesPerPixel; //!< Vector with the number of samples per pixels of each attachment in the render pass
	vector<VkImageLayout>           m_vectorAttachmentFinalLayout;     //!< Vector with the final layout to apply to each one of the render pass attachments
	vector<VkAttachmentReference>   m_vectorColorReference;            //!< Vector with the attachment reference data for each one of the color attachments in the render pass
	vector<VkAttachmentLoadOp>      m_vectorAttachmentLoadOp;          //!< Vector with the load op for the attachment descriptions, if not defined, VK_ATTACHMENT_LOAD_OP_CLEAR will be used
	bool                            m_hasDepthAttachment;              //!< True if the render pass has depth attachment, false otherwise
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _RENDERPASS_H_
