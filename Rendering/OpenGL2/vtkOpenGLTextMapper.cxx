/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLTextMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLTextMapper.h"

#include "vtkActor2D.h"
#include "vtkCoordinate.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLGL2PSHelper.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkViewport.h"

vtkStandardNewMacro(vtkOpenGLTextMapper)

//------------------------------------------------------------------------------
void vtkOpenGLTextMapper::PrintSelf(std::ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkOpenGLTextMapper::RenderOverlay(vtkViewport *vp, vtkActor2D *act)
{
  // Render to GL2PS if capturing:
  vtkOpenGLGL2PSHelper *gl2ps = vtkOpenGLGL2PSHelper::GetInstance();
  if (gl2ps)
  {
    switch (gl2ps->GetActiveState())
    {
      case vtkOpenGLGL2PSHelper::Capture:
        this->RenderGL2PS(vp, act, gl2ps);
        return;
      case vtkOpenGLGL2PSHelper::Background:
        return; // No rendering.
      case vtkOpenGLGL2PSHelper::Inactive:
        break; // continue rendering.
    }
  }

  this->Superclass::RenderOverlay(vp, act);
}

//------------------------------------------------------------------------------
vtkOpenGLTextMapper::vtkOpenGLTextMapper()
{
}

//------------------------------------------------------------------------------
vtkOpenGLTextMapper::~vtkOpenGLTextMapper()
{
}

//------------------------------------------------------------------------------
void vtkOpenGLTextMapper::RenderGL2PS(vtkViewport *vp, vtkActor2D *act,
                                      vtkOpenGLGL2PSHelper *gl2ps)
{
  std::string input = (this->Input && this->Input[0]) ? this->Input : "";
  if (input.empty())
  {
    return;
  }

  vtkRenderer *ren = vtkRenderer::SafeDownCast(vp);
  if (!ren)
  {
    vtkWarningMacro("Viewport is not a renderer.");
    return;
  }

  // Figure out position:
  vtkCoordinate *coord = act->GetActualPositionCoordinate();
  double *textPos2 = coord->GetComputedDoubleDisplayValue(ren);
  double pos[3];
  pos[0] = textPos2[0];
  pos[1] = textPos2[1];
  pos[2] = -1.;

  gl2ps->DrawString(input, this->TextProperty, pos, pos[2] + 1e-6, ren);
}
