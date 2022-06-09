/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkOpenVRRenderer.cxx

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
#include "vtkOpenVRRenderer.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkOpenVRCamera.h"

vtkStandardNewMacro(vtkOpenVRRenderer);

//------------------------------------------------------------------------------
vtkCamera* vtkOpenVRRenderer::MakeCamera()
{
  vtkCamera* cam = vtkOpenVRCamera::New();
  this->InvokeEvent(vtkCommand::CreateCameraEvent, cam);
  return cam;
}
