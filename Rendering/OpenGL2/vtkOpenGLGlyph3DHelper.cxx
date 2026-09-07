// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOpenGLGlyph3DHelper.h"

#include "vtkOpenGLHelper.h"

#include "vtkCamera.h"
#include "vtkDataObject.h"
#include "vtkHardwareSelector.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLBufferObject.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLIndexBufferObject.h"
#include "vtkOpenGLInstanceCulling.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLResourceFreeCallback.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLState.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkOpenGLVertexBufferObjectGroup.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkShader.h"
#include "vtkShaderProgram.h"
#include "vtkTextureObject.h"
#include "vtkTransformFeedback.h"

#include "vtkGlyph3DVS.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN

namespace
{
// Square lattice covering the disc of radius lineWidth/2, spacing lineWidth/ceil(lineWidth)
// so consecutive samples are never more than 1 px apart. Corners outside the disc are
// dropped. Returns pixel-space offsets; the shader converts to NDC.
void BuildLineWidthOffsets(float lineWidth, std::vector<float>& offsets)
{
  offsets.clear();
  if (lineWidth <= 1.0f)
  {
    offsets.push_back(0.0f);
    offsets.push_back(0.0f);
    return;
  }

  const float half = 0.5f * lineWidth;
  const int steps = 2 * vtkMath::Ceil(half);
  const float step = lineWidth / static_cast<float>(steps);
  const float r2 = half * half + 1e-4f;

  offsets.reserve(2 * (steps + 1) * (steps + 1));
  for (int j = 0; j <= steps; ++j)
  {
    const float y = j * step - half;
    for (int i = 0; i <= steps; ++i)
    {
      const float x = i * step - half;
      if (x * x + y * y <= r2)
      {
        offsets.push_back(x);
        offsets.push_back(y);
      }
    }
  }
}
} // anonymous namespace

vtkStandardNewMacro(vtkOpenGLGlyph3DHelper);

//------------------------------------------------------------------------------
vtkOpenGLGlyph3DHelper::vtkOpenGLGlyph3DHelper()
{
  this->UsingInstancing = false;
  this->PopulateSelectionSettings = 0;

  // Shift and Scale are not used in this mapper producing errors when the Shift Scale
  // feature was enabled by the superclass.
  // GCMCMatrix could be modified accordingly but Shift and Scale coefficients are
  // computed with the glyph source and not the point cloud.
  // Disabling it for now.
  this->SetVBOShiftScaleMethod(ShiftScaleMethodType::DISABLE_SHIFT_SCALE);
}

//------------------------------------------------------------------------------
// Description:
// Release any graphics resources that are being consumed by this mapper.
void vtkOpenGLGlyph3DHelper::ReleaseGraphicsResources(vtkWindow* window)
{
  if (!this->ResourceCallback->IsReleasing())
  {
    this->ResourceCallback->Release();
    return;
  }

  this->InstanceBuffersBuildTime = vtkTimeStamp();
  this->NormalMatrixBuffer->ReleaseGraphicsResources();
  this->MatrixBuffer->ReleaseGraphicsResources();
  this->ColorBuffer->ReleaseGraphicsResources();
  if (this->LineWidthOffsetsTexture)
  {
    this->LineWidthOffsetsTexture->ReleaseGraphicsResources(window);
    this->LineWidthOffsetsTexture = nullptr;
  }
  // Force the offsets to be rebuilt and re-uploaded into a new texture.
  this->CachedLineWidth = -1.0;
  this->Superclass::ReleaseGraphicsResources(window);
}

//------------------------------------------------------------------------------
void vtkOpenGLGlyph3DHelper::GetShaderTemplate(
  std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* actor)
{
  this->Superclass::GetShaderTemplate(shaders, ren, actor);

  shaders[vtkShader::Vertex]->SetSource(vtkGlyph3DVS);
#ifdef GL_ES_VERSION_3_0
  shaders[vtkShader::Geometry]->SetSource("");
#endif
}

//------------------------------------------------------------------------------
void vtkOpenGLGlyph3DHelper::ReplaceShaderValues(
  std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* actor)
{
  this->Superclass::ReplaceShaderValues(shaders, ren, actor);
#ifdef GL_ES_VERSION_3_0
  this->ReplaceShaderPointSize(shaders, ren, actor);
  this->ReplaceShaderWideLines(shaders, ren, actor);
#endif
}

