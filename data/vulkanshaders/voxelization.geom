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

/////////////////////////////////////////////////////////////////////////////////////////////

layout (std140, binding = 2) uniform materialData
{
	mat4 projection;       //!< Orthographic projection matrix used for voxelization
	mat4 viewX;            //!< x axis view matrix used for voxelization
	mat4 viewY;            //!< y axis view matrix used for voxelization
	mat4 viewZ;            //!< z axis view matrix used for voxelization
	vec4 voxelizationSize; //!< 3D voxel texture size
} myMaterialData;

/////////////////////////////////////////////////////////////////////////////////////////////

layout (triangles) in;
layout (triangle_strip, max_vertices = 9) out;

/////////////////////////////////////////////////////////////////////////////////////////////

layout (location = 1) in vec2 vsUV[];

/////////////////////////////////////////////////////////////////////////////////////////////

layout (location = 1) out vec2 gsUV;
layout (location = 2) flat out uint faxis;

/////////////////////////////////////////////////////////////////////////////////////////////

void main(void)
{
	//Find the axis for the maximize the projected area of this triangle
	vec3 faceNormal = abs( normalize( cross( gl_in[1].gl_Position.xyz - gl_in[0].gl_Position.xyz, gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz) ) );
	float maxi      = max(faceNormal.x, max(faceNormal.y, faceNormal.z));

	mat4 view;

	if( maxi == faceNormal.x )
	{
		view  = myMaterialData.viewX;	
		faxis = 0;
	}
	else if( maxi == faceNormal.y )
	{
		view  = myMaterialData.viewY;
		faxis = 1;	
	}
	else
	{
		view  = myMaterialData.viewZ;	
		faxis = 2;
	}   

	vec4 projPosition[3];
	projPosition[0] = myMaterialData.projection * view * gl_in[0].gl_Position;
	projPosition[1] = myMaterialData.projection * view * gl_in[1].gl_Position;
	projPosition[2] = myMaterialData.projection * view * gl_in[2].gl_Position;

	for(uint i = 0; i < gl_in.length(); i++)
	{
		gsUV        = vsUV[i];
		gl_Position = projPosition[i];
		EmitVertex();
	}

	EndPrimitive();
}

/////////////////////////////////////////////////////////////////////////////////////////////
