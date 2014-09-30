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

#include "vtkBitArray.h"
#include "vtkCamera.h"
#include "vtkDataObject.h"
#include "vtkHardwareSelector.h"
#include "vtkMath.h"
#include "vtkMatrix3x3.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkProperty.h"
#include "vtkShader.h"
#include "vtkShaderProgram.h"
#include "vtkTransform.h"


#include "vtkglGlyph3DVSFragmentLit.h"

using vtkgl::replace;

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLGlyph3DHelper)

//-----------------------------------------------------------------------------
vtkOpenGLGlyph3DHelper::vtkOpenGLGlyph3DHelper()
{
  this->ModelTransformMatrix = NULL;
  this->ModelNormalMatrix = NULL;
  this->ModelColor = NULL;
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
  // deal with color
  if (vtkOpenGLRenderWindow::GetContextSupportsOpenGL32())
    {
    VSSource = replace(VSSource,"//VTK::Color::Dec",
                              "attribute vec4 glyphColor;\n"
                              "varying vec4 vertexColor;");
    }
  else
    {
    VSSource = replace(VSSource,
                       "//VTK::Color::Dec",
                       "uniform vec4 glyphColor;\n"
                       "varying vec4 vertexColor;");
    }
  VSSource = replace(VSSource,"//VTK::Color::Impl",
                            "vertexColor =  glyphColor;");


  // crate the material/color property declarations, and VS implementation
  // these are always defined
  std::string colorDec =
    "uniform float opacityUniform; // the fragment opacity\n"
    "uniform vec3 ambientColorUniform; // intensity weighted color\n"
    "uniform vec3 diffuseColorUniform; // intensity weighted color\n";
  // add some if we have a backface property
  if (actor->GetBackfaceProperty())
    {
    colorDec +=
      "uniform float opacityUniformBF; // the fragment opacity\n"
      "uniform vec3 ambientColorUniformBF; // intensity weighted color\n"
      "uniform vec3 diffuseColorUniformBF; // intensity weighted color\n";
    }
  // add more for specular
  if (lightComplexity)
    {
    colorDec +=
      "uniform vec3 specularColorUniform; // intensity weighted color\n"
      "uniform float specularPowerUniform;\n";
    if (actor->GetBackfaceProperty())
      {
      colorDec +=
        "uniform vec3 specularColorUniformBF; // intensity weighted color\n"
        "uniform float specularPowerUniformBF;\n";
      }
    }
  colorDec += "varying vec4 vertexColor;\n";
  FSSource = replace(FSSource,"//VTK::Color::Dec", colorDec);

  // now handle the more complex fragment shader implementation
  // the following are always defined variables.  We start
  // by assiging a default value from the uniform
  std::string colorImpl =
    "vec3 ambientColor;\n"
    "  vec3 diffuseColor;\n"
    "  float opacity;\n";
  if (lightComplexity)
    {
    colorImpl +=
      "  vec3 specularColor;\n"
      "  float specularPower;\n";
    }
  if (actor->GetBackfaceProperty())
    {
    if (lightComplexity)
      {
      colorImpl +=
        "  if (int(gl_FrontFacing) == 0) {\n"
        "    ambientColor = ambientColorUniformBF;\n"
        "    diffuseColor = diffuseColorUniformBF;\n"
        "    specularColor = specularColorUniformBF;\n"
        "    specularPower = specularPowerUniformBF;\n"
        "    opacity = opacityUniformBF; }\n"
        "  else {\n"
        "    ambientColor = ambientColorUniform;\n"
        "    diffuseColor = diffuseColorUniform;\n"
        "    specularColor = specularColorUniform;\n"
        "    specularPower = specularPowerUniform;\n"
        "    opacity = opacityUniform; }\n";
      }
    else
      {
      colorImpl +=
        "  if (int(gl_FrontFacing) == 0) {\n"
        "    ambientColor = ambientColorUniformBF;\n"
        "    diffuseColor = diffuseColorUniformBF;\n"
        "    opacity = opacityUniformBF; }\n"
        "  else {\n"
        "    ambientColor = ambientColorUniform;\n"
        "    diffuseColor = diffuseColorUniform;\n"
        "    opacity = opacityUniform; }\n";
      }
    }
  else
    {
    colorImpl +=
      "    ambientColor = ambientColorUniform;\n"
      "    diffuseColor = diffuseColorUniform;\n"
      "    opacity = opacityUniform;\n";
    if (lightComplexity)
      {
      colorImpl +=
        "    specularColor = specularColorUniform;\n"
        "    specularPower = specularPowerUniform;\n";
      }
    }

  // now handle scalar coloring
  if (this->ScalarMaterialMode == VTK_MATERIALMODE_AMBIENT ||
        (this->ScalarMaterialMode == VTK_MATERIALMODE_DEFAULT && actor->GetProperty()->GetAmbient() > actor->GetProperty()->GetDiffuse()))
    {
    FSSource = replace(FSSource,"//VTK::Color::Impl", colorImpl +
                                "  ambientColor = vertexColor.rgb;\n"
                                "  opacity = vertexColor.a;");
    }
  else if (this->ScalarMaterialMode == VTK_MATERIALMODE_DIFFUSE ||
        (this->ScalarMaterialMode == VTK_MATERIALMODE_DEFAULT && actor->GetProperty()->GetAmbient() <= actor->GetProperty()->GetDiffuse()))
    {
    FSSource = replace(FSSource,"//VTK::Color::Impl", colorImpl +
                                "  diffuseColor = vertexColor.rgb;\n"
                                "  opacity = vertexColor.a;");
    }
  else
    {
    FSSource = replace(FSSource,"//VTK::Color::Impl", colorImpl +
                                "  diffuseColor = vertexColor.rgb;\n"
                                "  ambientColor = vertexColor.rgb;\n"
                                "  opacity = vertexColor.a;");
    }

  if (vtkOpenGLRenderWindow::GetContextSupportsOpenGL32())
    {
    VSSource = replace(VSSource,
                       "//VTK::Glyph::Dec",
                       "attribute mat4 GCMCMatrix;");
    }
  else
    {
    VSSource = replace(VSSource,
                       "//VTK::Glyph::Dec",
                       "uniform mat4 GCMCMatrix;");
    }
  VSSource = replace(VSSource,
                     "//VTK::Glyph::Impl",
                     "vec4 vertex = GCMCMatrix * vertexMC;\n");

  // new code for normal matrix if we have normals
  if (this->Layout.NormalOffset)
    {
    if (vtkOpenGLRenderWindow::GetContextSupportsOpenGL32())
      {
      VSSource = replace(VSSource,
                                   "//VTK::Normal::Dec",
                                   "attribute vec3 normalMC;\n"
                                   "attribute mat3 glyphNormalMatrix;\n"
                                   "varying vec3 normalVCVarying;");
      }
    else
      {
      VSSource = replace(VSSource,
                                   "//VTK::Normal::Dec",
                                   "attribute vec3 normalMC;\n"
                                   "uniform mat3 glyphNormalMatrix;\n"
                                   "varying vec3 normalVCVarying;");
      }
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
      std::vector<float> &colors, std::vector<float> &matrices,
      std::vector<float> &normalMatrices, std::vector<vtkIdType> &pickIds,
      unsigned long pointMTime)
{
  if (vtkOpenGLRenderWindow::GetContextSupportsOpenGL32())
    {
    this->GlyphRenderInstances(ren, actor, numPts,
      colors, matrices, normalMatrices, pickIds, pointMTime);
    return;
    }

  bool primed = false;

  vtkHardwareSelector* selector = ren->GetSelector();
  bool selecting_points = selector && (selector->GetFieldAssociation() ==
    vtkDataObject::FIELD_ASSOCIATION_POINTS);

  for (vtkIdType inPtId = 0; inPtId < numPts; inPtId++)
    {
    if (selecting_points)
      {
      selector->RenderAttributeId(pickIds[inPtId]);
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

    program->SetUniform4f("glyphColor", &(colors[inPtId*4]));

    if (selector)
      {
      program->SetUniform3f("mapperIndex", selector->GetPropColorValue());
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

  if (this->ModelColor)
    {
    program->SetUniform4f("glyphColor", this->ModelColor);
    }
}

void vtkOpenGLGlyph3DHelper::GlyphRenderInstances(
    vtkRenderer* ren, vtkActor* actor, vtkIdType numPts,
    std::vector<float> &colors, std::vector<float> &matrices,
    std::vector<float> &normalMatrices, std::vector<vtkIdType> &pickIds,
    unsigned long pointMTime)
{
  vtkHardwareSelector* selector = ren->GetSelector();
  bool selecting_points = selector && (selector->GetFieldAssociation() ==
    vtkDataObject::FIELD_ASSOCIATION_POINTS);

  if (selecting_points)
    {
//    selector->RenderAttributeId(rgba[0] + (rgba[1] << 8) + (rgba[2] << 16));
    }

  this->RenderPieceStart(ren,actor);
  this->UpdateShader(this->Tris, ren, actor);

  // update the data buffers ?
  this->Tris.vao.Bind();
  if (pointMTime > this->InstanceBuffersLoadTime.GetMTime())
    {
    // add 3 new BOs?
    this->MatrixBuffer.Bind();
    this->MatrixBuffer.Upload(matrices, vtkgl::BufferObject::ArrayBuffer);
    if (!this->Tris.vao.AddAttributeMatrixWithDivisor(this->Tris.Program, this->MatrixBuffer,
        "GCMCMatrix", 0, 16*sizeof(float), VTK_FLOAT, 4, false, 1))
      {
      vtkErrorMacro(<< "Error setting 'GCMCMatrix' in shader VAO.");
      }
    this->MatrixBuffer.Release();

    this->NormalMatrixBuffer.Bind();
    this->NormalMatrixBuffer.Upload(normalMatrices, vtkgl::BufferObject::ArrayBuffer);
    if (!this->Tris.vao.AddAttributeMatrixWithDivisor(this->Tris.Program, this->NormalMatrixBuffer,
          "glyphNormalMatrix", 0, 9*sizeof(float), VTK_FLOAT, 3, false, 1))
      {
      vtkErrorMacro(<< "Error setting 'glyphNormalMatrix' in shader VAO.");
      }
    this->NormalMatrixBuffer.Release();

    this->ColorBuffer.Bind();
    this->ColorBuffer.Upload(colors, vtkgl::BufferObject::ArrayBuffer);
    if (!this->Tris.vao.AddAttributeArrayWithDivisor(this->Tris.Program, this->ColorBuffer,
          "glyphColor", 0, 4*sizeof(float), VTK_FLOAT, 4, false, 1))
      {
      vtkErrorMacro(<< "Error setting 'diffuse color' in shader VAO.");
      }
    this->ColorBuffer.Release();

    this->InstanceBuffersLoadTime.Modified();
    }

  vtkOpenGLCheckErrorMacro("failed after Render");

  this->Tris.ibo.Bind();
  glDrawElementsInstanced(GL_TRIANGLES,
                          static_cast<GLsizei>(this->Tris.indexCount),
                          GL_UNSIGNED_INT,
                          reinterpret_cast<const GLvoid *>(NULL),
                          numPts);
  vtkOpenGLCheckErrorMacro("failed after Render");

  this->Tris.ibo.Release();
  this->RenderPieceFinish(ren, actor);
}

//-----------------------------------------------------------------------------
void vtkOpenGLGlyph3DHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
