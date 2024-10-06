/*
Copyright 2024 Alejandro Cosin

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

#extension GL_EXT_shader_8bit_storage : enable

/////////////////////////////////////////////////////////////////////////////////////////////

layout (std140, binding = 2) uniform materialData
{
    vec4 sceneMin;                    //!< Scene min
    vec4 sceneExtentVoxelizationSize; //!< Scene extent and voxel size in w field
} myMaterialData;

/////////////////////////////////////////////////////////////////////////////////////////////

layout(binding = 3) uniform sampler2D reflectance;
layout(binding = 4) uniform sampler2D normal;

/////////////////////////////////////////////////////////////////////////////////////////////

layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 positionVS;
layout(location = 2) in vec3 worldNormalVS;
layout(location = 3) in vec3 worldTangentVS;
layout(location = 5) flat in float isDynamicSceneElement;

/////////////////////////////////////////////////////////////////////////////////////////////

layout(location = 0) out vec4 normalGBuffer;
layout(location = 1) out vec4 reflectanceGBuffer;
layout(location = 2) out vec4 positionGBuffer;

/////////////////////////////////////////////////////////////////////////////////////////////

/** Sample a BC5 texture at the coordinates given as parameter for the texture index given as
* parameter and reconstruct the normal direction
* @param uv           [in] texture coordinates
* @param textureIndex [in] texture index
* @param mipLevel     [in] mip level to sample
* @return normal value */
vec3 sampleBC5Normal(vec2 uv)
{
    vec2 normalSampled  = texture(normal, uv).xy;
    vec3 normalUnpacked = vec3((normalSampled.xy * 2.0) - 1.0, 0.0);
    normalUnpacked.z    = sqrt(1.0 - dot(normalUnpacked.xy, normalUnpacked.xy));
    return normalize(normalUnpacked);
}

/////////////////////////////////////////////////////////////////////////////////////////////

/** Sample normal direction texture and transform it into world space
* @return nothing */
vec4 computeNormalDirection()
{
    vec3 norm3PS   = worldNormalVS;
    norm3PS        = normalize(norm3PS);
    vec3 tang3PS   = worldTangentVS;
    tang3PS        = normalize(tang3PS);
    vec3 binormal3 = cross(norm3PS,tang3PS);
    binormal3      = normalize(binormal3);
    mat3 rotMat    = mat3(tang3PS, binormal3, norm3PS); //Construct the texture-space matrix and rotate the normal into the world space
    vec3 vnormal   = sampleBC5Normal(uv);
    vec3 snormal   = normalize(rotMat * vnormal);
    snormal       *= 0.5;
    snormal       += vec3(0.5);
    return vec4(snormal, 0.0);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void main()
{
	vec4 reflectance = texture(reflectance, uv);

//#ifdef MATERIAL_TYPE_ALPHATESTED
    if(reflectance.w < 0.3)
    {
        discard;
    }
//#endif

	reflectanceGBuffer = vec4(reflectance.rgb, 0.0);
    normalGBuffer      = computeNormalDirection();
    positionGBuffer    = vec4(positionVS.xyz, (isDynamicSceneElement == 1.0) ? 1.0 : -1.0);
}

/////////////////////////////////////////////////////////////////////////////////////////////
