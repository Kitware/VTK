// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkSkybox.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkObjectFactoryNewMacro(vtkSkybox);

//------------------------------------------------------------------------------
void vtkSkybox::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
// Always return uninitialized
double* vtkSkybox::GetBounds()
{
  return nullptr;
}

//------------------------------------------------------------------------------
vtkSkybox::vtkSkybox()
{
  this->Projection = vtkSkybox::Cube;
  this->FloorPlane[0] = 0.0;
  this->FloorPlane[1] = 1.0;
  this->FloorPlane[2] = 0.0;
  this->FloorPlane[3] = 0.0;
  this->FloorRight[0] = 1.0;
  this->FloorRight[1] = 0.0;
  this->FloorRight[2] = 0.0;
  this->FloorTexCoordScale[0] = 1.0;
  this->FloorTexCoordScale[1] = 1.0;
}

//------------------------------------------------------------------------------
vtkSkybox::~vtkSkybox() = default;
VTK_ABI_NAMESPACE_END
