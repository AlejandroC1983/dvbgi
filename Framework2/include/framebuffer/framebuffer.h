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

#ifndef _FRAMEBUFFER_H_
#define _FRAMEBUFFER_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../headers.h"
#include "../../include/util/genericresource.h"
#include "../../include/commonnamespace.h"

// CLASS FORWARDING

// NAMESPACE
using namespace commonnamespace;

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

class Framebuffer : public GenericResource
{
	friend class FramebufferManager;

protected:
	/** Parameter constructor
	* @param [in] shader's name
	* @return nothing */
	Framebuffer(string &&name);

	/** Destructor
	* @return nothing */
	virtual ~Framebuffer();

public:
	GETCOPY(uint32_t, m_width, Width)
	GETCOPY(uint32_t, m_height, Height)
	GET_PTR(VkRenderPass, m_renderPass, RenderPass)
	GETCOPY(VkFramebuffer, m_framebuffer, Framebuffer)
	GET(string, m_renderPassName, RenderPassName)
	GET(vector<string>, m_arrayAttachmentName, ArrayAttachmentName)
	GETCOPY(bool, m_has2DArrayTexture, Has2DArrayTexture)

protected:
	/** Returns true if the resource is used in the attachment vector
	* @param resourceName [in] resource name
	* @return true if the resource is used in the attachment vector, and flase otherwise */
	bool attachmentResourceIsUsed(const string& resourceName);

	/** Destorys m_framebuffer and cleans m_arrayAttachment and m_renderPass
	* @return nothing */
	void destroyFramebufferResources();

	uint32_t            m_width;               //!< Framebuffer width
	uint32_t            m_height;              //!< Framebuffer height
	VkRenderPass*       m_renderPass;          //!< Pointer to the render pass used to build this framebuffer
	vector<VkImageView> m_arrayAttachment;     //!< Vector with the attachments used in this framebuffer
	VkFramebuffer       m_framebuffer;         //!< Framebuffer object
	vector<string>      m_arrayAttachmentName; //!< Vector with the names of the attachments used in this framebuffer
	string              m_renderPassName;      //!< Name of the render pass to use for this framebuffer
	bool                m_has2DArrayTexture;   //!< flag to know if any of the texture attachments is a 2D array texture
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _FRAMEBUFFER_H_
