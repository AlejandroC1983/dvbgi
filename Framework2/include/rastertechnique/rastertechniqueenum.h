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

#ifndef _RASTER_TECHNIQUE_ENUM_H_
#define _RASTER_TECHNIQUE_ENUM_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES

// CLASS FORWARDING

// NAMESPACE
namespace rastertechniqueenum
{
	/////////////////////////////////////////////////////////////////////////////////////////////

	/** Policies for recording commands for each raster technique */
	enum class CommandRecordPolicy
	{
		CRP_SINGLE_TIME = 0,       //!< To record commands only one single time for the rester technique with this policy
		CRP_EVERY_SWAPCHAIN_IMAGE, //!< To record commands for every swap chainimage for the rester technique with this policy
		CRP_SIZE                   //!< Number of possible values
	};

	/////////////////////////////////////////////////////////////////////////////////////////////

	/** Simple enumerated type to describe the type of queue the recorded command buffer should be submitted to */
	enum class CommandBufferType
	{
		CBT_GRAPHICS_QUEUE = 0, //!< To indicate that the recorded command buffer should be submitted to the graphics queue
		CBT_COMPUTE_QUEUE,      //!< To indicate that the recorded command buffer should be submitted to the compute queue
		CBT_SIZE                //!< Number of possible values
	};

	/////////////////////////////////////////////////////////////////////////////////////////////

	/** Simple enumerated type to describe the type of queue a raster technique submits command buffers to */
	enum class RasterTechniqueType
	{
		RTT_GRAPHICS = 0, //!< Submit to the graphics queue
		RTT_COMPUTE,      //!< Submit to the compute queue
		RTT_SIZE          //!< Number of possible values
	};

	/////////////////////////////////////////////////////////////////////////////////////////////
}

// DEFINES
#define NUMBER_OF_VIEWPORTS 1
#define NUMBER_OF_SCISSORS NUMBER_OF_VIEWPORTS
typedef map<uint, rastertechniqueenum::CommandBufferType> mapUintCommandBufferType;

#endif _RASTER_TECHNIQUE_ENUM_H_
