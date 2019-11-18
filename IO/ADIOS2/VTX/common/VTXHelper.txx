/*=========================================================================

 Program:   Visualization Toolkit
 Module:    VTXHelper.txx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

/*
 * VTXHelper.txx
 *
 *  Created on: May 3, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef VTK_IO_ADIOS2_COMMON_VTXHelper_txx
#define VTK_IO_ADIOS2_COMMON_VTXHelper_txx

#include "VTXHelper.h"

#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkLongArray.h"
#include "vtkLongLongArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedLongLongArray.h"

namespace vtx
{
namespace helper
{

// TODO: extend other types
template <>
vtkSmartPointer<vtkDataArray> NewDataArray<int>()
{
  return vtkSmartPointer<vtkIntArray>::New();
}

template <>
vtkSmartPointer<vtkDataArray> NewDataArray<unsigned int>()
{
  return vtkSmartPointer<vtkUnsignedIntArray>::New();
}

template <>
vtkSmartPointer<vtkDataArray> NewDataArray<long int>()
{
  return vtkSmartPointer<vtkLongArray>::New();
}

template <>
vtkSmartPointer<vtkDataArray> NewDataArray<unsigned long int>()
{
  return vtkSmartPointer<vtkUnsignedLongArray>::New();
}

template <>
vtkSmartPointer<vtkDataArray> NewDataArray<long long int>()
{
  return vtkSmartPointer<vtkLongLongArray>::New();
}

template <>
vtkSmartPointer<vtkDataArray> NewDataArray<unsigned long long int>()
{
  return vtkSmartPointer<vtkUnsignedLongLongArray>::New();
}

template <>
vtkSmartPointer<vtkDataArray> NewDataArray<float>()
{
  return vtkSmartPointer<vtkFloatArray>::New();
}

template <>
vtkSmartPointer<vtkDataArray> NewDataArray<double>()
{
  return vtkSmartPointer<vtkDoubleArray>::New();
}

} // end namespace helper
} // end namespace vtx

#endif /* VTK_IO_ADIOS2_VTX_COMMON_VTXHelper_txx */
