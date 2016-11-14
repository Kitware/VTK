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

#include "vtkOpenGLHelper.h"

#include "vtkBitArray.h"
#include "vtkCamera.h"
#include "vtkDataObject.h"
#include "vtkHardwareSelector.h"
#include "vtkMath.h"
#include "vtkMatrix3x3.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLBufferObject.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLIndexBufferObject.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkOpenGLVertexBufferObject.h"
#include "vtkProperty.h"
#include "vtkShader.h"
#include "vtkShaderProgram.h"
#include "vtkTransform.h"

#include "vtkGlyph3DVS.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLGlyph3DHelper)

//-----------------------------------------------------------------------------
vtkOpenGLGlyph3DHelper::vtkOpenGLGlyph3DHelper()
{
  this->NormalMatrixBuffer = vtkOpenGLBufferObject::New();
  this->MatrixBuffer = vtkOpenGLBufferObject::New();
  this->ColorBuffer = vtkOpenGLBufferObject::New();
  this->ModelTransformMatrix = NULL;
  this->ModelNormalMatrix = NULL;
  this->ModelColor = NULL;
  this->UseFastPath = false;
  this->UsingInstancing = false;
}

//-----------------------------------------------------------------------------
vtkOpenGLGlyph3DHelper::~vtkOpenGLGlyph3DHelper()
{
  this->NormalMatrixBuffer->Delete();
  this->NormalMatrixBuffer = 0;
  this->MatrixBuffer->Delete();
  this->MatrixBuffer = 0;
  this->ColorBuffer->Delete();
  this->ColorBuffer = 0;
}


// ---------------------------------------------------------------------------
// Description:
// Release any graphics resources that are being consumed by this mapper.
void vtkOpenGLGlyph3DHelper::ReleaseGraphicsResources(vtkWindow *window)
{
  this->NormalMatrixBuffer->ReleaseGraphicsResources();
  this->MatrixBuffer->ReleaseGraphicsResources();
  this->ColorBuffer->ReleaseGraphicsResources();
  this->Superclass::ReleaseGraphicsResources(window);
}

//-----------------------------------------------------------------------------
void vtkOpenGLGlyph3DHelper::GetShaderTemplate(
    std::map<vtkShader::Type, vtkShader *> shaders,
    vtkRenderer *ren, vtkActor *actor)
{
  this->Superclass::GetShaderTemplate(shaders,ren,actor);

  shaders[vtkShader::Vertex]->SetSource(vtkGlyph3DVS);
}

void vtkOpenGLGlyph3DHelper::ReplaceShaderPositionVC(
  std::map<vtkShader::Type, vtkShader *> shaders,
  vtkRenderer *ren, vtkActor *actor)
{
  std::string VSSource = shaders[vtkShader::Vertex]->GetSource();

  if (this->LastLightComplexity[this->LastBoundBO] > 0)
  {
    // we use vertex instead of vertexMC
    vtkShaderProgram::Substitute(VSSource,
      "//VTK::PositionVC::Impl",
      "vertexVCVSOutput = MCVCMatrix * vertex;\n"
      "  gl_Position = MCDCMatrix * vertex;\n");
  }
  else
  {
    vtkShaderProgram::Substitute(VSSource,
      "//VTK::PositionVC::Impl",
      "gl_Position = MCDCMatrix * vertex;\n");
  }

  shaders[vtkShader::Vertex]->SetSource(VSSource);

  this->Superclass::ReplaceShaderPositionVC(shaders,ren,actor);
}

