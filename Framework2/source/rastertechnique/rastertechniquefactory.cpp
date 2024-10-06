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
#include "../../include/util/loopmacrodefines.h"
#include "../../include/util/factorytemplate.h"
#include "../../include/rastertechnique/rastertechniquefactory.h"
#include "../../include/rastertechnique/rastertechnique.h"
#include "../../include/rastertechnique/scenerastercolortexturetechnique.h"
#include "../../include/rastertechnique/scenevoxelizationtechnique.h"
#include "../../include/rastertechnique/bufferprefixsumtechnique.h"
#include "../../include/rastertechnique/scenelightingtechnique.h"
#include "../../include/rastertechnique/voxelrasterinscenariotechnique.h"
#include "../../include/rastertechnique/shadowmappingvoxeltechnique.h"
#include "../../include/rastertechnique/bufferprocesstechnique.h"
#include "../../include/rastertechnique/litvoxeltechnique.h"
#include "../../include/rastertechnique/lightbouncevoxelirradiancetechnique.h"
#include "../../include/rastertechnique/distanceshadowmappingtechnique.h"
#include "../../include/rastertechnique/antialiasingtechnique.h"
#include "../../include/rastertechnique/computefrustumcullingtechnique.h"
#include "../../include/rastertechnique/sceneindirectdrawtechnique.h"
#include "../../include/rastertechnique/sceneraytracingtechnique.h"
#include "../../include/rastertechnique/voxelvisibilityraytracingtechnique.h"
#include "../../include/rastertechnique/staticscenevoxelizationtechnique.h"
#include "../../include/rastertechnique/dynamicscenevoxelizationtechnique.h"
#include "../../include/rastertechnique/dynamicvoxelcopytobuffertechnique.h"
#include "../../include/rastertechnique/dynamicvoxelvisibilityraytracingtechnique.h"
#include "../../include/rastertechnique/resetlitvoxeldatatechnique.h"
#include "../../include/rastertechnique/cameravisibleraytracingtechnique.h"
#include "../../include/rastertechnique/processcameravisibleresultstechnique.h"
#include "../../include/rastertechnique/dynamiclightbouncevoxelirradiancetechnique.h"
#include "../../include/rastertechnique/clear3dtexturestechnique.h"
#include "../../include/rastertechnique/irradiancegaussianblurtechnique.h"
#include "../../include/rastertechnique/staticneighbourinformationtechnique.h"
#include "../../include/rastertechnique/skeletalanimationupdatetechnique.h"
#include "../../include/rastertechnique/scenelightingdeferredtechnique.h"
#include "../../include/rastertechnique/gbuffertechnique.h"
#include "../../include/rastertechnique/raytracingdeferredshadowstechnique.h"
#include "../../include/rastertechnique/lightbouncedynamicvoxelirradiancetechnique.h"

// DEFINES

// NAMESPACE

// STATIC MEMBER INITIALIZATION
map<string, ObjectFactory<RasterTechnique>*> FactoryTemplate<RasterTechnique>::m_mapFactory = []() { map<string, ObjectFactory<RasterTechnique>*> temporal; return temporal; } ();
// C++ guarantees static variables inside a compilation unit (.cpp) are initialized in order of declaration, so registering the types
// below will find the static map already initialized
REGISTER_TYPE(RasterTechnique, RasterTechnique)
REGISTER_TYPE(RasterTechnique, SceneRasterColorTextureTechnique)
REGISTER_TYPE(RasterTechnique, SceneVoxelizationTechnique)
REGISTER_TYPE(RasterTechnique, BufferPrefixSumTechnique)
REGISTER_TYPE(RasterTechnique, SceneLightingTechnique)
REGISTER_TYPE(RasterTechnique, VoxelRasterInScenarioTechnique)
REGISTER_TYPE(RasterTechnique, ShadowMappingVoxelTechnique)
REGISTER_TYPE(RasterTechnique, BufferProcessTechnique)
REGISTER_TYPE(RasterTechnique, LitVoxelTechnique)
REGISTER_TYPE(RasterTechnique, LightBounceVoxelIrradianceTechnique)
REGISTER_TYPE(RasterTechnique, DistanceShadowMappingTechnique)
REGISTER_TYPE(RasterTechnique, AntialiasingTechnique)
REGISTER_TYPE(RasterTechnique, ComputeFrustumCullingTechnique)
REGISTER_TYPE(RasterTechnique, SceneIndirectDrawTechnique)
REGISTER_TYPE(RasterTechnique, SceneRayTracingTechnique)
REGISTER_TYPE(RasterTechnique, VoxelVisibilityRayTracingTechnique)
REGISTER_TYPE(RasterTechnique, StaticSceneVoxelizationTechnique)
REGISTER_TYPE(RasterTechnique, DynamicSceneVoxelizationTechnique)
REGISTER_TYPE(RasterTechnique, DynamicVoxelCopyToBufferTechnique)
REGISTER_TYPE(RasterTechnique, DynamicVoxelVisibilityRayTracingTechnique)
REGISTER_TYPE(RasterTechnique, ResetLitvoxelData)
REGISTER_TYPE(RasterTechnique, CameraVisibleRayTracingTechnique)
REGISTER_TYPE(RasterTechnique, ProcessCameraVisibleResultsTechnique)
REGISTER_TYPE(RasterTechnique, DynamicLightBounceVoxelIrradianceTechnique)
REGISTER_TYPE(RasterTechnique, Clear3DTexturesTechnique)
REGISTER_TYPE(RasterTechnique, IrradianceGaussianBlurTechnique)
REGISTER_TYPE(RasterTechnique, StaticNeighbourInformationTechnique)
REGISTER_TYPE(RasterTechnique, SkeletalAnimationUpdateTechnique)
REGISTER_TYPE(RasterTechnique, SceneLightingDeferredTechnique)
REGISTER_TYPE(RasterTechnique, GBufferTechnique)
REGISTER_TYPE(RasterTechnique, RayTracingDeferredShadowsTechnique)
REGISTER_TYPE(RasterTechnique, LightBounceDynamicVoxelIrradianceTechnique)

/////////////////////////////////////////////////////////////////////////////////////////////
