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

#ifndef _COMMON_NAMESPACE_H_
#define _COMMON_NAMESPACE_H_

// GLOBAL INCLUDES
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <unordered_map>
#include <memory>
#include <functional>
#include <utility>
#include <algorithm>
#include <fstream>
#include <ctime>
#include <random>
#include <iomanip>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/constants.hpp>
#include "gli.hpp"
#include <glm/gtx/intersect.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/detail/type_half.hpp>
#include <glm/ext.hpp>

// PROJECT INCLUDES

// CLASS FORWARDING

// NAMESPACE

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

namespace commonnamespace
{
	// std namespace
	using std::cout;
	using std::endl;
	using std::vector;
	using std::map;
	using std::string;
	using std::unordered_map;
	using std::unique_ptr;
	using std::hash;
	using std::move;
	using std::pair;
	using std::make_pair;
	using std::make_unique;
	using std::to_string;
	using std::ofstream;
	using std::setprecision;
	using std::max;
	using std::find;
	using std::ifstream;
	using std::remove;
	using std::ios;

	// glm namespace
	using glm::vec3;
	using glm::vec2;
	using glm::vec4;
	using glm::dvec3;
	using glm::dvec2;
	using glm::dvec4;
	using glm::ivec3;
	using glm::ivec2;
	using glm::ivec4;
	using glm::uvec3;
	using glm::uvec2;
	using glm::uvec4;
	using glm::bvec3;
	using glm::bvec2;
	using glm::bvec4;
	using glm::mat2;
	using glm::mat3;
	using glm::mat4;
	using glm::mat2x3;
	using glm::mat2x4;
	using glm::mat3x2;
	using glm::mat3x4;
	using glm::mat4x2;
	using glm::mat4x3;
	using glm::dmat2;
	using glm::dmat3;
	using glm::dmat4;
	using glm::dmat2x3;
	using glm::dmat2x4;
	using glm::dmat3x2;
	using glm::dmat3x4;
	using glm::dmat4x2;
	using glm::dmat4x3;
	using glm::pi;
	using glm::quat;
	using glm::normalize;
	using glm::inverse;
	using glm::cross;
	using glm::dot;
	using glm::inverseTranspose;
	using glm::l2Norm;
	using glm::radians;
	using glm::make_mat4;
	using glm::clamp;
}

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _COMMON_NAMESPACE_H_