void vtkOpenGLGlyph3DHelper::ReplaceShaderColor(
  std::map<vtkShader::Type, vtkShader *> shaders,
  vtkRenderer *ren, vtkActor *actor)
{
  std::string VSSource = shaders[vtkShader::Vertex]->GetSource();
  std::string FSSource = shaders[vtkShader::Fragment]->GetSource();

  // deal with color
  if (this->UsingInstancing)
  {
    vtkShaderProgram::Substitute(VSSource,
      "//VTK::Color::Dec",
      "attribute vec4 glyphColor;\n"
      "varying vec4 vertexColorVSOutput;");
  }
  else
  {
    vtkShaderProgram::Substitute(VSSource,
      "//VTK::Color::Dec",
      "uniform vec4 glyphColor;\n"
      "varying vec4 vertexColorVSOutput;");
  }
  vtkShaderProgram::Substitute(VSSource,"//VTK::Color::Impl",
    "vertexColorVSOutput =  glyphColor;");

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
  if (this->LastLightComplexity[this->LastBoundBO])
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
  colorDec += "varying vec4 vertexColorVSOutput;\n";
  vtkShaderProgram::Substitute(FSSource,"//VTK::Color::Dec", colorDec);

  // now handle the more complex fragment shader implementation
  // the following are always defined variables.  We start
  // by assiging a default value from the uniform
  std::string colorImpl =
    "vec3 ambientColor;\n"
    "  vec3 diffuseColor;\n"
    "  float opacity;\n";
  if (this->LastLightComplexity[this->LastBoundBO])
  {
    colorImpl +=
      "  vec3 specularColor;\n"
      "  float specularPower;\n";
  }
  if (actor->GetBackfaceProperty())
  {
    if (this->LastLightComplexity[this->LastBoundBO])
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
    if (this->LastLightComplexity[this->LastBoundBO])
    {
      colorImpl +=
        "    specularColor = specularColorUniform;\n"
        "    specularPower = specularPowerUniform;\n";
    }
  }

  // now handle scalar coloring
  if (this->ScalarMaterialMode == VTK_MATERIALMODE_AMBIENT ||
        (this->ScalarMaterialMode == VTK_MATERIALMODE_DEFAULT &&
         actor->GetProperty()->GetAmbient() > actor->GetProperty()->GetDiffuse()))
  {
    vtkShaderProgram::Substitute(FSSource,"//VTK::Color::Impl",
      colorImpl +
      "  ambientColor = vertexColorVSOutput.rgb;\n"
      "  opacity = vertexColorVSOutput.a;");
  }
  else if (this->ScalarMaterialMode == VTK_MATERIALMODE_DIFFUSE ||
        (this->ScalarMaterialMode == VTK_MATERIALMODE_DEFAULT &&
         actor->GetProperty()->GetAmbient() <= actor->GetProperty()->GetDiffuse()))
  {
    vtkShaderProgram::Substitute(FSSource,"//VTK::Color::Impl",
      colorImpl +
      "  diffuseColor = vertexColorVSOutput.rgb;\n"
      "  opacity = vertexColorVSOutput.a;");
  }
  else
  {
    vtkShaderProgram::Substitute(FSSource,"//VTK::Color::Impl",
      colorImpl +
      "  diffuseColor = vertexColorVSOutput.rgb;\n"
      "  ambientColor = vertexColorVSOutput.rgb;\n"
      "  opacity = vertexColorVSOutput.a;");
  }

  if (this->UsingInstancing)
  {
    vtkShaderProgram::Substitute(VSSource,
       "//VTK::Glyph::Dec",
       "attribute mat4 GCMCMatrix;");
  }
  else
  {
    vtkShaderProgram::Substitute(VSSource,
      "//VTK::Glyph::Dec",
      "uniform mat4 GCMCMatrix;");
  }
  vtkShaderProgram::Substitute(VSSource,
    "//VTK::Glyph::Impl",
    "vec4 vertex = GCMCMatrix * vertexMC;\n");

  shaders[vtkShader::Vertex]->SetSource(VSSource);
  shaders[vtkShader::Fragment]->SetSource(FSSource);

  this->Superclass::ReplaceShaderColor(shaders,ren,actor);
}

