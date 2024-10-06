/*
Copyright 2016 Alejandro Cosin

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

#ifndef _ATTRIBUTEDEFINES_H_
#define _ATTRIBUTEDEFINES_H_

// GLOBAL INCLUDES
#include "../headers.h"
#include "../../include/commonnamespace.h"

// PROJECT INCLUDES

// NAMESPACE
using namespace commonnamespace;

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

/** Namespace where all the attributes that can be used in the intitalization of any class that inherits from the
* GenericResource class are placed. A string and its corresponding hashed value should be the typical pairs of data. */

namespace attributedefines
{
	// Pipeline building parameters
	extern const char* g_frontFace;
	extern const char* g_cullMode;
	extern const char* g_depthClampEnable;
	extern const char* g_depthBiasEnable;
	extern const char* g_depthBiasConstantFactor;
	extern const char* g_depthBiasClamp;
	extern const char* g_depthBiasSlopeFactor;
	extern const char* g_blendEnable;
	extern const char* g_alphaColorBlendOp;
	extern const char* g_sourceAlphaColorBlendFactor;
	extern const char* g_destinationAlphaColorBlendFactor;
	extern const char* g_colorWriteMask;
	extern const char* g_lineWidth;
	extern const char* g_depthTestEnable;
	extern const char* g_depthWriteEnable;
	extern const char* g_depthCompareOp;
	extern const char* g_depthBoundsTestEnable;
	extern const char* g_stencilTestEnable;
	extern const char* g_backFailOp;
	extern const char* g_backPassOp;
	extern const char* g_backCompareOp;
	extern const char* g_backCompareMask;
	extern const char* g_backReference;
	extern const char* g_backDepthFailOp;
	extern const char* g_backWriteMask;
	extern const char* g_minDepthBounds;
	extern const char* g_maxDepthBounds;
	extern const char* g_pipelineData;

	// Pipeline building parameters
	extern const char* g_pipelineShaderStage;
	extern const char* g_pipelineLayout;

	// Pipeline building hashed parameters
	extern const uint g_frontFaceHashed;
	extern const uint g_cullModeHashed;
	extern const uint g_depthClampEnableHashed;
	extern const uint g_depthBiasEnableHashed;
	extern const uint g_depthBiasConstantFactorHashed;
	extern const uint g_depthBiasClampHashed;
	extern const uint g_depthBiasSlopeFactorHashed;
	extern const uint g_blendEnableHashed;
	extern const uint g_alphaColorBlendOpHashed;
	extern const uint g_sourceAlphaColorBlendFactorHashed;
	extern const uint g_destinationAlphaColorBlendFactorHashed;
	extern const uint g_colorWriteMaskHashed;
	extern const uint g_lineWidthHashed;
	extern const uint g_depthTestEnableHashed;
	extern const uint g_depthWriteEnableHashed;
	extern const uint g_depthCompareOpHashed;
	extern const uint g_depthBoundsTestEnableHashed;
	extern const uint g_stencilTestEnableHashed;
	extern const uint g_backFailOpHashed;
	extern const uint g_backPassOpHashed;
	extern const uint g_backCompareOpHashed;
	extern const uint g_backCompareMaskHashed;
	extern const uint g_backReferenceHashed;
	extern const uint g_backDepthFailOpHashed;
	extern const uint g_backWriteMaskHashed;
	extern const uint g_minDepthBoundsHashed;
	extern const uint g_maxDepthBoundsHashed;
	extern const uint g_pipelineDataHashed;
	extern const uint g_pipelineDataHashed;

	// Pipeline building hashed parameters
	extern const uint g_pipelineShaderStageHashed;
	extern const uint g_pipelineLayoutHashed;

	// Render pass building parameters
	extern const char* g_renderPassAttachmentFormat;
	extern const char* g_renderPassAttachmentSamplesPerPixel;
	extern const char* g_renderPassAttachmentFinalLayout;
	extern const char* g_renderPassAttachmentColorReference;
	extern const char* g_renderPassAttachmentDepthReference;
	extern const char* g_renderPassAttachmentPipelineBindPoint;
	extern const char* g_renderPassAttachmentLoadOp;

	// Render pass building hashed parameters
	extern const uint g_renderPassAttachmentFormatHashed;
	extern const uint g_renderPassAttachmentSamplesPerPixelHashed;
	extern const uint g_renderPassAttachmentFinalLayoutHashed;
	extern const uint g_renderPassAttachmentColorReferenceHashed;
	extern const uint g_renderPassAttachmentDepthReferenceHashed;
	extern const uint g_renderPassAttachmentPipelineBindPointHashed;
	extern const uint g_renderPassAttachmentLoadOpHashed;

	// Manager template names
	extern const char* g_textureManager;
	extern const char* g_bufferManager;
	extern const char* g_shaderManager;
	extern const char* g_framebufferManager;
	extern const char* g_inputSingleton;
	extern const char* g_sceneSingleton;
	extern const char* g_pipelineManager;
	extern const char* g_uniformBufferManager;
	extern const char* g_materialManager;
	extern const char* g_rasterTechniqueManager;
	extern const char* g_renderPassManager;
	extern const char* g_cameraManager;
	extern const char* g_accelerationStructureManager;
	extern const char* g_componentManager;
	extern const char* g_skeletalAnimationManager;

	// Reflectance and normal material texture resource names
	extern const char* g_reflectanceTextureResourceName;
	extern const char* g_normalTextureResourceName;

