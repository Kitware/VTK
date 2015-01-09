/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAbstractRenderDevice.h"
#include "vtkObjectFactory.h"

vtkAbstractObjectFactoryNewMacro(vtkAbstractRenderDevice)

vtkAbstractRenderDevice::vtkAbstractRenderDevice() : GLMajor(2), GLMinor(1)
{
}

vtkAbstractRenderDevice::~vtkAbstractRenderDevice()
{
}

void vtkAbstractRenderDevice::SetRequestedGLVersion(int major, int minor)
{
  this->GLMajor = major;
  this->GLMinor = minor;
}

void vtkAbstractRenderDevice::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