void vtkOpenGLGlyph3DHelper::ReplaceShaderNormal(
  std::map<vtkShader::Type, vtkShader *> shaders,
  vtkRenderer *ren, vtkActor *actor)
{
  std::string VSSource = shaders[vtkShader::Vertex]->GetSource();
  std::string FSSource = shaders[vtkShader::Fragment]->GetSource();

  // new code for normal matrix if we have normals
  if (this->VBO->NormalOffset)
  {
    if (this->UsingInstancing)
    {
      vtkShaderProgram::Substitute(VSSource,
         "//VTK::Normal::Dec",
         "uniform mat3 normalMatrix;\n"
         "attribute vec3 normalMC;\n"
         "attribute mat3 glyphNormalMatrix;\n"
         "varying vec3 normalVCVSOutput;");
    }
    else
    {
      vtkShaderProgram::Substitute(VSSource,
         "//VTK::Normal::Dec",
         "uniform mat3 normalMatrix;\n"
         "attribute vec3 normalMC;\n"
         "uniform mat3 glyphNormalMatrix;\n"
         "varying vec3 normalVCVSOutput;");
    }
    vtkShaderProgram::Substitute(VSSource, "//VTK::Normal::Impl",
      "normalVCVSOutput = normalMatrix * glyphNormalMatrix * normalMC;");
  }

  shaders[vtkShader::Vertex]->SetSource(VSSource);
  shaders[vtkShader::Fragment]->SetSource(FSSource);

  this->Superclass::ReplaceShaderNormal(shaders,ren,actor);
}


void vtkOpenGLGlyph3DHelper::ReplaceShaderClip(
  std::map<vtkShader::Type, vtkShader *> shaders,
  vtkRenderer *ren, vtkActor *actor)
{
  std::string VSSource = shaders[vtkShader::Vertex]->GetSource();

  // override one part of the clipping code
  if (this->GetNumberOfClippingPlanes())
  {
    // add all the clipping planes
    int numClipPlanes = this->GetNumberOfClippingPlanes();
    if (numClipPlanes > 6)
    {
      vtkErrorMacro(<< "OpenGL has a limit of 6 clipping planes");
      numClipPlanes = 6;
    }

    vtkShaderProgram::Substitute(VSSource,
       "//VTK::Clip::Impl",
       "for (int planeNum = 0; planeNum < numClipPlanes; planeNum++)\n"
       "    {\n"
       "    clipDistances[planeNum] = dot(clipPlanes[planeNum], vertex);\n"
       "    }\n");
  }

  shaders[vtkShader::Vertex]->SetSource(VSSource);

  this->Superclass::ReplaceShaderClip(shaders,ren,actor);
}

void vtkOpenGLGlyph3DHelper::ReplaceShaderPicking(
  std::map<vtkShader::Type, vtkShader *> shaders,
  vtkRenderer *, vtkActor *)
{
  std::string FSSource = shaders[vtkShader::Fragment]->GetSource();

  if (this->LastSelectionState >= vtkHardwareSelector::MIN_KNOWN_PASS)
  {
    vtkShaderProgram::Substitute(FSSource, "//VTK::Picking::Dec",
      "uniform vec3 mapperIndex;");
    vtkShaderProgram::Substitute(FSSource,
      "//VTK::Picking::Impl",
      "  gl_FragData[0] = vec4(mapperIndex,1.0);\n");
  }
  shaders[vtkShader::Fragment]->SetSource(FSSource);
}

