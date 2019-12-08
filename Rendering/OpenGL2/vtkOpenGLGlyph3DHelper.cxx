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
#include "vtkOpenGLActor.h"
#include "vtkOpenGLBufferObject.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLIndexBufferObject.h"
#include "vtkOpenGLInstanceCulling.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLResourceFreeCallback.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkOpenGLVertexBufferObject.h"
#include "vtkOpenGLVertexBufferObjectGroup.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkShader.h"
#include "vtkShaderProgram.h"
#include "vtkTransform.h"
#include "vtkTransformFeedback.h"

#include "vtkGlyph3DVS.h"

#include <algorithm>
#include <numeric>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLGlyph3DHelper);

//-----------------------------------------------------------------------------
vtkOpenGLGlyph3DHelper::vtkOpenGLGlyph3DHelper()
{
  this->UsingInstancing = false;
  this->PopulateSelectionSettings = 0;
}

// ---------------------------------------------------------------------------
// Description:
// Release any graphics resources that are being consumed by this mapper.
void vtkOpenGLGlyph3DHelper::ReleaseGraphicsResources(vtkWindow* window)
{
  this->NormalMatrixBuffer->ReleaseGraphicsResources();
  this->MatrixBuffer->ReleaseGraphicsResources();
  this->ColorBuffer->ReleaseGraphicsResources();
  this->Superclass::ReleaseGraphicsResources(window);
}

//-----------------------------------------------------------------------------
void vtkOpenGLGlyph3DHelper::GetShaderTemplate(
  std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* actor)
{
  this->Superclass::GetShaderTemplate(shaders, ren, actor);

  shaders[vtkShader::Vertex]->SetSource(vtkGlyph3DVS);
}

