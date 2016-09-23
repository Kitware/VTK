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
#include "vtkShaderProgram.h"

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
    return;
  }

  if (!program->isBound())
  {
    vtkGenericWarningMacro("attempt to render to unbound program");
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

//------------------------------------------------------------------------------
std::string vtkOpenGLRenderUtilities::GetFullScreenQuadVertexShader()
{
  // Pass through:
  return "//VTK::System::Dec\n"
         "attribute vec4 ndCoordIn;\n"
         "attribute vec2 texCoordIn;\n"
         "varying vec2 texCoord;\n"
         "void main()\n"
         "{\n"
         "  gl_Position = ndCoordIn;\n"
         "  texCoord = texCoordIn;\n"
         "}\n";
}

//------------------------------------------------------------------------------
std::string vtkOpenGLRenderUtilities::GetFullScreenQuadFragmentShaderTemplate()
{
  return "//VTK::System::Dec\n"
         "//VTK::Output::Dec\n"
         "in vec2 texCoord;\n"
         "//VTK::FSQ::Decl\n"
         "void main()\n"
         "{\n"
         "//VTK::FSQ::Impl\n"
         "}\n";
}

//------------------------------------------------------------------------------
std::string vtkOpenGLRenderUtilities::GetFullScreenQuadGeometryShader()
{
  return "";
}

//------------------------------------------------------------------------------
bool vtkOpenGLRenderUtilities::PrepFullScreenVAO(vtkOpenGLBufferObject *vertBuf,
                                                 vtkOpenGLVertexArrayObject *vao,
                                                 vtkShaderProgram *prog)
{
  bool res;

  // ndCoord_x, ndCoord_y, texCoord_x, texCoord_y
  float verts[16] = {  1.f, 1.f, 1.f, 1.f,
                      -1.f, 1.f, 0.f, 1.f,
                       1.f,-1.f, 1.f, 0.f,
                      -1.f,-1.f, 0.f, 0.f };

  vertBuf->SetType(vtkOpenGLBufferObject::ArrayBuffer);
  res = vertBuf->Upload(verts, 16, vtkOpenGLBufferObject::ArrayBuffer);
  if (!res)
  {
    vtkGenericWarningMacro("Error uploading fullscreen quad vertex data.");
    return false;
  }

  vao->Bind();

  res = vao->AddAttributeArray(prog, vertBuf, "ndCoordIn", 0, 4 * sizeof(float),
                               VTK_FLOAT, 2, false);
  if (!res)
  {
    vao->Release();
    vtkGenericWarningMacro("Error binding ndCoords to VAO.");
    return false;
  }

  res = vao->AddAttributeArray(prog, vertBuf, "texCoordIn", 2 * sizeof(float),
                               4 * sizeof(float), VTK_FLOAT, 2, false);
  if (!res)
  {
    vao->Release();
    vtkGenericWarningMacro("Error binding texCoords to VAO.");
    return false;
  }

  vao->Release();
  return true;
}

//------------------------------------------------------------------------------
void vtkOpenGLRenderUtilities::DrawFullScreenQuad()
{
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
