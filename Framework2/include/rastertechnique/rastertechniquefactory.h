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

#ifndef _RASTERTECHNIQUEFACTORY_H_
#define _RASTERTECHNIQUEFACTORY_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/util/factorytemplate.h"
#include "../../include/util/managertemplate.h"

// CLASS FORWARDING
class RasterTechnique;

// NAMESPACE

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

class RasterTechniqueFactory: public FactoryTemplate<RasterTechnique>
{

};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _RASTERTECHNIQUEFACTORY_H_
