/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLRenderUtilities.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtk_glew.h"
#include "vtkOpenGLRenderUtilities.h"

#include "vtkNew.h"
#include "vtkOpenGLBufferObject.h"
#include "vtkOpenGLVertexArrayObject.h"

// ----------------------------------------------------------------------------
vtkOpenGLRenderUtilities::vtkOpenGLRenderUtilities()
{
}

// ----------------------------------------------------------------------------
vtkOpenGLRenderUtilities::~vtkOpenGLRenderUtilities()
{
}

void vtkOpenGLRenderUtilities::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

// ---------------------------------------------------------------------------
// a program must be bound
// a VAO must be bound
void vtkOpenGLRenderUtilities::RenderQuad(
  float *verts,
  float *tcoords,
  vtkShaderProgram *program, vtkOpenGLVertexArrayObject *vao)
{
  GLuint iboData[] = {0, 1, 2, 0, 2, 3};
  vtkOpenGLRenderUtilities::RenderTriangles(verts, 4,
    iboData, 6,
    tcoords,
    program, vao);
}

// ---------------------------------------------------------------------------
// a program must be bound
// a VAO must be bound
void vtkOpenGLRenderUtilities::RenderTriangles(
  float *verts, unsigned int numVerts,
  GLuint *iboData, unsigned int numIndices,
  float *tcoords,
  vtkShaderProgram *program, vtkOpenGLVertexArrayObject *vao)
{
  if (!program || !vao || !verts)
    {
    vtkGenericWarningMacro(<< "Error must have verts, program and vao");
    }

  vtkNew<vtkOpenGLBufferObject> vbo;
  vbo->Upload(verts, numVerts*3, vtkOpenGLBufferObject::ArrayBuffer);
  vao->Bind();
  if (!vao->AddAttributeArray(program, vbo.Get(), "vertexMC", 0,
      sizeof(float)*3, VTK_FLOAT, 3, false))
    {
    vtkGenericWarningMacro(<< "Error setting 'vertexMC' in shader VAO.");
    }

  vtkNew<vtkOpenGLBufferObject> tvbo;
  if (tcoords)
    {
    tvbo->Upload(tcoords, numVerts*2, vtkOpenGLBufferObject::ArrayBuffer);
    if (!vao->AddAttributeArray(program, tvbo.Get(), "tcoordMC", 0,
        sizeof(float)*2, VTK_FLOAT, 2, false))
      {
      vtkGenericWarningMacro(<< "Error setting 'tcoordMC' in shader VAO.");
      }
    }

  vtkNew<vtkOpenGLBufferObject> ibo;
  vao->Bind();
  ibo->Upload(iboData, numIndices, vtkOpenGLBufferObject::ElementArrayBuffer);
  glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT,
    reinterpret_cast<const GLvoid *>(NULL));
  ibo->Release();
  ibo->ReleaseGraphicsResources();
  vao->RemoveAttributeArray("vertexMC");
  vao->RemoveAttributeArray("tcoordMC");
  vao->Release();
  vbo->Release();
  vbo->ReleaseGraphicsResources();
  if (tcoords)
    {
    tvbo->Release();
    tvbo->ReleaseGraphicsResources();
    }
}
