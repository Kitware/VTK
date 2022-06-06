/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkOpenXRRenderer.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

Parts Copyright Valve Corporation from hellovr_opengl_main.cpp
under their BSD license found here:
https://github.com/ValveSoftware/openvr/blob/master/LICENSE

=========================================================================*/
#include "vtkOpenXRRenderer.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkOpenXRCamera.h"

vtkStandardNewMacro(vtkOpenXRRenderer);

//------------------------------------------------------------------------------
vtkOpenXRRenderer::vtkOpenXRRenderer()
{
  // better default
  this->ClippingRangeExpansion = 0.05;
}

//------------------------------------------------------------------------------
vtkCamera* vtkOpenXRRenderer::MakeCamera()
{
  vtkCamera* cam = vtkOpenXRCamera::New();
  this->InvokeEvent(vtkCommand::CreateCameraEvent, cam);
  return cam;
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
