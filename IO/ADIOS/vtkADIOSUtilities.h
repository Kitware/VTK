/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkADIOSUtilities.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkADIOSUtilities_h
#define vtkADIOSUtilities_h

#include <vtkType.h>

#include "ADIOSUtilities.h"

namespace ADIOS
{
namespace Type
{

// Specialization for vtkIdType
template<> ADIOS_DATATYPES NativeToADIOS<vtkIdType>();

// Description:
// Map VTK datatypes into ADIOS data types
ADIOS_DATATYPES VTKToADIOS(int tv);

// Description:
// Map VTK datatypes into ADIOS data types
int ADIOSToVTK(ADIOS_DATATYPES ta);

} // End namespace Type
} // End namespace ADIOS

#endif
// VTK-HeaderTest-Exclude: vtkADIOSUtilities.h