//------------------------------------------------------------------------------
void vtkOpenGLGlyph3DHelper::ReplaceShaderPositionVC(
  std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* actor)
{
  std::string VSSource = shaders[vtkShader::Vertex]->GetSource();

  if (this->PrimitiveInfo[this->LastBoundBO].LastLightComplexity > 0)
  {
    // we use vertex instead of vertexMC
    vtkShaderProgram::Substitute(VSSource, "//VTK::PositionVC::Impl",
      "vertexVCVSOutput = MCVCMatrix * vertex;\n"
      "  gl_Position = MCDCMatrix * vertex;\n");
  }
  else
  {
    vtkShaderProgram::Substitute(
      VSSource, "//VTK::PositionVC::Impl", "gl_Position = MCDCMatrix * vertex;\n");
  }

  shaders[vtkShader::Vertex]->SetSource(VSSource);

  this->Superclass::ReplaceShaderPositionVC(shaders, ren, actor);
}

//------------------------------------------------------------------------------
void vtkOpenGLGlyph3DHelper::ReplaceShaderColor(
  std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* actor)
{
  std::string VSSource = shaders[vtkShader::Vertex]->GetSource();
  std::string FSSource = shaders[vtkShader::Fragment]->GetSource();
  std::string GSSource = shaders[vtkShader::Geometry]->GetSource();

  // deal with color
  if (this->UsingInstancing)
  {
    vtkShaderProgram::Substitute(VSSource, "//VTK::Color::Dec",
      "in vec4 glyphColor;\n"
      "out vec4 vertexColorVSOutput;");
    vtkShaderProgram::Substitute(GSSource, "//VTK::Color::Dec",
      "in vec4 vertexColorVSOutput[];\n"
      "out vec4 vertexColorGSOutput;");
    vtkShaderProgram::Substitute(
      GSSource, "//VTK::Color::Impl", "vertexColorGSOutput = vertexColorVSOutput[i];");
    vtkShaderProgram::Substitute(
      VSSource, "//VTK::Color::Impl", "vertexColorVSOutput =  glyphColor;");
    vtkShaderProgram::Substitute(FSSource, "//VTK::Color::Dec",
      "in vec4 vertexColorVSOutput;\n"
      "//VTK::Color::Dec",
      false);
  }
  else
  {
    vtkShaderProgram::Substitute(VSSource, "//VTK::Color::Dec", "");
    vtkShaderProgram::Substitute(FSSource, "//VTK::Color::Dec",
      "uniform vec4 glyphColor;\n"
      "//VTK::Color::Dec",
      false);
    vtkShaderProgram::Substitute(FSSource, "//VTK::Color::Impl",
      "vec4 vertexColorVSOutput = glyphColor;\n"
      "//VTK::Color::Impl",
      false);
  }

  // now handle scalar coloring
  if (!this->DrawingVertices)
  {
    if (actor->GetBackfaceProperty())
    {
      vtkShaderProgram::Substitute(FSSource, "//VTK::Color::Impl",
        "//VTK::Color::Impl\n"
        "  diffuseColor = (gl_FrontFacing) ? diffuseIntensity * vertexColorVSOutput.rgb : "
        "diffuseIntensityBF * diffuseColorUniformBF;\n"
        "  ambientColor = (gl_FrontFacing) ? ambientIntensity * vertexColorVSOutput.rgb : "
        "ambientIntensityBF * ambientColorUniformBF;\n"
        "  opacity = (gl_FrontFacing) ? opacity * vertexColorVSOutput.a : opacityUniformBF;");
    }
    else
    {
      vtkShaderProgram::Substitute(FSSource, "//VTK::Color::Impl",
        "//VTK::Color::Impl\n"
        "  diffuseColor = diffuseIntensity * vertexColorVSOutput.rgb;\n"
        "  ambientColor = ambientIntensity * vertexColorVSOutput.rgb;\n"
        "  opacity = opacity * vertexColorVSOutput.a;\n");
    }
  }

  if (this->UsingInstancing)
  {
    vtkShaderProgram::Substitute(VSSource, "//VTK::Glyph::Dec", "in mat4 GCMCMatrix;");
  }
  else
  {
    vtkShaderProgram::Substitute(VSSource, "//VTK::Glyph::Dec", "uniform mat4 GCMCMatrix;");
  }
  vtkShaderProgram::Substitute(
    VSSource, "//VTK::Glyph::Impl", "vec4 vertex = GCMCMatrix * vertexMC;\n");

  shaders[vtkShader::Vertex]->SetSource(VSSource);
  shaders[vtkShader::Fragment]->SetSource(FSSource);
  shaders[vtkShader::Geometry]->SetSource(GSSource);

  this->Superclass::ReplaceShaderColor(shaders, ren, actor);
}

