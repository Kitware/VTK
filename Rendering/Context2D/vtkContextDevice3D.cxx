/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContextDevice3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkContextDevice3D.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkUnsignedCharArray.h"

VTK_ABI_NAMESPACE_BEGIN
vtkAbstractObjectFactoryNewMacro(vtkContextDevice3D);

vtkContextDevice3D::vtkContextDevice3D() = default;

vtkContextDevice3D::~vtkContextDevice3D() = default;

//------------------------------------------------------------------------------
void vtkContextDevice3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkContextDevice3D::DrawPoints(
  vtkDataArray* positions, vtkUnsignedCharArray* colors, std::uintptr_t)
{
  float* f = vtkArrayDownCast<vtkFloatArray>(positions)->GetPointer(0);
  const int nv = positions->GetNumberOfTuples();
  auto c = (colors && colors->GetNumberOfTuples() > 0) ? colors->GetPointer(0) : nullptr;
  const int nc_comps = colors ? colors->GetNumberOfComponents() : 0;
  this->DrawPoints(f, nv, c, nc_comps);
}

//------------------------------------------------------------------------------
void vtkContextDevice3D::DrawTriangleMesh(
  vtkDataArray* positions, vtkUnsignedCharArray* colors, std::uintptr_t)
{
  float* f = vtkArrayDownCast<vtkFloatArray>(positions)->GetPointer(0);
  const int nv = positions->GetNumberOfTuples();
  auto c = (colors && colors->GetNumberOfTuples() > 0) ? colors->GetPointer(0) : nullptr;
  const int nc_comps = colors ? colors->GetNumberOfComponents() : 0;
  this->DrawTriangleMesh(f, nv, c, nc_comps);
}
VTK_ABI_NAMESPACE_END
