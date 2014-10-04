/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLGlyph3DHelper.h"

#include "vtkglVBOHelper.h"

#include "vtkCamera.h"
#include "vtkTransform.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"
#include "vtkProperty.h"
#include "vtkMatrix3x3.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"

#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLShaderCache.h"

#include "vtkHardwareSelector.h"

#include "vtkShader.h"
#include "vtkShaderProgram.h"

#include "vtkBitArray.h"
#include "vtkDataObject.h"

#include "vtkglGlyph3DVSFragmentLit.h"

using vtkgl::replace;

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLGlyph3DHelper)

//-----------------------------------------------------------------------------
vtkOpenGLGlyph3DHelper::vtkOpenGLGlyph3DHelper()
{
  this->ModelTransformMatrix = NULL;
  this->ModelNormalMatrix = NULL;
}

//-----------------------------------------------------------------------------
void vtkOpenGLGlyph3DHelper::GetShaderTemplate(std::string &VSSource,
                                          std::string &FSSource,
                                          std::string &GSSource,
                                          int lightComplexity, vtkRenderer* ren, vtkActor *actor)
{
  this->Superclass::GetShaderTemplate(VSSource,FSSource,GSSource,lightComplexity,ren,actor);

  VSSource = vtkglGlyph3DVSFragmentLit;
}

void vtkOpenGLGlyph3DHelper::ReplaceShaderValues(std::string &VSSource,
                                                 std::string &FSSource,
                                                 std::string &GSSource,
                                                 int lightComplexity,
                                                 vtkRenderer* ren,
                                                 vtkActor *actor)
{
  // new code for normal matrix if we have normals
  if (this->Layout.NormalOffset)
    {
    VSSource = replace(VSSource,
                                 "//VTK::Normal::Dec",
                                 "attribute vec3 normalMC; varying vec3 normalVCVarying;");
    VSSource = replace(VSSource,
                                 "//VTK::Normal::Impl",
                                 "normalVCVarying = normalMatrix * glyphNormalMatrix * normalMC;");
    }

  this->Superclass::ReplaceShaderValues(VSSource,FSSource,GSSource,lightComplexity,ren,actor);
}

//-----------------------------------------------------------------------------
vtkOpenGLGlyph3DHelper::~vtkOpenGLGlyph3DHelper()
{
}

