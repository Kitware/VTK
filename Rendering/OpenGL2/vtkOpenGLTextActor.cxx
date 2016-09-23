/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLTextActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLTextActor.h"

#include "vtkCoordinate.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLGL2PSHelper.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkViewport.h"

vtkStandardNewMacro(vtkOpenGLTextActor)

//------------------------------------------------------------------------------
void vtkOpenGLTextActor::PrintSelf(std::ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
int vtkOpenGLTextActor::RenderOverlay(vtkViewport *viewport)
{
  // Render to GL2PS if capturing:
  vtkOpenGLGL2PSHelper *gl2ps = vtkOpenGLGL2PSHelper::GetInstance();
  if (gl2ps)
  {
    switch (gl2ps->GetActiveState())
    {
      case vtkOpenGLGL2PSHelper::Capture:
        return this->RenderGL2PS(viewport, gl2ps);
      case vtkOpenGLGL2PSHelper::Background:
        return 0; // No rendering.
      case vtkOpenGLGL2PSHelper::Inactive:
        break; // continue rendering.
    }
  }

  return this->Superclass::RenderOverlay(viewport);
}

//------------------------------------------------------------------------------
vtkOpenGLTextActor::vtkOpenGLTextActor()
{
}

//------------------------------------------------------------------------------
vtkOpenGLTextActor::~vtkOpenGLTextActor()
{
}

//------------------------------------------------------------------------------
int vtkOpenGLTextActor::RenderGL2PS(vtkViewport *viewport,
                                    vtkOpenGLGL2PSHelper *gl2ps)
{
  std::string input = (this->Input && this->Input[0]) ? this->Input : "";
  if (input.empty())
  {
    return 0;
  }

  vtkRenderer *ren = vtkRenderer::SafeDownCast(viewport);
  if (!ren)
  {
    vtkWarningMacro("Viewport is not a renderer.");
    return 0;
  }

  // Figure out position:
  vtkCoordinate *coord = this->GetActualPositionCoordinate();
  double *textPos2 = coord->GetComputedDoubleDisplayValue(ren);
  double pos[3];
  pos[0] = textPos2[0];
  pos[1] = textPos2[1];
  pos[2] = -1.;

  vtkTextProperty *tprop = this->GetScaledTextProperty();
  gl2ps->DrawString(input, tprop, pos, pos[2] + 1e-6, ren);

  return 1;
}