void vtkOpenGLGlyph3DHelper::GlyphRender(
  vtkRenderer* ren,
  vtkActor* actor,
  vtkIdType numPts,
  std::vector<unsigned char> &colors,
  std::vector<float> &matrices,
  std::vector<float> &normalMatrices,
  std::vector<vtkIdType> &pickIds,
  vtkMTimeType pointMTime)
{
  // we always tell our triangle VAO to emulate unless we
  // have opngl 3.2 to be safe
  // this is because it seems that GLEW_ARB_vertex_array_object
  // does not always handle the attributes for GLEW_ARB_instanced_arrays
  this->Tris.VAO->SetForceEmulation(
    !vtkOpenGLRenderWindow::GetContextSupportsOpenGL32());

  this->CurrentInput = this->GetInput();
  this->UsingInstancing = false;

  vtkHardwareSelector* selector = ren->GetSelector();
  bool selecting_points = selector && (selector->GetFieldAssociation() ==
    vtkDataObject::FIELD_ASSOCIATION_POINTS);

  int representation = actor->GetProperty()->GetRepresentation();


#if GL_ES_VERSION_2_0 != 1 || GL_ES_VERSION_3_0 == 1
  if (actor->GetProperty()->GetRepresentation() == VTK_SURFACE &&
      !selector && GLEW_ARB_instanced_arrays)
  {
    this->GlyphRenderInstances(ren, actor, numPts,
      colors, matrices, normalMatrices, pointMTime);
    return;
  }
#endif

  bool primed = false;

  // First we do the triangles, update the shader, set uniforms, etc.
  GLenum mode = (representation == VTK_POINTS) ? GL_POINTS :
    (representation == VTK_WIREFRAME) ? GL_LINES : GL_TRIANGLES;
  if (selecting_points)
  {
#if GL_ES_VERSION_2_0 != 1
    glPointSize(6.0);
#endif
    mode = GL_POINTS;
  }

  for (vtkIdType inPtId = 0; inPtId < numPts; inPtId++)
  {
    if (selecting_points)
    {
      selector->RenderAttributeId(pickIds[inPtId]);
    }
    if (!primed)
    {
      this->RenderPieceStart(ren,actor);
      this->UpdateShaders(this->Tris, ren, actor);
      this->Tris.IBO->Bind();
      primed = true;
    }

    // handle the middle
    vtkShaderProgram *program = this->Tris.Program;

    // Apply the extra transform
    program->SetUniformMatrix4x4("GCMCMatrix", &(matrices[inPtId*16]));

    // for lit shaders set normal matrix
    if (this->LastLightComplexity[this->LastBoundBO] > 0 && this->VBO->NormalOffset &&
        !this->UsingInstancing)
    {
      program->SetUniformMatrix3x3("glyphNormalMatrix", &(normalMatrices[inPtId*9]));
    }

    program->SetUniform4uc("glyphColor", &(colors[inPtId*4]));

    if (selector)
    {
      program->SetUniform3f("mapperIndex", selector->GetPropColorValue());
    }

    glDrawRangeElements(mode, 0,
                        static_cast<GLuint>(this->VBO->VertexCount - 1),
                        static_cast<GLsizei>(this->Tris.IBO->IndexCount),
                        GL_UNSIGNED_INT,
                        reinterpret_cast<const GLvoid *>(NULL));
  }
  if (primed)
  {
    this->Tris.IBO->Release();
    this->RenderPieceFinish(ren,actor);
  }
}

//-----------------------------------------------------------------------------
void vtkOpenGLGlyph3DHelper::SetCameraShaderParameters(vtkOpenGLHelper &cellBO,
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
  if (this->LastLightComplexity[&cellBO] > 0 && this->ModelNormalMatrix &&
     this->VBO->NormalOffset && !this->UsingInstancing)
  {
    program->SetUniformMatrix3x3("glyphNormalMatrix", this->ModelNormalMatrix);
  }
}

//-----------------------------------------------------------------------------
void vtkOpenGLGlyph3DHelper::SetPropertyShaderParameters(vtkOpenGLHelper &cellBO,
                                                         vtkRenderer *ren, vtkActor *actor)
{
  // do the superclass and then reset a couple values
  this->Superclass::SetPropertyShaderParameters(cellBO,ren,actor);

  vtkShaderProgram *program = cellBO.Program;

  if (this->ModelColor)
  {
    program->SetUniform4uc("glyphColor", this->ModelColor);
  }
}

//-----------------------------------------------------------------------------
void vtkOpenGLGlyph3DHelper::SetMapperShaderParameters(vtkOpenGLHelper &cellBO,
                                                         vtkRenderer *ren, vtkActor *actor)
{
  this->Superclass::SetMapperShaderParameters(cellBO,ren,actor);

  vtkHardwareSelector* selector = ren->GetSelector();
  if (selector && selector->GetCurrentPass() == vtkHardwareSelector::ID_LOW24)
  {
    cellBO.Program->SetUniform3f("mapperIndex", selector->GetPropColorValue());
  }
}