void vtkOpenGLGlyph3DHelper::ReplaceShaderPositionVC(
  std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* actor)
{
  std::string VSSource = shaders[vtkShader::Vertex]->GetSource();

  if (this->LastLightComplexity[this->LastBoundBO] > 0)
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
  if (!this->DrawingEdgesOrVertices)
  {
    vtkShaderProgram::Substitute(FSSource, "//VTK::Color::Impl",
      "//VTK::Color::Impl\n"
      "  diffuseColor = diffuseIntensity * vertexColorVSOutput.rgb;\n"
      "  ambientColor = ambientIntensity * vertexColorVSOutput.rgb;\n"
      "  opacity = opacity * vertexColorVSOutput.a;");
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
}

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

void vtkOpenGLGlyph3DHelper::GlyphRender(vtkRenderer* ren, vtkActor* actor, vtkIdType numPts,
  std::vector<unsigned char>& colors, std::vector<float>& matrices,
  std::vector<float>& normalMatrices, std::vector<vtkIdType>& pickIds, vtkMTimeType pointMTime,
  bool culling)
{
  this->ResourceCallback->RegisterGraphicsResources(
    static_cast<vtkOpenGLRenderWindow*>(ren->GetRenderWindow()));

  this->UsingInstancing = false;

  vtkHardwareSelector* selector = ren->GetSelector();

  if (!selector && GLEW_ARB_instanced_arrays)
  {
    // if there is no triangle, culling is useless.
    // GLEW_ARB_gpu_shader5 is needed by the culling shader.
#ifndef GL_ES_VERSION_3_0
    if (this->CurrentInput->GetNumberOfPolys() <= 0 || !GLEW_ARB_gpu_shader5 ||
      !GLEW_ARB_transform_feedback3)
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

  if (selecting_points)
  {
#ifndef GL_ES_VERSION_3_0
    glPointSize(6.0);
#endif
    representation = GL_POINTS;
  }

  bool draw_surface_with_edges =
    (actor->GetProperty()->GetEdgeVisibility() && representation == VTK_SURFACE) && !selector;
  int numVerts = this->VBOs->GetNumberOfTuples("vertexMC");
  for (int i = PrimitiveStart;
       i < (draw_surface_with_edges ? PrimitiveEnd : PrimitiveTriStrips + 1); i++)
  {
    this->DrawingEdgesOrVertices = (i > PrimitiveTriStrips ? true : false);
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
        if (this->LastLightComplexity[this->LastBoundBO] > 0 &&
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

        glDrawRangeElements(mode, 0, static_cast<GLuint>(numVerts - 1),
          static_cast<GLsizei>(this->Primitives[i].IBO->IndexCount), GL_UNSIGNED_INT, nullptr);
      }
      this->Primitives[i].IBO->Release();
    }
  }
  this->RenderPieceFinish(ren, actor);
}

//-----------------------------------------------------------------------------
void vtkOpenGLGlyph3DHelper::SetMapperShaderParameters(
  vtkOpenGLHelper& cellBO, vtkRenderer* ren, vtkActor* actor)
{
  this->Superclass::SetMapperShaderParameters(cellBO, ren, actor);

  vtkHardwareSelector* selector = ren->GetSelector();
  if (selector)
  {
    cellBO.Program->SetUniform3f("mapperIndex", selector->GetPropColorValue());
  }
}

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

  bool draw_surface_with_edges =
    (actor->GetProperty()->GetEdgeVisibility() && representation == VTK_SURFACE);
  for (int i = PrimitiveStart;
       i < (draw_surface_with_edges ? PrimitiveEnd : PrimitiveTriStrips + 1); i++)
  {
    this->DrawingEdgesOrVertices = (i > PrimitiveTriStrips ? true : false);
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
            glDrawElementsInstancedARB(mode,
              static_cast<GLsizei>(this->InstanceCulling->GetLOD(j).IBO->IndexCount),
              GL_UNSIGNED_INT, nullptr, this->InstanceCulling->GetLOD(j).NumberOfInstances);
#endif
            this->InstanceCulling->GetLOD(j).IBO->Release();
          }
          else
          {
#ifdef GL_ES_VERSION_3_0
            glDrawArraysInstanced(
              GL_POINTS, 0, 1, this->InstanceCulling->GetLOD(j).NumberOfInstances);
#else
            glDrawArraysInstancedARB(
              GL_POINTS, 0, 1, this->InstanceCulling->GetLOD(j).NumberOfInstances);
#endif
          }
        }
      }
      else
      {
        this->UpdateShaders(this->Primitives[i], ren, actor);
        if (!this->Primitives[i].Program)
        {
          return;
        }

        // do the superclass and then reset a couple values
        if ((this->InstanceBuffersBuildTime > this->InstanceBuffersLoadTime ||
              this->Primitives[i].ShaderSourceTime > this->InstanceBuffersLoadTime))
        {
          this->Primitives[i].VAO->Bind();

          this->MatrixBuffer->Bind();
          if (!this->Primitives[i].VAO->AddAttributeMatrixWithDivisor(this->Primitives[i].Program,
                this->MatrixBuffer, "GCMCMatrix", 0, 16 * sizeof(float), VTK_FLOAT, 4, false, 1,
                4 * sizeof(float)))
          {
            vtkErrorMacro("Error setting 'GCMCMatrix' in shader VAO.");
          }
          this->MatrixBuffer->Release();

          if (withNormals && this->Primitives[i].Program->IsAttributeUsed("glyphNormalMatrix"))
          {
            this->NormalMatrixBuffer->Bind();
            if (!this->Primitives[i].VAO->AddAttributeMatrixWithDivisor(this->Primitives[i].Program,
                  this->NormalMatrixBuffer, "glyphNormalMatrix", 0, 9 * sizeof(float), VTK_FLOAT, 3,
                  false, 1, 3 * sizeof(float)))
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
                  4, true, 1, false))
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
          GL_UNSIGNED_INT, nullptr, numPts);
#else
        glDrawElementsInstancedARB(mode, static_cast<GLsizei>(this->Primitives[i].IBO->IndexCount),
          GL_UNSIGNED_INT, nullptr, numPts);
#endif

        this->Primitives[i].IBO->Release();
      }
    }
  }

  vtkOpenGLCheckErrorMacro("failed after Render");
  this->RenderPieceFinish(ren, actor);
}

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
void vtkOpenGLGlyph3DHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void vtkOpenGLGlyph3DHelper::SetLODs(std::vector<std::pair<float, float> >& lods)
{
  this->LODs = lods;
}

//-----------------------------------------------------------------------------
void vtkOpenGLGlyph3DHelper::SetLODColoring(bool val)
{
  this->InstanceCulling->SetColorLOD(val);
}
