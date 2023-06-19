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

#define VTK_GDA_VALUERANGE_INSTANTIATING
#include "vtkGenericDataArray.h"

#include "vtkDataArrayPrivate.txx"
#include "vtkOStreamWrapper.h"

namespace vtkDataArrayPrivate
{
VTK_ABI_NAMESPACE_BEGIN
VTK_INSTANTIATE_VALUERANGE_ARRAYTYPE(vtkDataArray, double)
VTK_ABI_NAMESPACE_END
} // namespace vtkDataArrayPrivate
