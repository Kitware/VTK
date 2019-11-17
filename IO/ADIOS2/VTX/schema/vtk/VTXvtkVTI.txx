/*=========================================================================

 Program:   Visualization Toolkit
 Module:    VTXvtkVTI.txx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

/*
 * VTXvtkVTI.txx
 *
 *  Created on: June 3, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef VTK_IO_ADIOS2_VTX_SCHEMA_VTK_VTXvtkVTI_txx
#define VTK_IO_ADIOS2_VTX_SCHEMA_VTK_VTXvtkVTI_txx

#include "VTXvtkVTI.h"

#include "VTX/common/VTXHelper.h"

namespace vtx
{
namespace schema
{

template <class T>
void VTXvtkVTI::SetDimensionsCommon(
  adios2::Variable<T> variable, const types::DataArray& dataArray, const size_t step)
{
  const adios2::Dims shape = variable.Shape(step);
  if (shape.size() == 3) // 3D to 3D
  {
    variable.SetSelection({ dataArray.Start, dataArray.Count });
  }
  else if (shape.size() == 1)
  {
    const size_t start1D = helper::LinearizePoint(dataArray.Shape, dataArray.Start);
    const size_t count1D = helper::TotalElements(dataArray.Count);
    variable.SetSelection({ { start1D }, { count1D } });
  }
}

} // end namespace schema
} // end namespace vtx

#endif /* VTK_IO_ADIOS2_VTX_SCHEMA_VTK_VTXvtkVTI_txx */
