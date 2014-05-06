/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGL2Actor.h"

#include "vtkMapper.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderer.h"
#include "vtkProperty.h"
#include "vtkOpenGLError.h"

#include "vtkOpenGL.h"
#include <math.h>

vtkStandardNewMacro(vtkOpenGL2Actor);

// Actual actor render method.
void vtkOpenGL2Actor::Render(vtkRenderer *ren, vtkMapper *mapper)
{
  vtkOpenGLClearErrorMacro();

  // get opacity
  double opacity = this->GetProperty()->GetOpacity();
  if (opacity == 1.0)
    {
    glDepthMask(GL_TRUE);
    }
  else
    {
    // Add this check here for GL_SELECT mode
    // If we are not picking, then don't write to the zbuffer
    // because we probably haven't sorted the polygons. If we
    // are picking, then translucency doesn't matter - we want to
    // pick the thing closest to us.
    GLint param;
    glGetIntegerv(GL_RENDER_MODE, &param);
    if (param == GL_SELECT )
      {
      glDepthMask(GL_TRUE);
      }
    else
      {
      if (ren->GetLastRenderingUsedDepthPeeling())
        {
        glDepthMask(GL_TRUE); // transparency with depth peeling
        }
      else
        {
        glDepthMask(GL_FALSE); // transparency with alpha blending
        }
      }
    }

  // send a render to the mapper; update pipeline
  mapper->Render(ren, this);

  if (opacity != 1.0)
    {
    glDepthMask(GL_TRUE);
    }

  vtkOpenGLCheckErrorMacro("failed after Render");
}

//----------------------------------------------------------------------------
void vtkOpenGL2Actor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