//------------------------------------------------------------------------------
void vtkOpenGLGlyph3DHelper::ReplaceShaderNormal(
  std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* actor)
{
  std::string VSSource = shaders[vtkShader::Vertex]->GetSource();
  std::string FSSource = shaders[vtkShader::Fragment]->GetSource();

  // new code for normal matrix if we have normals
  if (this->VBOs->GetNumberOfComponents("normalMC") == 3)
  {
    if (this->UsingInstancing)
    {
      vtkShaderProgram::Substitute(VSSource, "//VTK::Normal::Dec",
        "uniform mat3 normalMatrix;\n"
        "in vec3 normalMC;\n"
        "in mat3 glyphNormalMatrix;\n"
        "out vec3 normalVCVSOutput;");
    }
    else
    {
      vtkShaderProgram::Substitute(VSSource, "//VTK::Normal::Dec",
        "uniform mat3 normalMatrix;\n"
        "in vec3 normalMC;\n"
        "uniform mat3 glyphNormalMatrix;\n"
        "out vec3 normalVCVSOutput;");
    }
    vtkShaderProgram::Substitute(VSSource, "//VTK::Normal::Impl",
      "normalVCVSOutput = normalMatrix * glyphNormalMatrix * normalMC;");
  }

  shaders[vtkShader::Vertex]->SetSource(VSSource);
  shaders[vtkShader::Fragment]->SetSource(FSSource);

  this->Superclass::ReplaceShaderNormal(shaders, ren, actor);
}

void vtkOpenGLGlyph3DHelper::ReplaceShaderClip(
  std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* actor)
{
  std::string VSSource = shaders[vtkShader::Vertex]->GetSource();

  // override one part of the clipping code
  if (this->GetNumberOfClippingPlanes())
  {
    // add all the clipping planes
    int numClipPlanes = this->GetNumberOfClippingPlanes();
    if (numClipPlanes > 6)
    {
      vtkErrorMacro("OpenGL has a limit of 6 clipping planes");
    }

    vtkShaderProgram::Substitute(VSSource, "//VTK::Clip::Impl",
      "for (int planeNum = 0; planeNum < numClipPlanes; planeNum++)\n"
      "    {\n"
      "    clipDistancesVSOutput[planeNum] = dot(clipPlanes[planeNum], vertex);\n"
      "    }\n");
  }

  shaders[vtkShader::Vertex]->SetSource(VSSource);
  this->Superclass::ReplaceShaderClip(shaders, ren, actor);
#if defined(GL_ES_VERSION_3_0)
  VSSource = shaders[vtkShader::Vertex]->GetSource();
  vtkShaderProgram::Substitute(
    VSSource, "uniform int numClipPlanes", "uniform highp int numClipPlanes");
  auto FSSource = shaders[vtkShader::Fragment]->GetSource();
  vtkShaderProgram::Substitute(
    FSSource, "uniform int numClipPlanes", "uniform highp int numClipPlanes");
  shaders[vtkShader::Fragment]->SetSource(FSSource);
  shaders[vtkShader::Vertex]->SetSource(VSSource);
#endif
}

//------------------------------------------------------------------------------
void vtkOpenGLGlyph3DHelper::ReplaceShaderPicking(
  std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer*, vtkActor*)
{
  std::string FSSource = shaders[vtkShader::Fragment]->GetSource();

  if (this->LastSelectionState >= vtkHardwareSelector::MIN_KNOWN_PASS)
  {
    vtkShaderProgram::Substitute(FSSource, "//VTK::Picking::Dec", "uniform vec3 mapperIndex;");
    vtkShaderProgram::Substitute(
      FSSource, "//VTK::Picking::Impl", "  gl_FragData[0] = vec4(mapperIndex,1.0);\n");
  }
  shaders[vtkShader::Fragment]->SetSource(FSSource);
}

