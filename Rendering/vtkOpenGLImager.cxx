/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLImager.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLImager.h"
#include "vtkImageWindow.h"
#ifndef VTK_IMPLEMENT_MESA_CXX
  #ifdef __APPLE__
    #include <OpenGL/gl.h>
  #else
    #include <GL/gl.h>
  #endif
#endif
#include "vtkObjectFactory.h"


#ifndef VTK_IMPLEMENT_MESA_CXX
vtkCxxRevisionMacro(vtkOpenGLImager, "1.17");
vtkStandardNewMacro(vtkOpenGLImager);
#endif



int vtkOpenGLImager::RenderOpaqueGeometry()
{
  int lowerLeft[2];

  float *vport = this->GetViewport();

  float vpu, vpv;
  vpu = vport[0];
  vpv = vport[1];  
  this->NormalizedDisplayToDisplay(vpu,vpv);
  lowerLeft[0] = (int)(vpu+0.5);
  lowerLeft[1] = (int)(vpv+0.5);
  float vpu2, vpv2;
  vpu2 = vport[2];
  vpv2 = vport[3];  
  this->NormalizedDisplayToDisplay(vpu2,vpv2);
  int usize = (int)(vpu2 + 0.5) - lowerLeft[0];
  int vsize = (int)(vpv2 + 0.5) - lowerLeft[1];  

  // we will set this for all modes on the sparc
  glViewport(lowerLeft[0],lowerLeft[1],usize, vsize);
  glEnable( GL_SCISSOR_TEST );
  glScissor(lowerLeft[0],lowerLeft[1],usize,vsize);
  return vtkImager::RenderOpaqueGeometry();
}

void vtkOpenGLImager::Erase()
{
  int lowerLeft[2];

  float *vport = this->GetViewport();

  float vpu, vpv;
  vpu = vport[0];
  vpv = vport[1];  
  this->NormalizedDisplayToDisplay(vpu,vpv);
  lowerLeft[0] = (int)(vpu+0.5);
  lowerLeft[1] = (int)(vpv+0.5);
  float vpu2, vpv2;
  vpu2 = vport[2];
  vpv2 = vport[3];  
  this->NormalizedDisplayToDisplay(vpu2,vpv2);
  int usize = (int)(vpu2 + 0.5) - lowerLeft[0];
  int vsize = (int)(vpv2 + 0.5) - lowerLeft[1];  

  // we will set this for all modes on the sparc
  glViewport(lowerLeft[0],lowerLeft[1],usize, vsize);
  glEnable( GL_SCISSOR_TEST );
  glScissor(lowerLeft[0],lowerLeft[1],usize,vsize);

  glClearDepth( (GLclampd)(1.0));
  glClearColor( ((GLclampf)(this->Background[0])),
                ((GLclampf)(this->Background[1])),
                ((GLclampf)(this->Background[2])),
                ((GLclampf)(1.0)) );

  vtkDebugMacro(<< "glClear\n");
  glClear((GLbitfield)GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

