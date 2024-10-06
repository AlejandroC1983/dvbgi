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
#include "../../include/framebuffer/framebuffer.h"
#include "../../include/core/coremanager.h"
#include "../../include/parameter/attributedefines.h"
#include "../../include/texture/texture.h"
#include "../../include/texture/texturemanager.h"

// NAMESPACE
using namespace attributedefines;

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

Framebuffer::Framebuffer(string &&name) : GenericResource(move(name), move(string("Framebuffer")), GenericResourceType::GRT_FRAMEBUFFER)
	, m_width(0)
	, m_height(0)
	, m_renderPass(nullptr)
	, m_has2DArrayTexture(false)
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

Framebuffer::~Framebuffer()
{
	destroyFramebufferResources();
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Framebuffer::attachmentResourceIsUsed(const string& resourceName)
{
	return (find(m_arrayAttachmentName.begin(), m_arrayAttachmentName.end(), resourceName) != m_arrayAttachmentName.end());
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Framebuffer::destroyFramebufferResources()
{
	vkDestroyFramebuffer(coreM->getLogicalDevice(), m_framebuffer, NULL);
	m_framebuffer = VK_NULL_HANDLE;
	m_renderPass  = nullptr;
	m_arrayAttachment.clear();
}

/////////////////////////////////////////////////////////////////////////////////////////////
