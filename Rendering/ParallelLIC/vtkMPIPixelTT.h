/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMPIPixelTT.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkMPIPixelTT_h
#define vtkMPIPixelTT_h

#include "vtkType.h" // for vtk types
#include "vtkMPI.h"

// Description:
// Traits class for converting from vtk data type enum
// to the appropriate C or MPI datatype.
template<typename T> class vtkMPIPixelTT;

#define vtkMPIPixelTTMacro1(_ctype) \
template<> \
class vtkMPIPixelTT<_ctype> \
{ \
public: \
  static MPI_Datatype MPIType; \
  static int VTKType; \
};

vtkMPIPixelTTMacro1(void)
vtkMPIPixelTTMacro1(char)
vtkMPIPixelTTMacro1(signed char)
vtkMPIPixelTTMacro1(unsigned char)
vtkMPIPixelTTMacro1(short)
vtkMPIPixelTTMacro1(unsigned short)
vtkMPIPixelTTMacro1(int)
vtkMPIPixelTTMacro1(unsigned int)
vtkMPIPixelTTMacro1(long)
vtkMPIPixelTTMacro1(unsigned long)
vtkMPIPixelTTMacro1(float)
vtkMPIPixelTTMacro1(double)
//vtkMPIPixelTTMacro1(vtkIdType)
vtkMPIPixelTTMacro1(long long)
vtkMPIPixelTTMacro1(unsigned long long)

#endif
// VTK-HeaderTest-Exclude: vtkMPIPixelTT.h
