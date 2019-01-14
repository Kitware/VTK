/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMPIPixelTT.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMPIPixelTT.h"

#define vtkMPIPixelTTMacro2(_ctype, _mpiEnum, _vtkEnum) \
MPI_Datatype vtkMPIPixelTT<_ctype>::MPIType = _mpiEnum; \
int vtkMPIPixelTT<_ctype>::VTKType = _vtkEnum;

vtkMPIPixelTTMacro2(void, MPI_BYTE, VTK_VOID)
vtkMPIPixelTTMacro2(char, MPI_CHAR, VTK_CHAR)
vtkMPIPixelTTMacro2(signed char, MPI_CHAR, VTK_SIGNED_CHAR)
vtkMPIPixelTTMacro2(unsigned char, MPI_UNSIGNED_CHAR, VTK_UNSIGNED_CHAR)
vtkMPIPixelTTMacro2(short, MPI_SHORT, VTK_SHORT)
vtkMPIPixelTTMacro2(unsigned short, MPI_UNSIGNED_SHORT, VTK_UNSIGNED_SHORT)
vtkMPIPixelTTMacro2(int, MPI_INT, VTK_INT)
vtkMPIPixelTTMacro2(unsigned int, MPI_UNSIGNED, VTK_UNSIGNED_INT)
vtkMPIPixelTTMacro2(long, MPI_LONG, VTK_LONG)
vtkMPIPixelTTMacro2(unsigned long, MPI_UNSIGNED_LONG, VTK_UNSIGNED_LONG)
vtkMPIPixelTTMacro2(float, MPI_FLOAT, VTK_FLOAT)
vtkMPIPixelTTMacro2(double, MPI_DOUBLE, VTK_DOUBLE)
//vtkMPIPixelTTMacro2(vtkIdType, MPI_LONG_LONG, VTK_IDTYPE)
vtkMPIPixelTTMacro2(long long, MPI_LONG_LONG, VTK_LONG_LONG)
vtkMPIPixelTTMacro2(unsigned long long, MPI_UNSIGNED_LONG_LONG, VTK_UNSIGNED_LONG_LONG)
