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
#include "vtkRenderWindow.h"

#include "vtkOpenGL.h"
#include <math.h>

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
    vtkHardwareSelector* selector = ren->GetSelector();
    bool picking = (ren->GetRenderWindow()->GetIsPicking() || selector != NULL);
    if (picking)
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
void vtkOpenGLActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
