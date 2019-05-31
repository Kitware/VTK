/*=========================================================================

 Program:   Visualization Toolkit
 Module:    ADIOS2DataArray.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

/*
 * ADIOS2DataArray.h : wrapper around vtkDataArray adding adios2 relevant information
 *
 *  Created on: Jun 4, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef VTK_IO_ADIOS2_ADIOS2DATAARRAY_H_
#define VTK_IO_ADIOS2_ADIOS2DATAARRAY_H_

#include "vtkDataArray.h"
#include "vtkSmartPointer.h"

#include <adios2.h>

namespace adios2vtk
{
namespace types
{

class DataArray
{
public:
  std::vector<std::string> m_VectorVariables;
  vtkSmartPointer<vtkDataArray> m_vtkDataArray;
  adios2::Dims m_Shape;
  adios2::Dims m_Start;
  adios2::Dims m_Count;

  DataArray() = default;
  ~DataArray() = default;

  bool IsScalar() const noexcept;
};

} // end namespace types
} // end namespace adios2vtk

#endif /* VTK_IO_ADIOS2_ADIOS2DATAARRAY_H_ */
