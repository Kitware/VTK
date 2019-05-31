/*=========================================================================

 Program:   Visualization Toolkit
 Module:    ADIOS2xmlVTI.txx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

/*
 * ADIOS2xmlVTI.txx
 *
 *  Created on: June 3, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef VTK_IO_ADIOS2_SCHEMA_XML_VTK_ADIOS2XMLVTI_TXX_
#define VTK_IO_ADIOS2_SCHEMA_XML_VTK_ADIOS2XMLVTI_TXX_

#include "ADIOS2xmlVTI.h"

#include <iostream>

namespace adios2vtk
{
namespace schema
{
template<class T>
void ADIOS2xmlVTI::SetDimensionsCommon(
  adios2::Variable<T> variable, const types::DataArray& dataArray, const size_t step)
{
  const adios2::Dims shape = variable.Shape(step);
  if (shape.size() == 3) // 3D to 3D
  {
    variable.SetSelection({ dataArray.m_Start, dataArray.m_Count });
  }
  else if (shape.size() == 2) // 2D into 3D
  {
    // TODO
  }
  else if (shape.size() == 1) // linearized 1D into 3D mapped
  {
    // TODO
  }
}

} // end namespace schema
} // end namespace adios2vtk

#endif /* VTK_IO_ADIOS2_SCHEMA_XML_VTK_ADIOS2XMLVTI_TXX_ */
