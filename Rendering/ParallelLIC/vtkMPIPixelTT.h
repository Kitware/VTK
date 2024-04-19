// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkMPIPixelTT_h
#define vtkMPIPixelTT_h

#include "vtkMPI.h"
#include "vtkType.h" // for vtk types

// Description:
// Traits class for converting from vtk data type enum
// to the appropriate C or MPI datatype.
VTK_ABI_NAMESPACE_BEGIN
template <typename T>
class vtkMPIPixelTT;

#define vtkMPIPixelTTMacro1(_ctype)                                                                \
  template <>                                                                                      \
  class vtkMPIPixelTT<_ctype>                                                                      \
  {                                                                                                \
  public:                                                                                          \
    static MPI_Datatype MPIType;                                                                   \
    static int VTKType;                                                                            \
  }

vtkMPIPixelTTMacro1(void);
vtkMPIPixelTTMacro1(char);
vtkMPIPixelTTMacro1(signed char);
vtkMPIPixelTTMacro1(unsigned char);
vtkMPIPixelTTMacro1(short);
vtkMPIPixelTTMacro1(unsigned short);
vtkMPIPixelTTMacro1(int);
vtkMPIPixelTTMacro1(unsigned int);
vtkMPIPixelTTMacro1(long);
vtkMPIPixelTTMacro1(unsigned long);
vtkMPIPixelTTMacro1(float);
vtkMPIPixelTTMacro1(double);
// vtkMPIPixelTTMacro1(vtkIdType);
vtkMPIPixelTTMacro1(long long);
vtkMPIPixelTTMacro1(unsigned long long);

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkMPIPixelTT.h