void vtkOpenGLGlyph3DHelper::GlyphRender(vtkRenderer* ren, vtkActor* actor, vtkIdType numPts,
      std::vector<unsigned char> &colors, std::vector<float> &matrices,
      std::vector<float> &normalMatrices)
{
  bool primed = false;
  unsigned char rgba[4];

  vtkHardwareSelector* selector = ren->GetSelector();
  bool selecting_points = selector && (selector->GetFieldAssociation() ==
    vtkDataObject::FIELD_ASSOCIATION_POINTS);

  for (vtkIdType inPtId = 0; inPtId < numPts; inPtId++)
    {
    rgba[0] = colors[inPtId*4];
    rgba[1] = colors[inPtId*4+1];
    rgba[2] = colors[inPtId*4+2];
    rgba[3] = colors[inPtId*4+3];

    if (selecting_points)
      {
      selector->RenderAttributeId(rgba[0] + (rgba[1] << 8) + (rgba[2] << 16));
      }
    if (!primed)
      {
      this->RenderPieceStart(ren,actor);
      this->UpdateShader(this->Tris, ren, actor);
      this->Tris.ibo.Bind();
      primed = true;
      }

    // handle the middle
    vtkShaderProgram *program = this->Tris.Program;
    vtkgl::VBOLayout &layout = this->Layout;

    // Apply the extra transform
    program->SetUniformMatrix4x4("GCMCMatrix", &(matrices[inPtId*16]));

    // for lit shaders set normal matrix
    if (this->LastLightComplexity > 0)
      {
      program->SetUniformMatrix3x3("glyphNormalMatrix", &(normalMatrices[inPtId*9]));
      }

    // Query the actor for some of the properties that can be applied.
    float diffuseColor[3] = {rgba[0]/255.0f,rgba[1]/255.0f,rgba[2]/255.0f};
    float opacity = rgba[3]/255.0f;

    program->SetUniformf("opacityUniform", opacity);
    program->SetUniform3f("diffuseColorUniform", diffuseColor);

    if (selector)
      {
      program->SetUniform3f("mapperIndex", selector->GetPropColorValue());
      float *fv = selector->GetPropColorValue();
      int iv = (int)(fv[0]*255) + (int)(fv[1]*255)*256;
      if (iv == 0)
        {
        abort();
        }
      }

    // First we do the triangles, update the shader, set uniforms, etc.
    if (actor->GetProperty()->GetRepresentation() == VTK_POINTS)
      {
      glDrawRangeElements(GL_POINTS, 0,
                          static_cast<GLuint>(layout.VertexCount - 1),
                          static_cast<GLsizei>(this->Tris.indexCount),
                          GL_UNSIGNED_INT,
                          reinterpret_cast<const GLvoid *>(NULL));
      }
    if (actor->GetProperty()->GetRepresentation() == VTK_WIREFRAME)
      {
      // TODO wireframe of triangles is not lit properly right now
      // you either have to generate normals and send them down
      // or use a geometry shader.
      glMultiDrawElements(GL_LINE_LOOP,
                        (GLsizei *)(&this->Tris.elementsArray[0]),
                        GL_UNSIGNED_INT,
                        reinterpret_cast<const GLvoid **>(&(this->Tris.offsetArray[0])),
                        (GLsizei)this->Tris.offsetArray.size());
      }
    if (actor->GetProperty()->GetRepresentation() == VTK_SURFACE)
      {
      glDrawRangeElements(GL_TRIANGLES, 0,
                          static_cast<GLuint>(layout.VertexCount - 1),
                          static_cast<GLsizei>(this->Tris.indexCount),
                          GL_UNSIGNED_INT,
                          reinterpret_cast<const GLvoid *>(NULL));
      }
    }
  if (primed)
    {
    this->Tris.ibo.Release();
    this->RenderPieceFinish(ren,actor);
    }
}

//-----------------------------------------------------------------------------
void vtkOpenGLGlyph3DHelper::SetCameraShaderParameters(vtkgl::CellBO &cellBO,
                                                    vtkRenderer* ren, vtkActor *actor)
{
  // do the superclass and then reset a couple values
  this->Superclass::SetCameraShaderParameters(cellBO,ren,actor);

  vtkShaderProgram *program = cellBO.Program;

  // Apply the extra transform
  if (this->ModelTransformMatrix)
    {
    program->SetUniformMatrix4x4("GCMCMatrix", this->ModelTransformMatrix);
    }

  // for lit shaders set normal matrix
  if (this->LastLightComplexity > 0 && this->ModelNormalMatrix)
    {
    program->SetUniformMatrix3x3("glyphNormalMatrix", this->ModelNormalMatrix);
    }
}

//-----------------------------------------------------------------------------
void vtkOpenGLGlyph3DHelper::SetPropertyShaderParameters(vtkgl::CellBO &cellBO,
                                                       vtkRenderer *ren, vtkActor *actor)
{
  // do the superclass and then reset a couple values
  this->Superclass::SetPropertyShaderParameters(cellBO,ren,actor);

  vtkShaderProgram *program = cellBO.Program;

  // Override the model color when the value was set directly on the mapper.
  float diffuseColor[3];
  for (int i = 0; i < 3; ++i)
    {
    diffuseColor[i] = this->ModelColor[i]/255.0;
    }
  float opacity = this->ModelColor[3]/255.0;

  program->SetUniformf("opacityUniform", opacity);
  program->SetUniform3f("diffuseColorUniform", diffuseColor);
}


//-----------------------------------------------------------------------------
void vtkOpenGLGlyph3DHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
