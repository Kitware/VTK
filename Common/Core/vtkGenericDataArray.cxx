/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericDataArray.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// We can't include vtkDataArrayPrivate.txx from a header, since it pulls in
// windows.h and creates a bunch of name collisions. So we compile the range
// lookup functions into this translation unit where we can encapsulate the
// header.
// We only compile the 64-bit integer versions of these. All others just
// reuse the double-precision vtkDataArray::GetRange version, since they
// won't lose precision.

#define VTK_GDA_VALUERANGE_INSTANTIATING
#include "vtkGenericDataArray.h"

#include "vtkDataArrayPrivate.txx"

#include "vtkAOSDataArrayTemplate.h"
#include "vtkSOADataArrayTemplate.h"

#ifdef VTK_USE_SCALED_SOA_ARRAYS
#include "vtkScaledSOADataArrayTemplate.h"
#endif

namespace vtkDataArrayPrivate
{
VTK_INSTANTIATE_VALUERANGE_VALUETYPE(long)
VTK_INSTANTIATE_VALUERANGE_VALUETYPE(unsigned long)
VTK_INSTANTIATE_VALUERANGE_VALUETYPE(long long)
VTK_INSTANTIATE_VALUERANGE_VALUETYPE(unsigned long long)
VTK_INSTANTIATE_VALUERANGE_ARRAYTYPE(vtkDataArray, double)
} // namespace vtkDataArrayPrivate
