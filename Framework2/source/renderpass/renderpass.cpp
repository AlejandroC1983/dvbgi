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
#include "../../include/renderpass/renderpass.h"
#include "../../include/parameter/attributedefines.h"
#include "../../include/parameter/attributedata.h"
#include "../../include/core/coremanager.h"

// NAMESPACE
using namespace attributedefines;

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

RenderPass::RenderPass(string &&name) : GenericResource(move(name), move(string("RenderPass")), GenericResourceType::GRT_RENDERPASS)
	, m_depthReference({})
	, m_pipelineBindPoint(VK_PIPELINE_BIND_POINT_MAX_ENUM)
	, m_renderPass(VK_NULL_HANDLE)
	, m_hasDepthAttachment(false)
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

RenderPass::~RenderPass()
{
	vkDestroyRenderPass(coreM->getLogicalDevice(), m_renderPass, NULL);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void RenderPass::updateRenderPassParameters()
{
	if (m_parameterData->elementExists(g_renderPassAttachmentFormatHashed))
	{
		AttributeData<vector<VkFormat>*>* attribute = m_parameterData->getElement<AttributeData<vector<VkFormat>*>*>(g_renderPassAttachmentFormatHashed);
		m_vectorAttachmentFormat = (*attribute->m_data);
		m_parameterData->removeElement(g_renderPassAttachmentFormatHashed);
	}

	if (m_parameterData->elementExists(g_renderPassAttachmentSamplesPerPixelHashed))
	{
		AttributeData<vector<VkSampleCountFlagBits>*>* attribute = m_parameterData->getElement<AttributeData<vector<VkSampleCountFlagBits>*>*>(g_renderPassAttachmentSamplesPerPixelHashed);
		m_vectorAttachmentSamplesPerPixel = (*attribute->m_data);
		m_parameterData->removeElement(g_renderPassAttachmentSamplesPerPixelHashed);
	}

	if (m_parameterData->elementExists(g_renderPassAttachmentFinalLayoutHashed))
	{
		AttributeData<vector<VkImageLayout>*>* attribute = m_parameterData->getElement<AttributeData<vector<VkImageLayout>*>*>(g_renderPassAttachmentFinalLayoutHashed);
		m_vectorAttachmentFinalLayout = (*attribute->m_data);
		m_parameterData->removeElement(g_renderPassAttachmentFinalLayoutHashed);
	}

	if (m_parameterData->elementExists(g_renderPassAttachmentColorReferenceHashed))
	{
		AttributeData<vector<VkAttachmentReference>*>* attribute = m_parameterData->getElement<AttributeData<vector<VkAttachmentReference>*>*>(g_renderPassAttachmentColorReferenceHashed);
		m_vectorColorReference = (*attribute->m_data);
		m_parameterData->removeElement(g_renderPassAttachmentColorReferenceHashed);
	}

	if (m_parameterData->elementExists(g_renderPassAttachmentDepthReferenceHashed))
	{
		AttributeData<VkAttachmentReference*>* attribute = m_parameterData->getElement<AttributeData<VkAttachmentReference*>*>(g_renderPassAttachmentDepthReferenceHashed);
		m_depthReference = (*attribute->m_data);
		m_parameterData->removeElement(g_renderPassAttachmentDepthReferenceHashed);
		m_hasDepthAttachment = true;
	}

	if (m_parameterData->elementExists(g_renderPassAttachmentPipelineBindPointHashed))
	{
		AttributeData<VkPipelineBindPoint*>* attribute = m_parameterData->getElement<AttributeData<VkPipelineBindPoint*>*>(g_renderPassAttachmentPipelineBindPointHashed);
		m_pipelineBindPoint = (*attribute->m_data);
		m_parameterData->removeElement(g_renderPassAttachmentPipelineBindPointHashed);
	}

	if (m_parameterData->elementExists(g_renderPassAttachmentLoadOpHashed))
	{
		AttributeData<vector<VkAttachmentLoadOp>*>* attribute = m_parameterData->getElement<AttributeData<vector<VkAttachmentLoadOp>*>*>(g_renderPassAttachmentLoadOpHashed);
		m_vectorAttachmentLoadOp = (*attribute->m_data);
		m_parameterData->removeElement(g_renderPassAttachmentLoadOpHashed);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void RenderPass::buildRenderPass()
{
	if ((m_vectorAttachmentFormat.size() != m_vectorAttachmentFinalLayout.size() ||
		(m_vectorAttachmentFormat.size() != m_vectorAttachmentSamplesPerPixel.size())))
	{
		cout << "ERROR: no the same number of attachment format and attachment final layout data in RenderPass::buildRenderPass" << endl;
		return;
	}

	vector<VkAttachmentDescription> attchmentDescriptions;
	uint attachmentNumber = uint(m_vectorAttachmentFormat.size());
	attchmentDescriptions.resize(attachmentNumber);
	forI(attachmentNumber)
	{
		attchmentDescriptions[i].format         = m_vectorAttachmentFormat[i];
		attchmentDescriptions[i].samples        = m_vectorAttachmentSamplesPerPixel[i];
		attchmentDescriptions[i].loadOp         = (m_vectorAttachmentLoadOp.size() > 0) ? m_vectorAttachmentLoadOp[i] : VK_ATTACHMENT_LOAD_OP_CLEAR;
		attchmentDescriptions[i].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
		attchmentDescriptions[i].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attchmentDescriptions[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

		VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		if ((attchmentDescriptions[i].loadOp == VK_ATTACHMENT_LOAD_OP_LOAD))
		{
			if (Texture::isDepthStencilTexture(m_vectorAttachmentFormat[i]))
			{
				initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			}
			else if (Texture::isDepthTexture(m_vectorAttachmentFormat[i]))
			{
				initialLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
			}
			else
			{
				initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			}
		}

		attchmentDescriptions[i].initialLayout  = initialLayout;
		attchmentDescriptions[i].finalLayout    = m_vectorAttachmentFinalLayout[i];
		attchmentDescriptions[i].flags          = 0; // Read doc to know if is really needed / the dependency can be found
	}

	VkSubpassDescription subpassDescription    = {};
	subpassDescription.pipelineBindPoint       = m_pipelineBindPoint;
	subpassDescription.colorAttachmentCount    = uint32_t(m_vectorColorReference.size());
	subpassDescription.pColorAttachments       = m_vectorColorReference.data();
	subpassDescription.pDepthStencilAttachment = m_hasDepthAttachment ? &m_depthReference : nullptr;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.pNext           = NULL;
	renderPassInfo.attachmentCount = uint32_t(attchmentDescriptions.size());
	renderPassInfo.pAttachments    = attchmentDescriptions.data();
	renderPassInfo.subpassCount    = 1;
	renderPassInfo.pSubpasses      = &subpassDescription;
	renderPassInfo.dependencyCount = 0;
	renderPassInfo.pDependencies   = NULL;

	VkResult result = vkCreateRenderPass(coreM->getLogicalDevice(), &renderPassInfo, nullptr, &m_renderPass);
	assert(result == VK_SUCCESS);
}

/////////////////////////////////////////////////////////////////////////////////////////////
