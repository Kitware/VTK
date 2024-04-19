// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAbstractRenderDevice.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkAbstractObjectFactoryNewMacro(vtkAbstractRenderDevice);

vtkAbstractRenderDevice::vtkAbstractRenderDevice()
  : GLMajor(2)
  , GLMinor(1)
{
}

vtkAbstractRenderDevice::~vtkAbstractRenderDevice() = default;

void vtkAbstractRenderDevice::SetRequestedGLVersion(int major, int minor)
{
  this->GLMajor = major;
  this->GLMinor = minor;
}

void vtkAbstractRenderDevice::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
