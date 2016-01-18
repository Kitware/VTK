/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSOADataArrayTemplateInstantiate.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// This file generates instantiations of vtkSOADataArrayTemplate for the
// common data types. For AoS arrays, this is done in the more derived classes
// (e.g. vtkFloatArray.cxx.o contains the instantiation of
// vtkAOSDataArrayTemplate<float>), but since these aren't derived from in VTK
// (yet), we instantiate them here.

#define VTK_SOA_DATA_ARRAY_TEMPLATE_INSTANTIATING
#include "vtkSOADataArrayTemplate.txx"

VTK_SOA_DATA_ARRAY_TEMPLATE_INSTANTIATE(char);
VTK_SOA_DATA_ARRAY_TEMPLATE_INSTANTIATE(double);
VTK_SOA_DATA_ARRAY_TEMPLATE_INSTANTIATE(float);
VTK_SOA_DATA_ARRAY_TEMPLATE_INSTANTIATE(int);
VTK_SOA_DATA_ARRAY_TEMPLATE_INSTANTIATE(long);
VTK_SOA_DATA_ARRAY_TEMPLATE_INSTANTIATE(long long);
VTK_SOA_DATA_ARRAY_TEMPLATE_INSTANTIATE(short);
VTK_SOA_DATA_ARRAY_TEMPLATE_INSTANTIATE(signed char);
VTK_SOA_DATA_ARRAY_TEMPLATE_INSTANTIATE(unsigned char);
VTK_SOA_DATA_ARRAY_TEMPLATE_INSTANTIATE(unsigned int);
VTK_SOA_DATA_ARRAY_TEMPLATE_INSTANTIATE(unsigned long);
VTK_SOA_DATA_ARRAY_TEMPLATE_INSTANTIATE(unsigned long long);
VTK_SOA_DATA_ARRAY_TEMPLATE_INSTANTIATE(unsigned short);
