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
#include "../../include/framebuffer/framebuffermanager.h"
#include "../../include/framebuffer/framebuffer.h"
#include "../../include/core/coremanager.h"
#include "../../include/renderpass/renderpass.h"
#include "../../include/renderpass/renderpassmanager.h"
#include "../../include/texture/texture.h"
#include "../../include/texture/texturemanager.h"
#include "../../include/parameter/attributedefines.h"

// NAMESPACE
using namespace attributedefines;

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

FramebufferManager::FramebufferManager()
{
	m_managerName = g_framebufferManager;
}

/////////////////////////////////////////////////////////////////////////////////////////////

FramebufferManager::~FramebufferManager()
{
	destroyResources();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void FramebufferManager::destroyResources()
{
	forIT(m_mapElement)
	{
		delete it->second;
		it->second = nullptr;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

Framebuffer* FramebufferManager::buildFramebuffer(string&& instanceName, uint32_t width, uint32_t height, string&& renderPassName, vector<string>&& m_arrayAttachmentName)
{
	if (existsElement(move(instanceName)))
	{
		return getElement(move(instanceName));
	}

	Framebuffer* framebuffer           = new Framebuffer(move(string(instanceName)));
	framebuffer->m_width               = width;
	framebuffer->m_height              = height;
	framebuffer->m_renderPassName      = move(renderPassName);
	framebuffer->m_arrayAttachmentName = move(m_arrayAttachmentName);

	framebuffer->setReady(verifyCanBeBuilt(framebuffer));
	if (framebuffer->getReady())
	{
		bool result = buildFramebufferResources(framebuffer);
		framebuffer->setReady(result);
	}

	addElement(move(string(instanceName)), framebuffer);
	framebuffer->m_name = move(instanceName);

	coreM->setObjectName(uint64_t(framebuffer->m_framebuffer), VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT, framebuffer->getName());
	
	return framebuffer;
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool FramebufferManager::verifyCanBeBuilt(Framebuffer* framebuffer)
{
	bool result = true;

	// Update render pass data
	RenderPass* renderPass = renderPassM->getElement(move(string(framebuffer->m_renderPassName)));
	if (renderPass != nullptr)
	{
		framebuffer->m_renderPass = renderPass->refRenderPass();
	}
	else
	{
		framebuffer->m_renderPass = VK_NULL_HANDLE;
		result = false;
	}

	// Update attachment data
	uint numElement = uint(framebuffer->m_arrayAttachmentName.size());
	framebuffer->m_arrayAttachment.resize(numElement);
	memset(&framebuffer->m_arrayAttachment[0], VK_NULL_HANDLE, numElement * sizeof(VkImageView));
	forI(numElement)
	{
		Texture* texture = textureM->getElement(move(string(framebuffer->m_arrayAttachmentName[i])));
		if (texture != nullptr)
		{
			if (texture->getImageViewType() == VkImageViewType::VK_IMAGE_VIEW_TYPE_3D)
			{
				framebuffer->m_arrayAttachment[i] = texture->getView3DAttachment();
				framebuffer->m_has2DArrayTexture  = true;

				if ((texture->getWidth() != texture->getHeight()) || (texture->getWidth() != texture->getDepth()))
				{
					cout << "ERROR: 3D TEXTURE IN FramebufferManager::verifyCanBeBuilt TO BE USED AS FRMAEBUFFER ATTACHMENT DOES NOT HAVE THE SAME VALUES ACROSS ALL DIMENSIONS" << endl;
				}
			}
			else
			{
				framebuffer->m_arrayAttachment[i] = texture->getView();
			}
		}
		else
		{
			result = false;
		}
	}

	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool FramebufferManager::buildFramebufferResources(Framebuffer* framebuffer)
{
	if (framebuffer->getReady())
	{
		VkFramebufferCreateInfo fbInfo = {};
		fbInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fbInfo.pNext           = NULL;
		fbInfo.renderPass      = *framebuffer->getRenderPass();
		fbInfo.attachmentCount = uint32_t(framebuffer->m_arrayAttachment.size());
		fbInfo.pAttachments    = framebuffer->m_arrayAttachment.data();
		fbInfo.width           = framebuffer->m_width;
		fbInfo.height          = framebuffer->m_height;
		fbInfo.layers          = framebuffer->getHas2DArrayTexture() ? framebuffer->m_width : 1;

		VkResult result = vkCreateFramebuffer(coreM->getLogicalDevice(), &fbInfo, NULL, &framebuffer->m_framebuffer);
		assert(result == VK_SUCCESS);

		if (result == VK_SUCCESS)
		{
			return true;
		}
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void FramebufferManager::assignSlots()
{
	textureM->refElementSignal().connect<FramebufferManager, &FramebufferManager::slotElement>(this);
	renderPassM->refElementSignal().connect<FramebufferManager, &FramebufferManager::slotElement>(this);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void FramebufferManager::slotElement(const char* managerName, string&& elementName, ManagerNotificationType notificationType)
{
	if (!gpuPipelineM->getPipelineInitialized())
	{
		return;
	}

	switch (notificationType)
	{
		case ManagerNotificationType::MNT_ADDED:
		{
			vector<Framebuffer*> vectorFramebuffer = computeNotifyAffectedElements(managerName, move(string(elementName)), notificationType);

			forI(vectorFramebuffer.size())
			{
				if (verifyCanBeBuilt(vectorFramebuffer[i]))
				{
					bool result = buildFramebufferResources(vectorFramebuffer[i]);
					vectorFramebuffer[i]->setReady(result);
					if (result)
					{
						if (gpuPipelineM->getPipelineInitialized())
						{
							emitSignalElement(move(string(vectorFramebuffer[i]->getName())), ManagerNotificationType::MNT_CHANGED);
						}
					}
				}
			}

			break;
		}
		case ManagerNotificationType::MNT_REMOVED:
		{
			vector<Framebuffer*> vectorFramebuffer = computeNotifyAffectedElements(managerName, move(string(elementName)), notificationType);

			forI(vectorFramebuffer.size())
			{
				vectorFramebuffer[i]->destroyFramebufferResources();
				vectorFramebuffer[i]->setReady(false);
				vectorFramebuffer[i]->setDirty(true);
				if (gpuPipelineM->getPipelineInitialized())
				{
					emitSignalElement(move(string(vectorFramebuffer[i]->getName())), ManagerNotificationType::MNT_CHANGED);
				}
			}

			break;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

vector<Framebuffer*> FramebufferManager::computeNotifyAffectedElements(const char* managerName, string&& elementName, ManagerNotificationType notificationType)
{
	vector<Framebuffer*> vectorFramebuffer;

	if (managerName == g_textureManager)
	{
		Framebuffer* framebuffer;
		map<string, Framebuffer*>::iterator it = m_mapElement.begin();
		for (it; it != m_mapElement.end(); ++it)
		{
			framebuffer = it->second;
			if (framebuffer->attachmentResourceIsUsed(elementName))
			{
				vectorFramebuffer.push_back(framebuffer);
			}
		}
	}

	if (managerName == g_renderPassManager)
	{
		Framebuffer* framebuffer;
		map<string, Framebuffer*>::iterator it = m_mapElement.begin();
		for (it; it != m_mapElement.end(); ++it)
		{
			framebuffer = it->second;
			if (framebuffer->getRenderPassName() == elementName)
			{
				vectorFramebuffer.push_back(framebuffer);
			}
		}
	}

	return vectorFramebuffer;
}

/////////////////////////////////////////////////////////////////////////////////////////////