#if GL_ES_VERSION_2_0 != 1 || GL_ES_VERSION_3_0 == 1
void vtkOpenGLGlyph3DHelper::GlyphRenderInstances(
    vtkRenderer* ren, vtkActor* actor, vtkIdType numPts,
    std::vector<unsigned char> &colors, std::vector<float> &matrices,
    std::vector<float> &normalMatrices,
    vtkMTimeType pointMTime)
{
  this->UsingInstancing = true;
  this->RenderPieceStart(ren,actor);
  this->UpdateShaders(this->Tris, ren, actor);

  // do the superclass and then reset a couple values
  if (this->Tris.IBO->IndexCount &&   // we have points and one of
      (this->VBOBuildTime > this->InstanceBuffersLoadTime ||
      this->Tris.ShaderSourceTime > this->InstanceBuffersLoadTime ||
      pointMTime > this->InstanceBuffersLoadTime.GetMTime()))
  {
    this->Tris.VAO->Bind();
    // add 3 new BOs?
    this->MatrixBuffer->Bind();
    this->MatrixBuffer->Upload(matrices, vtkOpenGLBufferObject::ArrayBuffer);
    if (!this->Tris.VAO->AddAttributeMatrixWithDivisor(this->Tris.Program, this->MatrixBuffer,
        "GCMCMatrix", 0, 16*sizeof(float), VTK_FLOAT, 4, false, 1))
    {
      vtkErrorMacro(<< "Error setting 'GCMCMatrix' in shader VAO.");
    }
    this->MatrixBuffer->Release();

    if (this->VBO->NormalOffset && this->LastLightComplexity[this->LastBoundBO] > 0)
    {
      this->NormalMatrixBuffer->Bind();
      this->NormalMatrixBuffer->Upload(
        normalMatrices, vtkOpenGLBufferObject::ArrayBuffer);
      if (!this->Tris.VAO->AddAttributeMatrixWithDivisor(
            this->Tris.Program, this->NormalMatrixBuffer,
            "glyphNormalMatrix", 0, 9*sizeof(float), VTK_FLOAT, 3, false, 1))
      {
        vtkErrorMacro(<< "Error setting 'glyphNormalMatrix' in shader VAO.");
      }
      this->NormalMatrixBuffer->Release();
    }

    if (this->Tris.Program->IsAttributeUsed("glyphColor"))
    {
      this->ColorBuffer->Bind();
      this->ColorBuffer->Upload(colors, vtkOpenGLBufferObject::ArrayBuffer);
      if (!this->Tris.VAO->AddAttributeArrayWithDivisor(
            this->Tris.Program, this->ColorBuffer,
            "glyphColor", 0, 4*sizeof(unsigned char), VTK_UNSIGNED_CHAR, 4, true, 1, false))
      {
        vtkErrorMacro(<< "Error setting 'diffuse color' in shader VAO.");
      }
      this->ColorBuffer->Release();
    }
    this->InstanceBuffersLoadTime.Modified();
  }

  this->Tris.IBO->Bind();
#if GL_ES_VERSION_3_0 == 1
  glDrawElementsInstanced(GL_TRIANGLES,
                        static_cast<GLsizei>(this->Tris.IBO->IndexCount),
                        GL_UNSIGNED_INT,
                        reinterpret_cast<const GLvoid *>(NULL),
                        numPts);
#else
  if (GLEW_ARB_instanced_arrays)
  {
    glDrawElementsInstancedARB(GL_TRIANGLES,
                          static_cast<GLsizei>(this->Tris.IBO->IndexCount),
                          GL_UNSIGNED_INT,
                          reinterpret_cast<const GLvoid *>(NULL),
                          numPts);
  }
#endif
  vtkOpenGLCheckErrorMacro("failed after Render");

  this->Tris.IBO->Release();
  this->RenderPieceFinish(ren, actor);
}
#endif

//-----------------------------------------------------------------------------
void vtkOpenGLGlyph3DHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