//------------------------------------------------------------------------------
void vtkOpenGLGlyph3DHelper::ReplaceShaderPointSize(
  std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer*, vtkActor*)
{
  std::string VSSource = shaders[vtkShader::Vertex]->GetSource();
  // Point size
  vtkShaderProgram::Substitute(VSSource, "//VTK::PointSizeGLES30::Dec", "uniform float pointSize;");
  vtkShaderProgram::Substitute(
    VSSource, "//VTK::PointSizeGLES30::Impl", "gl_PointSize = pointSize;");
  shaders[vtkShader::Vertex]->SetSource(VSSource);
}

//------------------------------------------------------------------------------
void vtkOpenGLGlyph3DHelper::ReplaceShaderWideLines(
  std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* renderer, vtkActor* actor)
{
  // Thick lines are emulated by drawing shifted copies of each line, one per instance.
  // gl_InstanceID is always zero outside of an instanced draw call, so this must not be
  // applied when the glyphs are drawn one at a time. See GlyphRender.
  if (this->UsingInstancing && this->HaveWideLines(renderer, actor))
  {
    // The offsets are fetched from a texture rather than a uniform array so that the number of
    // samples is limited by GL_MAX_TEXTURE_SIZE instead of GL_MAX_VERTEX_UNIFORM_VECTORS, whose
    // GLES 3.0 minimum of 256 vectors is easily exhausted by a wide line. This also keeps the
    // shader source independent of the line width, so changing the width no longer rebuilds it.
    // The samplerBuffer is emulated with a 2D texture on GLES, see
    // vtkOpenGLShaderCache::ReplaceShaderValues.
    std::string VSSource = shaders[vtkShader::Vertex]->GetSource();
    vtkShaderProgram::Substitute(VSSource, "//VTK::LineWidthGLES30::Dec",
      R"(uniform vec2 viewportSize;
uniform float halfLineWidth;
uniform int lineWidthCopies;
uniform highp samplerBuffer lineWidthOffsets;
)");
    vtkShaderProgram::Substitute(VSSource, "//VTK::LineWidthGLES30::Impl",
      R"(if (halfLineWidth > 0.0)
  {
    int lineCopyInstanceID = gl_InstanceID % lineWidthCopies;
    float w = gl_Position.w;
    vec3 ndc = gl_Position.xyz / w;
    ndc.xy += 2.0 * texelFetchBuffer(lineWidthOffsets, lineCopyInstanceID).xy / viewportSize;
    gl_Position = vec4(ndc * w, w);
  })");
    shaders[vtkShader::Vertex]->SetSource(VSSource);
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLGlyph3DHelper::GlyphRender(vtkRenderer* ren, vtkActor* actor, vtkIdType numPts,
  std::vector<unsigned char>& colors, std::vector<float>& matrices,
  std::vector<float>& normalMatrices, std::vector<vtkIdType>& pickIds, vtkMTimeType pointMTime,
  bool culling)
{
  this->ResourceCallback->RegisterGraphicsResources(
    static_cast<vtkOpenGLRenderWindow*>(ren->GetRenderWindow()));

  this->UsingInstancing = false;

  vtkHardwareSelector* selector = ren->GetSelector();
  if (!selector && GLAD_GL_ARB_instanced_arrays)
  {
    // if there is no triangle, culling is useless.
    // GLAD_GL_ARB_gpu_shader5 is needed by the culling shader.
#ifndef GL_ES_VERSION_3_0
    if (this->CurrentInput->GetNumberOfPolys() <= 0 || !GLAD_GL_ARB_gpu_shader5 ||
      !GLAD_GL_ARB_transform_feedback3)
    {
      culling = false;
    }
#else
    // disable culling on OpenGL ES
    culling = false;
#endif

    this->GlyphRenderInstances(
      ren, actor, numPts, colors, matrices, normalMatrices, pointMTime, culling);
    return;
  }

  bool selecting_points =
    selector && (selector->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_POINTS);

  int representation = actor->GetProperty()->GetRepresentation();

  this->RenderPieceStart(ren, actor);

  vtkOpenGLRenderWindow* renWin = static_cast<vtkOpenGLRenderWindow*>(ren->GetRenderWindow());
  vtkOpenGLState* ostate = renWin->GetState();

  if (selecting_points)
  {
#ifndef GL_ES_VERSION_3_0
    ostate->vtkglPointSize(6.0);
#else
    (void)ostate;
#endif
    representation = GL_POINTS;
  }
  int iEnd = vtkOpenGLPolyDataMapper::PrimitiveEnd;
  if (!actor->GetProperty()->GetVertexVisibility() ||
    (selector && selector->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_POINTS))
  {
    // when selecting points, the selection pass renders points at a larger size.
    // so don't show vertices as they might conflict with the selection pass.
    iEnd = vtkOpenGLPolyDataMapper::PrimitiveVertices;
  }
  int numVerts = this->VBOs->GetNumberOfTuples("vertexMC");
  for (int i = PrimitiveStart; i < iEnd; i++)
  {
    this->DrawingVertices = i > PrimitiveTriStrips;
    if (this->Primitives[i].IBO->IndexCount)
    {
      this->UpdateShaders(this->Primitives[i], ren, actor);
      GLenum mode = this->GetOpenGLMode(representation, i);
      this->Primitives[i].IBO->Bind();
      for (vtkIdType inPtId = 0; inPtId < numPts; inPtId++)
      {
        // handle the middle
        vtkShaderProgram* program = this->Primitives[i].Program;

        if (!program)
        {
          return;
        }

        // Apply the extra transform
        program->SetUniformMatrix4x4("GCMCMatrix", &(matrices[inPtId * 16]));

        // for lit shaders set normal matrix
        if (this->PrimitiveInfo[this->LastBoundBO].LastLightComplexity > 0 &&
          this->VBOs->GetNumberOfComponents("normalMC") == 3 && !this->UsingInstancing)
        {
          program->SetUniformMatrix3x3("glyphNormalMatrix", &(normalMatrices[inPtId * 9]));
        }

        program->SetUniform4uc("glyphColor", &(colors[inPtId * 4]));

        if (selector)
        {
          if (selector->GetCurrentPass() == vtkHardwareSelector::POINT_ID_LOW24 ||
            selector->GetCurrentPass() == vtkHardwareSelector::POINT_ID_HIGH24 ||
            selector->GetCurrentPass() == vtkHardwareSelector::CELL_ID_LOW24 ||
            selector->GetCurrentPass() == vtkHardwareSelector::CELL_ID_HIGH24)
          {
            selector->SetPropColorValue(pickIds[inPtId]);
          }
          program->SetUniform3f("mapperIndex", selector->GetPropColorValue());
        }
#ifdef GL_ES_VERSION_3_0
        if (selecting_points)
        {
          program->SetUniformf("pointSize", 6.0);
        }
        else if (mode == GL_POINTS)
        {
          // set point size from actor property when drawing points and not selecting points
          const float pointSize = actor->GetProperty()->GetPointSize();
          program->SetUniformf("pointSize", pointSize);
        }
#endif
        glDrawRangeElements(mode, 0, static_cast<GLuint>(numVerts - 1),
          static_cast<GLsizei>(this->Primitives[i].IBO->IndexCount), GL_UNSIGNED_INT, nullptr);
      }
      this->Primitives[i].IBO->Release();
    }
  }
  this->RenderPieceFinish(ren, actor);
}

//------------------------------------------------------------------------------
void vtkOpenGLGlyph3DHelper::SetMapperShaderParameters(
  vtkOpenGLHelper& cellBO, vtkRenderer* renderer, vtkActor* actor)
{
  this->Superclass::SetMapperShaderParameters(cellBO, renderer, actor);
#ifdef GL_ES_VERSION_3_0
  // Must match the condition guarding the shader code in ReplaceShaderWideLines.
  if (this->UsingInstancing && this->HaveWideLines(renderer, actor))
  {
    int vp[4] = {};
    auto* renWin = vtkOpenGLRenderWindow::SafeDownCast(renderer->GetRenderWindow());
    vtkOpenGLState* ostate = renWin->GetState();
    ostate->vtkglGetIntegerv(GL_VIEWPORT, vp);
    float vpSize[2];
    vpSize[0] = vp[2];
    vpSize[1] = vp[3];
    const float lineWidth = actor->GetProperty()->GetLineWidth();

    // Rebuild and re-upload the sample offsets when the line width changes. This is done here
    // rather than while building the shader because the draw call needs the offset count even on
    // the frames where the shader is reused.
    if (lineWidth != this->CachedLineWidth || !this->LineWidthOffsetsTexture)
    {
      BuildLineWidthOffsets(lineWidth, this->LineWidthOffsets);
      this->CachedLineWidth = lineWidth;
      if (!this->LineWidthOffsetsTexture)
      {
        this->LineWidthOffsetsTexture = vtkSmartPointer<vtkTextureObject>::New();
      }
      this->LineWidthOffsetsTexture->SetContext(renWin);
      this->LineWidthOffsetsTexture->EmulateTextureBufferWith2DTexturesFromRaw(
        static_cast<unsigned int>(this->LineWidthOffsets.size() / 2), 2, VTK_FLOAT,
        this->LineWidthOffsets.data());
    }

    const int lineWidthCopies = static_cast<int>(this->LineWidthOffsets.size() / 2);
    this->LineWidthOffsetsTexture->Activate();
    cellBO.Program->SetUniform2f("viewportSize", vpSize);
    cellBO.Program->SetUniformf("halfLineWidth", 0.5 * lineWidth);
    cellBO.Program->SetUniformi("lineWidthCopies", lineWidthCopies);
    cellBO.Program->SetUniformi(
      "lineWidthOffsets", this->LineWidthOffsetsTexture->GetTextureUnit());
    vtkOpenGLCheckErrorMacro("failed after WideLines uniform update");
  }
#endif
}

//------------------------------------------------------------------------------
void vtkOpenGLGlyph3DHelper::GlyphRenderInstances(vtkRenderer* ren, vtkActor* actor,
  vtkIdType numPts, std::vector<unsigned char>& colors, std::vector<float>& matrices,
  std::vector<float>& normalMatrices, vtkMTimeType pointMTime, bool culling)
{
  this->UsingInstancing = true;
  this->RenderPieceStart(ren, actor);
  int representation = actor->GetProperty()->GetRepresentation();

  bool withNormals = (this->VBOs->GetNumberOfComponents("normalMC") == 3);

  // update the VBOs if needed
  if (pointMTime > this->InstanceBuffersBuildTime.GetMTime())
  {
    this->MatrixBuffer->Upload(matrices, vtkOpenGLBufferObject::ArrayBuffer);

    if (withNormals)
    {
      this->NormalMatrixBuffer->Upload(normalMatrices, vtkOpenGLBufferObject::ArrayBuffer);
    }

    this->ColorBuffer->Upload(colors, vtkOpenGLBufferObject::ArrayBuffer);
    this->InstanceBuffersBuildTime.Modified();
  }

  for (int i = PrimitiveStart; i < vtkOpenGLPolyDataMapper::PrimitiveEnd; i++)
  {
    this->DrawingVertices = i > PrimitiveTriStrips;
    if (this->Primitives[i].IBO->IndexCount)
    {
      GLenum mode = this->GetOpenGLMode(representation, i);

      // culling
      if (culling)
      {
        this->BuildCullingShaders(ren, actor, numPts, withNormals);
        if (!this->InstanceCulling->GetHelper().Program)
        {
          return;
        }

        this->InstanceCulling->RunCullingShaders(
          numPts, this->MatrixBuffer, this->ColorBuffer, this->NormalMatrixBuffer);

        // draw each LOD

        this->UpdateShaders(this->Primitives[i], ren, actor);
        if (!this->Primitives[i].Program)
        {
          return;
        }
#ifdef GL_ES_VERSION_3_0
        if (mode == GL_POINTS)
        {
          // set point size from actor property when drawing points and not selecting points
          const float pointSize = actor->GetProperty()->GetPointSize();
          this->Primitives[i].Program->SetUniformf("pointSize", pointSize);
        }
#endif

        size_t stride = (withNormals ? 29 : 20) * sizeof(float);

        this->Primitives[i].VAO->Bind();

        for (vtkIdType j = 0; j < this->InstanceCulling->GetNumberOfLOD(); j++)
        {
          if (this->InstanceCulling->GetLOD(j).NumberOfInstances == 0)
            continue;

          // add VBO of current instance in VAO
          if (!this->Primitives[i].VAO->AddAttributeArray(this->Primitives[i].Program,
                this->InstanceCulling->GetLOD(j).PositionVBO, "vertexMC", 0, 4 * sizeof(float),
                VTK_FLOAT, 4, false))
          {
            vtkErrorMacro("Error setting 'vertexMC' in shader VAO.");
          }

          if (withNormals)
          {
            if (!this->Primitives[i].VAO->AddAttributeArray(this->Primitives[i].Program,
                  this->InstanceCulling->GetLOD(j).NormalVBO, "normalMC", 0, 3 * sizeof(float),
                  VTK_FLOAT, 3, false))
            {
              vtkErrorMacro("Error setting 'normalMC' in shader VAO.");
            }
          }

          // add instances attributes based on transform feedback buffers
          if (!this->Primitives[i].VAO->AddAttributeArrayWithDivisor(this->Primitives[i].Program,
                this->InstanceCulling->GetLODBuffer(j), "glyphColor", 16 * sizeof(float), stride,
                VTK_FLOAT, 4, false, 1, false))
          {
            vtkErrorMacro("Error setting 'diffuse color' in shader VAO.");
          }

          if (!this->Primitives[i].VAO->AddAttributeMatrixWithDivisor(this->Primitives[i].Program,
                this->InstanceCulling->GetLODBuffer(j), "GCMCMatrix", 0, stride, VTK_FLOAT, 4,
                false, 1, 4 * sizeof(float)))
          {
            vtkErrorMacro("Error setting 'GCMCMatrix' in shader VAO.");
          }

          if (withNormals)
          {
            if (!this->Primitives[i].VAO->AddAttributeMatrixWithDivisor(this->Primitives[i].Program,
                  this->InstanceCulling->GetLODBuffer(j), "glyphNormalMatrix", 20 * sizeof(float),
                  stride, VTK_FLOAT, 3, false, 1, 3 * sizeof(float)))
            {
              vtkErrorMacro("Error setting 'glyphNormalMatrix' in shader VAO.");
            }
          }

          if (this->InstanceCulling->GetLOD(j).IBO->IndexCount > 0)
          {
            this->InstanceCulling->GetLOD(j).IBO->Bind();

#ifdef GL_ES_VERSION_3_0
            glDrawElementsInstanced(mode,
              static_cast<GLsizei>(this->InstanceCulling->GetLOD(j).IBO->IndexCount),
              GL_UNSIGNED_INT, nullptr, this->InstanceCulling->GetLOD(j).NumberOfInstances);
#else
            if (GLAD_GL_ARB_draw_instanced)
            {
              glDrawElementsInstancedARB(mode,
                static_cast<GLsizei>(this->InstanceCulling->GetLOD(j).IBO->IndexCount),
                GL_UNSIGNED_INT, nullptr, this->InstanceCulling->GetLOD(j).NumberOfInstances);
            }
            else
            {
              glDrawElementsInstanced(mode,
                static_cast<GLsizei>(this->InstanceCulling->GetLOD(j).IBO->IndexCount),
                GL_UNSIGNED_INT, nullptr, this->InstanceCulling->GetLOD(j).NumberOfInstances);
            }
#endif
            this->InstanceCulling->GetLOD(j).IBO->Release();
          }
          else
          {
#ifdef GL_ES_VERSION_3_0
            glDrawArraysInstanced(
              GL_POINTS, 0, 1, this->InstanceCulling->GetLOD(j).NumberOfInstances);
#else
            if (GLAD_GL_ARB_draw_instanced)
            {
              glDrawArraysInstancedARB(
                GL_POINTS, 0, 1, this->InstanceCulling->GetLOD(j).NumberOfInstances);
            }
            else
            {
              glDrawArraysInstanced(
                GL_POINTS, 0, 1, this->InstanceCulling->GetLOD(j).NumberOfInstances);
            }
#endif
          }
        }
      }
      else
      {
        GLsizei instanceCount = static_cast<GLsizei>(numPts);
        int divisor = 1;
        this->UpdateShaders(this->Primitives[i], ren, actor);
        if (!this->Primitives[i].Program)
        {
          return;
        }
#ifdef GL_ES_VERSION_3_0
        if (mode == GL_POINTS)
        {
          // set point size from actor property when drawing points and not selecting points
          const float pointSize = actor->GetProperty()->GetPointSize();
          this->Primitives[i].Program->SetUniformf("pointSize", pointSize);
        }
        if (this->HaveWideLines(ren, actor))
        {
          divisor = static_cast<int>(this->LineWidthOffsets.size() / 2);
          instanceCount = numPts * divisor;
        }
#endif

        // do the superclass and then reset a couple values
        if ((this->InstanceBuffersBuildTime > this->InstanceBuffersLoadTime ||
              this->Primitives[i].ShaderSourceTime > this->InstanceBuffersLoadTime))
        {
          this->Primitives[i].VAO->Bind();

          this->MatrixBuffer->Bind();
          if (!this->Primitives[i].VAO->AddAttributeMatrixWithDivisor(this->Primitives[i].Program,
                this->MatrixBuffer, "GCMCMatrix", 0, 16 * sizeof(float), VTK_FLOAT, 4, false,
                divisor, 4 * sizeof(float)))
          {
            vtkErrorMacro("Error setting 'GCMCMatrix' in shader VAO.");
          }
          this->MatrixBuffer->Release();

          if (withNormals && this->Primitives[i].Program->IsAttributeUsed("glyphNormalMatrix"))
          {
            this->NormalMatrixBuffer->Bind();
            if (!this->Primitives[i].VAO->AddAttributeMatrixWithDivisor(this->Primitives[i].Program,
                  this->NormalMatrixBuffer, "glyphNormalMatrix", 0, 9 * sizeof(float), VTK_FLOAT, 3,
                  false, divisor, 3 * sizeof(float)))
            {
              vtkErrorMacro("Error setting 'glyphNormalMatrix' in shader VAO.");
            }
            this->NormalMatrixBuffer->Release();
          }

          if (this->Primitives[i].Program->IsAttributeUsed("glyphColor"))
          {
            this->ColorBuffer->Bind();
            if (!this->Primitives[i].VAO->AddAttributeArrayWithDivisor(this->Primitives[i].Program,
                  this->ColorBuffer, "glyphColor", 0, 4 * sizeof(unsigned char), VTK_UNSIGNED_CHAR,
                  4, true, divisor, false))
            {
              vtkErrorMacro("Error setting 'diffuse color' in shader VAO.");
            }
            this->ColorBuffer->Release();
          }
          this->InstanceBuffersLoadTime.Modified();
        }

        this->Primitives[i].IBO->Bind();

#ifdef GL_ES_VERSION_3_0
        glDrawElementsInstanced(mode, static_cast<GLsizei>(this->Primitives[i].IBO->IndexCount),
          GL_UNSIGNED_INT, nullptr, instanceCount);
#else
        if (GLAD_GL_ARB_draw_instanced)
        {
          glDrawElementsInstancedARB(mode,
            static_cast<GLsizei>(this->Primitives[i].IBO->IndexCount), GL_UNSIGNED_INT, nullptr,
            instanceCount);
        }
        else
        {
          glDrawElementsInstanced(mode, static_cast<GLsizei>(this->Primitives[i].IBO->IndexCount),
            GL_UNSIGNED_INT, nullptr, instanceCount);
        }
#endif

        this->Primitives[i].IBO->Release();
      }
    }
  }

  if (this->LineWidthOffsetsTexture)
  {
    this->LineWidthOffsetsTexture->Deactivate();
  }

  vtkOpenGLCheckErrorMacro("failed after Render");
  this->RenderPieceFinish(ren, actor);
}

//------------------------------------------------------------------------------
void vtkOpenGLGlyph3DHelper::BuildCullingShaders(
  vtkRenderer* ren, vtkActor* actor, vtkIdType numPts, bool withNormals)
{
  vtkOpenGLRenderWindow* renWin = vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());

  if (!this->InstanceCulling->GetHelper().Program)
  {
    this->InstanceCulling->InitLOD(this->CurrentInput);

    for (auto& lod : this->LODs)
    {
      this->InstanceCulling->AddLOD(lod.first, lod.second);
    }
  }

  this->InstanceCulling->BuildCullingShaders(renWin->GetShaderCache(), numPts, withNormals);

  if (this->InstanceCulling->GetHelper().Program)
  {
    this->SetCameraShaderParameters(this->InstanceCulling->GetHelper(), ren, actor);

    double* bounds = this->CurrentInput->GetBounds();
    float BBoxSize[4] = { static_cast<float>(bounds[1] - bounds[0]),
      static_cast<float>(bounds[3] - bounds[2]), static_cast<float>(bounds[5] - bounds[4]), 0.f };

    this->InstanceCulling->GetHelper().Program->SetUniform4f("BBoxSize", BBoxSize);
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLGlyph3DHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkOpenGLGlyph3DHelper::SetLODs(std::vector<std::pair<float, float>>& lods)
{
  this->LODs = lods;
}

//------------------------------------------------------------------------------
void vtkOpenGLGlyph3DHelper::SetLODColoring(bool val)
{
  this->InstanceCulling->SetColorLOD(val);
}
VTK_ABI_NAMESPACE_END