	// Buffer name with the total amount of elements to process in a vkCmdDispatchIndirect command and
	// buffer where to write the VkDispatchIndirectCommand struct for the command
	extern const char* g_dispatchIndirectTotalNumberBufferName;
	extern const char* g_dispatchIndirectStructDataBufferName;

	// Hashed names of the buffers names
	extern const uint g_dispatchIndirectTotalNumberBufferNameHashed;
	extern const uint g_dispatchIndirectStructDataBufferNameHashed;

	// Reflectance and normal material texture resource hashed names
	extern const uint g_reflectanceTextureResourceNameHashed;
	extern const uint g_normalTextureResourceNameHashed;

	// Lit voxel compute shader source code chunk
	extern const char* g_litVoxelCodeChunk;

	// Lit voxel compute shader source code chunk hashed name
	extern const uint g_litVoxelCodeChunkHashed;

	// Light bounce voxel irradiance field compute shader source code chunk
	extern const char* g_lightBounceVoxelIrradianceCodeChunk;

	// Light bounce voxel irradiance field compute shader source code chunk hashed name
	extern const uint g_lightBounceVoxelIrradianceCodeChunkHashed;

	// Light bounce voxel Gaussian filter compute shader source code chunk
	extern const char* g_lightBounceVoxelGaussianFilterCodeChunk;

	// Light bounce voxel Gaussian filter compute shader source code chunk hashed name
	extern const uint g_lightBounceVoxelGaussianFilterCodeChunkHashed;

	// Build voxel shadow map geometry compute shader source code chunk
	extern const char* g_buildVoxelShadowMapGeometryCodeChunk;

	// Build voxel shadow map geometry compute shader source code chunk hashed name
	extern const uint g_buildVoxelShadowMapGeometryCodeChunkHashed;

	// Camera visible voxel compute shader source code chunk
	extern const char* g_cameraVisibleVoxelCodeChunk;

	// Camera visible voxel compute shader source code chunk hashed
	extern const uint g_cameraVisibleVoxelCodeChunkHashed;

	// Distance shadow mapping distance texture name compute shader source code chunk
	extern const char* g_distanceShadowMapDistanceTextureCodeChunk;

	// Distance shadow mapping distance texture name compute shader source code chunk hashed
	extern const uint g_distanceShadowMapDistanceTextureCodeChunkHashed;

	// Distance shadow mapping offscreen texture name compute shader source code chunk
	extern const char* g_distanceShadowMapOffscreenTextureCodeChunk;

	// Distance shadow mapping offscreen texture name compute shader source code chunk hashed
	extern const uint g_distanceShadowMapOffscreenTextureCodeChunkHashed;

	// Distance shadow mapping material name compute shader source code chunk
	extern const char* g_distanceShadowMapMaterialNameCodeChunk;

	// Distance shadow mapping material name compute shader source code chunk hashed
	extern const uint g_distanceShadowMapMaterialNameCodeChunkHashed;

	// Distance shadow mapping framebuffer name compute shader source code chunk
	extern const char* g_distanceShadowMapFramebufferNameCodeChunk;

	// Distance shadow mapping framebuffer name compute shader source code chunk hashed
	extern const uint g_distanceShadowMapFramebufferNameCodeChunkHashed;

	// Distance shadow mapping camera name compute shader source code chunk
	extern const char* g_distanceShadowMapCameraNameCodeChunk;

	// Distance shadow mapping camera name compute shader source code chunk hashed
	extern const uint g_distanceShadowMapCameraNameCodeChunkHashed;

	// Voxel visibility compute shader source code chunk
	extern const char* g_voxelVisibilityCodeChunk;

	// Voxel visibility compute shader source code chunk hashed name
	extern const uint g_voxelVisibilityCodeChunkHashed;

	// Voxel face index stream compaction shader source code chunk
	extern const char* g_voxelFaceIndexStreamCompactionCodeChunk;

	// Voxel face index stream compaction shader source code chunk hashed name
	extern const uint g_voxelFaceIndexStreamCompactionCodeChunkHashed;

	// Voxel face visible filtering compute shader source code chunk
	extern const char* g_voxelFacePenaltyCodeChunk;

	// Voxel face visible filtering compute shader source code chunk hashed name
	extern const uint g_voxelFacePenaltyCodeChunkHashed;

	// Distance shadow mapping indirect command buffer name code chunk
	extern const char* g_indirectCommandBufferCodeChunk;

	// Distance shadow mapping indirect command buffer name code chunk hashed
	extern const uint g_indirectCommandBufferCodeChunkHashed;

	// Compute frustum culling compute shader source code chunk
	extern const char* g_computeFrustumCullingCodeChunk;

	// Compute frustum culling compute shader source code chunk hashed name
	extern const uint g_computeFrustumCullingCodeChunkHashed;

	// Generic shader source code chunk
	extern const char* g_shaderCodeChunk;

	// Generic shader source code chunk hashed name
	extern const uint g_shaderCodeChunkHashed;

	// Material surface type
	extern const char* g_materialSurfaceType;

	// Material surface type hashed name
	extern const uint g_materialSurfaceTypeChunkHashed;

	// Vector with the scene nodes to be used for the scene ray trace shader
	extern const char* g_rayTraceVectorSceneNode;

	// Vector with the scene nodes to be used for the scene ray trace shader code chunk
	extern const uint g_rayTraceVectorSceneNodeHashed;
}

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _ATTRIBUTEDEFINES_H_
