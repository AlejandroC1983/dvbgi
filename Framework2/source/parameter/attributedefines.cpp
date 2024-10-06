/*
Copyright 2014 Alejandro Cosin

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
#include "../../include/parameter/attributedefines.h"
#include "../../include/commonnamespace.h"

// NAMESPACE
using namespace commonnamespace;

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

namespace attributedefines
{
	// Pipeline building parameters
	const char* g_frontFace                             = "frontFace";
	const char* g_cullMode                              = "cullMode";
	const char* g_depthClampEnable                      = "depthClampEnable";
	const char* g_depthBiasEnable                       = "depthBiasEnable";
	const char* g_depthBiasConstantFactor               = "depthBiasConstantFactor";
	const char* g_depthBiasClamp                        = "depthBiasClamp";
	const char* g_depthBiasSlopeFactor                  = "depthBiasSlopeFactor";
	const char* g_blendEnable                           = "blendEnable";
	const char* g_alphaColorBlendOp                     = "alphaColorBlendOp";
	const char* g_sourceAlphaColorBlendFactor           = "sourceAlphaColorBlendFactor";
	const char* g_destinationAlphaColorBlendFactor      = "destinationAlphaColorBlendFactor";
	const char* g_colorWriteMask                        = "colorWriteMask";
	const char* g_lineWidth                             = "lineWidth";
	const char* g_depthTestEnable                       = "depthTestEnable";
	const char* g_depthWriteEnable                      = "depthWriteEnable";
	const char* g_depthCompareOp                        = "depthCompareOp";
	const char* g_depthBoundsTestEnable                 = "depthBoundsTestEnable";
	const char* g_stencilTestEnable                     = "stencilTestEnable";
	const char* g_backFailOp                            = "backFailOp";
	const char* g_backPassOp                            = "backPassOp";
	const char* g_backCompareOp                         = "backCompareOp";
	const char* g_backCompareMask                       = "backCompareMask";
	const char* g_backReference                         = "backReference";
	const char* g_backDepthFailOp                       = "backDepthFailOp";
	const char* g_backWriteMask                         = "backWriteMask";
	const char* g_minDepthBounds                        = "minDepthBounds";
	const char* g_maxDepthBounds                        = "maxDepthBounds";
	const char* g_pipelineData                          = "pipelineData";

	// Pipeline building parameters
	const char* g_pipelineShaderStage                   = "pipelineShaderStage";
	const char* g_pipelineLayout                        = "pipelineLayout";

	const uint g_frontFaceHashed                        = uint(hash<string>()(g_frontFace));
	const uint g_cullModeHashed                         = uint(hash<string>()(g_cullMode));
	const uint g_depthClampEnableHashed                 = uint(hash<string>()(g_depthClampEnable));
	const uint g_depthBiasEnableHashed                  = uint(hash<string>()(g_depthBiasEnable));
	const uint g_depthBiasConstantFactorHashed          = uint(hash<string>()(g_depthBiasConstantFactor));
	const uint g_depthBiasClampHashed                   = uint(hash<string>()(g_depthBiasClamp));
	const uint g_depthBiasSlopeFactorHashed             = uint(hash<string>()(g_depthBiasSlopeFactor));
	const uint g_blendEnableHashed                      = uint(hash<string>()(g_blendEnable));
	const uint g_alphaColorBlendOpHashed                = uint(hash<string>()(g_alphaColorBlendOp));
	const uint g_sourceAlphaColorBlendFactorHashed      = uint(hash<string>()(g_sourceAlphaColorBlendFactor));
	const uint g_destinationAlphaColorBlendFactorHashed = uint(hash<string>()(g_destinationAlphaColorBlendFactor));
	const uint g_colorWriteMaskHashed                   = uint(hash<string>()(g_colorWriteMask));
	const uint g_lineWidthHashed                        = uint(hash<string>()(g_lineWidth));
	const uint g_depthTestEnableHashed                  = uint(hash<string>()(g_depthTestEnable));
	const uint g_depthWriteEnableHashed                 = uint(hash<string>()(g_depthWriteEnable));
	const uint g_depthCompareOpHashed                   = uint(hash<string>()(g_depthCompareOp));
	const uint g_depthBoundsTestEnableHashed            = uint(hash<string>()(g_depthBoundsTestEnable));
	const uint g_stencilTestEnableHashed                = uint(hash<string>()(g_stencilTestEnable));
	const uint g_backFailOpHashed                       = uint(hash<string>()(g_backFailOp));
	const uint g_backPassOpHashed                       = uint(hash<string>()(g_backPassOp));
	const uint g_backCompareOpHashed                    = uint(hash<string>()(g_backCompareOp));
	const uint g_backCompareMaskHashed                  = uint(hash<string>()(g_backCompareMask));
	const uint g_backReferenceHashed                    = uint(hash<string>()(g_backReference));
	const uint g_backDepthFailOpHashed                  = uint(hash<string>()(g_backDepthFailOp));
	const uint g_backWriteMaskHashed                    = uint(hash<string>()(g_backWriteMask));
	const uint g_minDepthBoundsHashed                   = uint(hash<string>()(g_minDepthBounds));
	const uint g_maxDepthBoundsHashed                   = uint(hash<string>()(g_maxDepthBounds));
	const uint g_pipelineDataHashed                     = uint(hash<string>()(g_pipelineData));

	// Pipeline building hashed parameters
	const uint g_pipelineShaderStageHashed              = uint(hash<string>()(g_pipelineShaderStage));
	const uint g_pipelineLayoutHashed                   = uint(hash<string>()(g_pipelineLayout));

	// Render pass building parameters
	const char* g_renderPassAttachmentFormat            = "attachmentFormat";
	const char* g_renderPassAttachmentSamplesPerPixel   = "attachmentSamplesPerPixel";
	const char* g_renderPassAttachmentFinalLayout       = "attachmentFinalLayout";
	const char* g_renderPassAttachmentColorReference    = "attachmentColorReference";
	const char* g_renderPassAttachmentDepthReference    = "attachmentDepthReference";
	const char* g_renderPassAttachmentPipelineBindPoint = "attachmentPipelineBindPoint";
	const char* g_renderPassAttachmentLoadOp            = "attachmentLoadOp";

	// Render pass building hashed parameters
	const uint g_renderPassAttachmentFormatHashed            = uint(hash<string>()(g_renderPassAttachmentFormat));
	const uint g_renderPassAttachmentSamplesPerPixelHashed   = uint(hash<string>()(g_renderPassAttachmentSamplesPerPixel));
	const uint g_renderPassAttachmentFinalLayoutHashed       = uint(hash<string>()(g_renderPassAttachmentFinalLayout));
	const uint g_renderPassAttachmentColorReferenceHashed    = uint(hash<string>()(g_renderPassAttachmentColorReference));
	const uint g_renderPassAttachmentDepthReferenceHashed    = uint(hash<string>()(g_renderPassAttachmentDepthReference));
	const uint g_renderPassAttachmentPipelineBindPointHashed = uint(hash<string>()(g_renderPassAttachmentPipelineBindPoint));
	const uint g_renderPassAttachmentLoadOpHashed            = uint(hash<string>()(g_renderPassAttachmentLoadOp));

	// Manager template names
	const char* g_textureManager               = "textureManager";
	const char* g_bufferManager                = "bufferManager";
	const char* g_shaderManager                = "shaderManager";
	const char* g_framebufferManager           = "framebufferManager";
	const char* g_inputSingleton               = "inputSingleton";
	const char* g_sceneSingleton               = "sceneSingleton";
	const char* g_pipelineManager              = "pipelineManager";
	const char* g_uniformBufferManager         = "uniformBufferManager";
	const char* g_materialManager              = "materialManager";
	const char* g_rasterTechniqueManager       = "rasterTechniqueManager";
	const char* g_renderPassManager            = "renderPassManager";
	const char* g_cameraManager                = "cameraManager";
	const char* g_accelerationStructureManager = "accelerationStructureManager";
	const char* g_componentManager             = "componentManager";
	const char* g_skeletalAnimationManager     = "skeletalAnimationManager";

	// Reflectance and normal material texture resource names
	const char* g_reflectanceTextureResourceName   = "reflectanceTextureResource";
	const char* g_normalTextureResourceName        = "normalTextureResource";

	// Reflectance and normal material texture resource hashed names
	const uint g_reflectanceTextureResourceNameHashed   = uint(hash<string>()(g_reflectanceTextureResourceName));
	const uint g_normalTextureResourceNameHashed        = uint(hash<string>()(g_normalTextureResourceName));

	// Lit voxel compute shader source code chunk
	const char* g_litVoxelCodeChunk = "litVoxelCodeChunk";

	// Lit voxel compute shader source code chunk hashed name
	const uint g_litVoxelCodeChunkHashed = uint(hash<string>()(g_litVoxelCodeChunk));

	// Light bounce voxel irradiance field compute shader source code chunk
	const char* g_lightBounceVoxelIrradianceCodeChunk = "lightBounceVoxelIrradianceCodeChunk";

	// Light bounce voxel irradiance field compute shader source code chunk hashed name
	const uint g_lightBounceVoxelIrradianceCodeChunkHashed = uint(hash<string>()(g_lightBounceVoxelIrradianceCodeChunk));

	// Light bounce voxel Gaussian filter compute shader source code chunk
	const char* g_lightBounceVoxelGaussianFilterCodeChunk = "lightBounceVoxelGaussianFilterCodeChunk";

	// Light bounce voxel Gaussian filter compute shader source code chunk hashed name
	const uint g_lightBounceVoxelGaussianFilterCodeChunkHashed = uint(hash<string>()(g_lightBounceVoxelGaussianFilterCodeChunk));

	// Build voxel shadow map geometry compute shader source code chunk
	const char* g_buildVoxelShadowMapGeometryCodeChunk = "buildVoxelShadowMapGeometryCodeChunk";

	// Build voxel shadow map geometry compute shader source code chunk hashed name
	const uint g_buildVoxelShadowMapGeometryCodeChunkHashed = uint(hash<string>()(g_buildVoxelShadowMapGeometryCodeChunk));

	// Camera visible voxel compute shader source code chunk
	const char* g_cameraVisibleVoxelCodeChunk = "cameraVisibleVoxelCodeChunk";

	// Camera visible voxel compute shader source code chunk hashed
	const uint g_cameraVisibleVoxelCodeChunkHashed = uint(hash<string>()(g_cameraVisibleVoxelCodeChunk));

	// Distance shadow mapping distance texture name compute shader source code chunk
	const char* g_distanceShadowMapDistanceTextureCodeChunk = "distanceShadowMapDistanceTextureCodeChunk";

	// Distance shadow mapping distance texture name compute shader source code chunk hashed
	const uint g_distanceShadowMapDistanceTextureCodeChunkHashed = uint(hash<string>()(g_distanceShadowMapDistanceTextureCodeChunk));

	// Distance shadow mapping offscreen texture name compute shader source code chunk
	const char* g_distanceShadowMapOffscreenTextureCodeChunk = "distanceShadowMapOffscreenTextureCodeChunk";

	// Distance shadow mapping offscreen texture name compute shader source code chunk hashed
	const uint g_distanceShadowMapOffscreenTextureCodeChunkHashed = uint(hash<string>()(g_distanceShadowMapOffscreenTextureCodeChunk));

	// Distance shadow mapping material name compute shader source code chunk
	const char* g_distanceShadowMapMaterialNameCodeChunk = "distanceShadowMapMaterialNameCodeChunk";

	// Distance shadow mapping material name compute shader source code chunk hashed
	const uint g_distanceShadowMapMaterialNameCodeChunkHashed = uint(hash<string>()(g_distanceShadowMapMaterialNameCodeChunk));

	// Distance shadow mapping framebuffer name compute shader source code chunk
	const char* g_distanceShadowMapFramebufferNameCodeChunk = "distanceShadowMapFramebufferNameCodeChunk";

	// Distance shadow mapping framebuffer name compute shader source code chunk hashed
	const uint g_distanceShadowMapFramebufferNameCodeChunkHashed = uint(hash<string>()(g_distanceShadowMapFramebufferNameCodeChunk));

	// Distance shadow mapping camera name compute shader source code chunk
	const char* g_distanceShadowMapCameraNameCodeChunk = "distanceShadowMapCameraNameCodeChunk";

	// Distance shadow mapping camera name compute shader source code chunk hashed
	const uint g_distanceShadowMapCameraNameCodeChunkHashed = uint(hash<string>()(g_distanceShadowMapCameraNameCodeChunk));

	// Voxel visibility compute shader source code chunk
	const char* g_voxelVisibilityCodeChunk = "voxelVisibilityCodeChunk";

	// Voxel visibility compute shader source code chunk hashed name
	const uint g_voxelVisibilityCodeChunkHashed = uint(hash<string>()(g_voxelVisibilityCodeChunk));

	// Voxel face index stream compaction shader source code chunk
	const char* g_voxelFaceIndexStreamCompactionCodeChunk = "voxelFaceIndexStreamCompactionCodeChunk";

	// Voxel face index stream compaction shader source code chunk hashed name
	const uint g_voxelFaceIndexStreamCompactionCodeChunkHashed = uint(hash<string>()(g_voxelFaceIndexStreamCompactionCodeChunk));

	// Voxel face visible filtering compute shader source code chunk
	const char* g_voxelFacePenaltyCodeChunk = "voxelFacePenaltyCodeChunk";

	// Voxel face visible filtering compute shader source code chunk hashed name
	const uint g_voxelFacePenaltyCodeChunkHashed = uint(hash<string>()(g_voxelFacePenaltyCodeChunk));

	// Distance shadow mapping indirect command buffer name code chunk
	const char* g_indirectCommandBufferCodeChunk = "indirectCommandBufferCodeChunk";

	// Distance shadow mapping indirect command buffer name code chunk hashed
	const uint g_indirectCommandBufferCodeChunkHashed = uint(hash<string>()(g_indirectCommandBufferCodeChunk));

	// Compute frustum culling compute shader source code chunk
	const char* g_computeFrustumCullingCodeChunk = "computeFrustumCullingCodeChunk";

	// Compute frustum culling compute shader source code chunk hashed name
	const uint g_computeFrustumCullingCodeChunkHashed = uint(hash<string>()(g_computeFrustumCullingCodeChunk));

	// Generic shader source code chunk
	const char* g_shaderCodeChunk = "shaderCodeChunk";

	// Generic shader source code chunk hashed name
	const uint g_shaderCodeChunkHashed = uint(hash<string>()(g_shaderCodeChunk));

	// Material surface type
	const char* g_materialSurfaceType = "materialSurfaceType";

	// Material surface type hashed name
	const uint g_materialSurfaceTypeChunkHashed = uint(hash<string>()(g_materialSurfaceType));

	// Vector with the scene nodes to be used for the scene ray trace shader
	const char* g_rayTraceVectorSceneNode = "rayTraceVectorSceneNode";

	// Vector with the scene nodes to be used for the scene ray trace shader code chunk
	const uint g_rayTraceVectorSceneNodeHashed = uint(hash<string>()(g_rayTraceVectorSceneNode));

	// Buffer name with the total amount of elements to process in a vkCmdDispatchIndirect command and
	// buffer where to write the VkDispatchIndirectCommand struct for the command
	const char* g_dispatchIndirectTotalNumberBufferName = "dispatchIndirectTotalNumberBufferName";
	const char* g_dispatchIndirectStructDataBufferName = "dispatchIndirectStructDataBufferName";

	// Buffer name with the total amount of elements to process in a vkCmdDispatchIndirect command and
	// buffer where to write the VkDispatchIndirectCommand struct for the command
	const uint g_dispatchIndirectTotalNumberBufferNameHashed = uint(hash<string>()(g_dispatchIndirectTotalNumberBufferName));
	const uint g_dispatchIndirectStructDataBufferNameHashed = uint(hash<string>()(g_dispatchIndirectStructDataBufferName));
}

/////////////////////////////////////////////////////////////////////////////////////////////
