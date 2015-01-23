/*==============================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLLabeledContourMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

==============================================================================*/
#include "vtkOpenGLLabeledContourMapper.h"

#include "vtkObjectFactory.h"
#include "vtkOpenGL.h"
#include "vtkOpenGLError.h"

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLLabeledContourMapper)

//------------------------------------------------------------------------------
vtkOpenGLLabeledContourMapper::vtkOpenGLLabeledContourMapper()
{
}

//------------------------------------------------------------------------------
vtkOpenGLLabeledContourMapper::~vtkOpenGLLabeledContourMapper()
{
}

//------------------------------------------------------------------------------
bool vtkOpenGLLabeledContourMapper::ApplyStencil(vtkRenderer *ren,
                                                 vtkActor *act)
{
  // Save some state:
  GLboolean colorMask[4];
  glGetBooleanv(GL_COLOR_WRITEMASK, colorMask);
  GLboolean depthMask;
  glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);

  // Enable rendering into the stencil buffer:
  glEnable(GL_STENCIL_TEST);
  glStencilMask(0xFF);
  glClearStencil(0);
  glClear(GL_STENCIL_BUFFER_BIT);
  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
  glDepthMask(GL_FALSE);
  glStencilFunc(GL_ALWAYS, 1, 0xFF);
  glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);

  // Draw stencil quads into stencil buffer:
  glColor4ub(255, 255, 255, 255);
  glEnableClientState(GL_VERTEX_ARRAY);
  glVertexPointer(3, GL_FLOAT, 0, this->StencilQuads);
  glDrawElements(GL_TRIANGLES, this->StencilQuadIndicesSize, GL_UNSIGNED_INT,
                 this->StencilQuadIndices);

  // Restore state:
  glColorMask(colorMask[0], colorMask[1], colorMask[2], colorMask[3]);
  glDepthMask(depthMask);

  // Setup GL to only draw in unstenciled regions:
  glStencilMask(0x00);
  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
  glStencilFunc(GL_EQUAL, 0, 0xFF);

  vtkOpenGLCheckErrorMacro("failed after ApplyStencil()");

  return this->Superclass::ApplyStencil(ren, act);
}

//------------------------------------------------------------------------------
bool vtkOpenGLLabeledContourMapper::RemoveStencil()
{
  glDisable(GL_STENCIL_TEST);
  vtkOpenGLCheckErrorMacro("failed after RemoveStencil()");
  return this->Superclass::RemoveStencil();
}
