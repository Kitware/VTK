/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkOpenXRRenderer.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

Parts Copyright Valve Coproration from hellovr_opengl_main.cpp
under their BSD license found here:
https://github.com/ValveSoftware/openvr/blob/master/LICENSE

=========================================================================*/
#include "vtkOpenXRRenderer.h"
#include "vtkOpenXRCamera.h"

#include "vtkObjectFactory.h"

#include "vtkActor.h"
#include "vtkImageCanvasSource2D.h"
#include "vtkInformation.h"
#include "vtkNew.h"
#include "vtkPlaneSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkTexture.h"
#include "vtkTransform.h"

#include "vtkOpenXRRenderWindow.h"

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
