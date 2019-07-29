/*=========================================================================

 Program:   Visualization Toolkit
 Module:    VTXDataArray.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

/*
 * VTXDataArray.cxx
 *
 *  Created on: Jun 4, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "VTXDataArray.h"

namespace vtx
{
namespace types
{

bool DataArray::IsScalar() const noexcept
{
  if (this->VectorVariables.empty())
  {
    return true;
  }
  return false;
}

} // end namespace types
} // end namespace vtx
