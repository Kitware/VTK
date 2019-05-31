/*=========================================================================

 Program:   Visualization Toolkit
 Module:    ADIOS2Helper.txx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

/*
 * ADIOS2Helper.txx
 *
 *  Created on: May 3, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef VTK_IO_ADIOS2_ADIOS2HELPER_TCC_
#define VTK_IO_ADIOS2_ADIOS2HELPER_TCC_

#include "ADIOS2Helper.h"

#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkLongArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"

namespace adios2vtk
{
namespace helper
{

template<class T>
std::vector<T> StringToVector(const std::string& input) noexcept
{
  std::vector<T> output;
  std::istringstream inputSS(input);

  T record;
  while (inputSS >> record)
  {
    output.push_back(record);
  }
  return output;
}

// TODO: extend other types
template<>
vtkSmartPointer<vtkDataArray> NewDataArray<unsigned int>()
{
  return vtkSmartPointer<vtkUnsignedIntArray>::New();
}

template<>
vtkSmartPointer<vtkDataArray> NewDataArray<int>()
{
  return vtkSmartPointer<vtkIntArray>::New();
}

template<>
vtkSmartPointer<vtkDataArray> NewDataArray<long int>()
{
  return vtkSmartPointer<vtkLongArray>::New();
}

template<>
vtkSmartPointer<vtkDataArray> NewDataArray<unsigned long int>()
{
  return vtkSmartPointer<vtkUnsignedLongArray>::New();
}

template<>
vtkSmartPointer<vtkDataArray> NewDataArray<float>()
{
  return vtkSmartPointer<vtkFloatArray>::New();
}

template<>
vtkSmartPointer<vtkDataArray> NewDataArray<double>()
{
  return vtkSmartPointer<vtkDoubleArray>::New();
}

} // end namespace helper
} // end namespace adios2vtk

#endif
