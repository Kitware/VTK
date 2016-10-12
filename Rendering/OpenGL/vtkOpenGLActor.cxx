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
#include "vtkOpenGLActor.h"

#include "vtkMapper.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderer.h"
#include "vtkProperty.h"
#include "vtkOpenGLError.h"

#include "vtkOpenGL.h"
#include <cmath>

vtkStandardNewMacro(vtkOpenGLActor);

// Actual actor render method.
void vtkOpenGLActor::Render(vtkRenderer *ren, vtkMapper *mapper)
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

  // build transformation
  if (!this->IsIdentity)
  {
    // compute the transposed matrix
    double mat[16];
    vtkMatrix4x4::Transpose(*this->GetMatrix()->Element, mat);

    // insert model transformation
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glMultMatrixd(mat);
  }

  // send a render to the mapper; update pipeline
  mapper->Render(ren, this);

  // pop transformation matrix
  if (!this->IsIdentity)
  {
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
  }

  if (opacity != 1.0)
  {
    glDepthMask(GL_TRUE);
  }

  vtkOpenGLCheckErrorMacro("failed after Render");
}

//----------------------------------------------------------------------------
void vtkOpenGLActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
