/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLInstanceCulling.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLInstanceCulling.h"

#include "vtkDecimatePro.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLBufferObject.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLIndexBufferObject.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataNormals.h"
#include "vtkShaderProgram.h"
#include "vtkTransformFeedback.h"
#include "vtkTriangleFilter.h"

#include <algorithm>
#include <array>
#include <sstream>

vtkStandardNewMacro(vtkOpenGLInstanceCulling);

//------------------------------------------------------------------------------
vtkOpenGLInstanceCulling::~vtkOpenGLInstanceCulling()
{
  this->DeleteLODs();
  this->CullingHelper.VAO->ReleaseGraphicsResources();
}

//------------------------------------------------------------------------------
void vtkOpenGLInstanceCulling::DeleteLODs()
{
  for (auto& lod : this->LODList)
  {
    lod.IBO->Delete();
    lod.PositionVBO->Delete();
    lod.NormalVBO->Delete();
    glDeleteQueries(1, &lod.Query);
  }
  this->LODList.clear();
}

//------------------------------------------------------------------------------
void vtkOpenGLInstanceCulling::UploadCurrentState(InstanceLOD& lod, vtkPolyData* pd)
{
  float* ptr = static_cast<float*>(pd->GetPoints()->GetVoidPointer(0));

  std::vector<float> points(4 * pd->GetNumberOfPoints());
  for (vtkIdType i = 0; i < pd->GetNumberOfPoints(); i++)
  {
    points[4 * i] = ptr[3 * i];
    points[4 * i + 1] = ptr[3 * i + 1];
    points[4 * i + 2] = ptr[3 * i + 2];
    points[4 * i + 3] = 1.f;
  }

  lod.PositionVBO->Upload(points, vtkOpenGLBufferObject::ArrayBuffer);

  vtkDataArray* normalsData = pd->GetPointData()->GetNormals();
  if (normalsData)
  {
    std::vector<float> normals(3 * pd->GetNumberOfPoints());
    for (vtkIdType i = 0; i < pd->GetNumberOfPoints(); i++)
    {
      double n[3];
      normalsData->GetTuple(i, n);
      normals[3 * i] = static_cast<float>(n[0]);
      normals[3 * i + 1] = static_cast<float>(n[1]);
      normals[3 * i + 2] = static_cast<float>(n[2]);
    }
    lod.NormalVBO->Upload(normals, vtkOpenGLBufferObject::ArrayBuffer);
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLInstanceCulling::AddLOD(float distance, float reduction)
{
  if (this->PolyData.Get() == nullptr)
  {
    vtkErrorMacro("Cannot add LOD, PolyData is not set yet.");
    return;
  }

  if (distance <= 0.0)
  {
    return;
  }

  InstanceLOD lod;
  lod.Distance = distance;
  glGenQueries(1, &lod.Query);
  lod.PositionVBO = vtkOpenGLBufferObject::New();
  lod.NormalVBO = vtkOpenGLBufferObject::New();
  lod.IBO = vtkOpenGLIndexBufferObject::New();

  vtkSmartPointer<vtkPolyData> pd = this->PolyData;

  if (reduction > 0.0 && reduction < 1.0)
  {
    vtkNew<vtkTriangleFilter> triangle;
    triangle->SetInputData(this->PolyData);

    vtkNew<vtkDecimatePro> decim;
    decim->SetInputConnection(triangle->GetOutputPort());
    decim->SetTargetReduction(reduction);

    vtkNew<vtkPolyDataNormals> normals;
    normals->SetInputConnection(decim->GetOutputPort());
    normals->Update();

    pd = normals->GetOutput();
  }

  if (reduction < 1.0 && pd->GetNumberOfPoints() > 0)
  {
    this->UploadCurrentState(lod, pd);
    lod.IBO->CreateTriangleIndexBuffer(pd->GetPolys(), pd->GetPoints());
  }
  else
  {
    std::array<float, 4> point;
    std::array<float, 3> normal;
    point[0] = 0.f;
    point[1] = 0.f;
    point[2] = 0.f;
    point[3] = 1.f;
    normal[0] = 0.f;
    normal[1] = 0.f;
    normal[2] = 1.f;
    lod.PositionVBO->Upload(point, vtkOpenGLBufferObject::ArrayBuffer);
    lod.NormalVBO->Upload(normal, vtkOpenGLBufferObject::ArrayBuffer);
  }

  this->LODList.push_back(lod);
}

//------------------------------------------------------------------------------
void vtkOpenGLInstanceCulling::InitLOD(vtkPolyData* pd)
{
  this->DeleteLODs();

  this->PolyData = pd;

  InstanceLOD lod;
  lod.Distance = std::numeric_limits<float>::min();
  glGenQueries(1, &lod.Query);
  lod.PositionVBO = vtkOpenGLBufferObject::New();
  lod.NormalVBO = vtkOpenGLBufferObject::New();
  lod.IBO = vtkOpenGLIndexBufferObject::New();

  this->UploadCurrentState(lod, pd);

  lod.IBO->CreateTriangleIndexBuffer(pd->GetPolys(), pd->GetPoints());

  this->LODList.push_back(lod);
}

//------------------------------------------------------------------------------
void vtkOpenGLInstanceCulling::BuildCullingShaders(
  vtkOpenGLShaderCache* cache, vtkIdType numInstances, bool withNormals)
{
  if (!this->CullingHelper.Program)
  {
    // sort LOD by distance
    std::sort(this->LODList.begin(), this->LODList.end());

    std::map<vtkShader::Type, vtkShader*> shaders;
    vtkNew<vtkShader> vss;
    vss->SetType(vtkShader::Vertex);
    shaders[vtkShader::Vertex] = vss;
    vtkNew<vtkShader> gss;
    gss->SetType(vtkShader::Geometry);
    shaders[vtkShader::Geometry] = gss;
    vtkNew<vtkShader> fss;
    fss->SetType(vtkShader::Fragment);
    shaders[vtkShader::Fragment] = fss;

    std::stringstream vstr;
    vstr << "//VTK::System::Dec"
            "\n"
            "\nuniform mat4 MCDCMatrix;"
            "\nuniform mat4 MCVCMatrix;"
            "\nuniform vec4 BBoxSize;"
            "\n"
            "\nin mat4 InstanceMatrix;"
            "\nin vec4 InstanceColor;"
            "\nin mat3 InstanceNormal;"
            "\n"
            "\nflat out int LODLevel;"
            "\nout mat4 InstanceMatrixVSOutput;"
            "\nout vec4 InstanceColorVSOutput;"
         << (withNormals ? "\nout mat3 InstanceNormalVSOutput;" : "")
         << "\n"
            "\nvoid main() {"
            "\n  InstanceMatrixVSOutput = InstanceMatrix;"
            "\n  InstanceColorVSOutput = InstanceColor;"
         << (withNormals ? "\n  InstanceNormalVSOutput = InstanceNormal;" : "")
         << "\n  vec4 PosMC = InstanceMatrix[3].xyzw;"
            "\n  vec4 p = MCDCMatrix * PosMC;"
            "\n  if (p.x < p.w && p.x > -p.w && p.y < p.w && p.y > -p.w)"
            "\n  {"
            "\n    vec4 pc = MCVCMatrix * PosMC;"
            "\n    vec4 ScaledBBoxSize = MCVCMatrix * InstanceMatrix * BBoxSize;"
            "\n    float lenPosVC = length(pc.xyz)/length(ScaledBBoxSize);";

    for (size_t i = 1; i < this->LODList.size(); i++)
    {
      vstr << "\n    if (lenPosVC < " << this->LODList[i].Distance
           << ")"
              "\n    {"
              "\n      LODLevel = "
           << (i - 1)
           << ";"
              "\n    }"
              "\n    else";
    }
    vstr << "\n    {"
            "\n      LODLevel = "
         << (this->LODList.size() - 1)
         << ";"
            "\n    }"
            "\n  }"
            "\n  else"
            "\n  {"
            "\n    LODLevel = -1;"
            "\n  }"
            "\n  gl_Position = p;"
            "\n}";

    vss->SetSource(vstr.str());

    std::stringstream gstr;
    gstr << "//VTK::System::Dec"
            "\n#extension GL_ARB_gpu_shader5 : enable" // required for EmitStreamVertex
            "\n"
            "\nlayout(points) in;"
            "\nlayout(points, max_vertices = 1) out;"
            "\n"
            "\nflat in int LODLevel[];"
            "\nin mat4 InstanceMatrixVSOutput[];"
            "\nin vec4 InstanceColorVSOutput[];"
         << (withNormals ? "\nin mat3 InstanceNormalVSOutput[];" : "") << "\n";

    for (size_t i = 0; i < this->LODList.size(); i++)
    {
      // We cannot group the stream declaration, OSX does not like that
      gstr << "\nlayout(stream = " << i << ") out vec4 matrixR0Culled" << i
           << ";"
              "\nlayout(stream = "
           << i << ") out vec4 matrixR1Culled" << i
           << ";"
              "\nlayout(stream = "
           << i << ") out vec4 matrixR2Culled" << i
           << ";"
              "\nlayout(stream = "
           << i << ") out vec4 matrixR3Culled" << i
           << ";"
              "\nlayout(stream = "
           << i << ") out vec4 colorCulled" << i << ";";

      if (withNormals)
      {
        gstr << "\nlayout(stream = " << i << ") out vec3 normalR0Culled" << i
             << ";"
                "\nlayout(stream = "
             << i << ") out vec3 normalR1Culled" << i
             << ";"
                "\nlayout(stream = "
             << i << ") out vec3 normalR2Culled" << i << ";";
      }
    }
    gstr << "\n"
            "\nvoid main() {";

    for (size_t i = 0; i < this->LODList.size(); i++)
    {
      gstr << "\n  if (LODLevel[0] == " << i
           << ")"
              "\n  {"
              "\n    gl_Position = gl_in[0].gl_Position;"
              "\n    matrixR0Culled"
           << i
           << " = InstanceMatrixVSOutput[0][0];"
              "\n    matrixR1Culled"
           << i
           << " = InstanceMatrixVSOutput[0][1];"
              "\n    matrixR2Culled"
           << i
           << " = InstanceMatrixVSOutput[0][2];"
              "\n    matrixR3Culled"
           << i
           << " = InstanceMatrixVSOutput[0][3];"
              "\n    colorCulled"
           << i << " = InstanceColorVSOutput[0];";

      if (withNormals)
      {
        gstr << "\n    normalR0Culled" << i
             << " = InstanceNormalVSOutput[0][0];"
                "\n    normalR1Culled"
             << i
             << " = InstanceNormalVSOutput[0][1];"
                "\n    normalR2Culled"
             << i << " = InstanceNormalVSOutput[0][2];";
      }

      if (this->ColorLOD)
      {
        int R = (i + 1) & 1;
        int G = ((i + 1) & 2) >> 1;
        int B = ((i + 1) & 4) >> 2;
        gstr << "\n    colorCulled" << i << " = vec4(" << R << "," << G << "," << B
             << ",InstanceColorVSOutput[0].a);";
      }

      gstr << "\n    EmitStreamVertex(" << i
           << ");"
              "\n  }";
    }

    gstr << "\n}";
    gss->SetSource(gstr.str());

    // dummy FS
    fss->SetSource("//VTK::System::Dec"
                   "\nvoid main() {"
                   "\n  discard;"
                   "\n}");

    vtkNew<vtkTransformFeedback> tf;
    for (size_t i = 0; i < this->LODList.size(); i++)
    {
      if (i != 0)
      {
        tf->AddVarying(vtkTransformFeedback::Next_Buffer, "gl_NextBuffer");
      }

      std::stringstream ss;
      ss << i;

      tf->AddVarying(vtkTransformFeedback::Color_RGBA_F, "matrixR0Culled" + ss.str());
      tf->AddVarying(vtkTransformFeedback::Color_RGBA_F, "matrixR1Culled" + ss.str());
      tf->AddVarying(vtkTransformFeedback::Color_RGBA_F, "matrixR2Culled" + ss.str());
      tf->AddVarying(vtkTransformFeedback::Color_RGBA_F, "matrixR3Culled" + ss.str());
      tf->AddVarying(vtkTransformFeedback::Color_RGBA_F, "colorCulled" + ss.str());

      if (withNormals)
      {
        tf->AddVarying(vtkTransformFeedback::Normal_F, "normalR0Culled" + ss.str());
        tf->AddVarying(vtkTransformFeedback::Normal_F, "normalR1Culled" + ss.str());
        tf->AddVarying(vtkTransformFeedback::Normal_F, "normalR2Culled" + ss.str());
      }
    }

    this->CullingHelper.Program = cache->ReadyShaderProgram(shaders, tf);
    tf->SetNumberOfVertices(numInstances);
    size_t instanceSize = withNormals ? 29 : 20;
    tf->Allocate(static_cast<int>(this->LODList.size()),
      instanceSize * sizeof(float) * numInstances, GL_DYNAMIC_COPY);
  }
  else
  {
    vtkShaderProgram* prog = this->CullingHelper.Program;
    cache->ReadyShaderProgram(prog, prog->GetTransformFeedback());
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLInstanceCulling::RunCullingShaders(vtkIdType numInstances,
  vtkOpenGLBufferObject* matrixBuffer, vtkOpenGLBufferObject* colorBuffer,
  vtkOpenGLBufferObject* normalBuffer)
{
  // update VAO with buffers
  this->CullingHelper.VAO->Bind();

  if (!this->CullingHelper.VAO->AddAttributeMatrixWithDivisor(this->CullingHelper.Program,
        matrixBuffer, "InstanceMatrix", 0, 16 * sizeof(float), VTK_FLOAT, 4, false, 0,
        4 * sizeof(float)))
  {
    vtkErrorMacro("Error setting 'InstanceMatrix' in culling shader VAO.");
  }

  if (!this->CullingHelper.VAO->AddAttributeArray(this->CullingHelper.Program, colorBuffer,
        "InstanceColor", 0, 4 * sizeof(unsigned char), VTK_UNSIGNED_CHAR, 4, true))
  {
    vtkErrorMacro("Error setting 'InstanceColor' in culling shader VAO.");
  }

  if (normalBuffer->GetHandle() != 0)
  {
    if (!this->CullingHelper.VAO->AddAttributeMatrixWithDivisor(this->CullingHelper.Program,
          normalBuffer, "InstanceNormal", 0, 9 * sizeof(float), VTK_FLOAT, 3, false, 0,
          3 * sizeof(float)))
    {
      vtkErrorMacro("Error setting 'InstanceNormal' in culling shader VAO.");
    }
  }

  // draw instances points
#ifndef GL_ES_VERSION_3_0
  for (size_t j = 0; j < this->LODList.size(); j++)
  {
    glBeginQueryIndexed(
      GL_PRIMITIVES_GENERATED, static_cast<GLuint>(j), static_cast<GLuint>(this->LODList[j].Query));
  }
#endif

  this->CullingHelper.Program->GetTransformFeedback()->BindBuffer(false);

  glDrawArrays(GL_POINTS, 0, numInstances);

  this->CullingHelper.Program->GetTransformFeedback()->ReadBuffer(-1);

#ifndef GL_ES_VERSION_3_0
  for (size_t j = 0; j < this->LODList.size(); j++)
  {
    glEndQueryIndexed(GL_PRIMITIVES_GENERATED, static_cast<GLuint>(j));
    glGetQueryObjectiv(
      this->LODList[j].Query, GL_QUERY_RESULT, &this->LODList[j].NumberOfInstances);
  }
#endif
}

//------------------------------------------------------------------------------
vtkOpenGLHelper& vtkOpenGLInstanceCulling::GetHelper()
{
  return this->CullingHelper;
}

//------------------------------------------------------------------------------
vtkOpenGLInstanceCulling::InstanceLOD& vtkOpenGLInstanceCulling::GetLOD(vtkIdType index)
{
  return this->LODList[index];
}

//------------------------------------------------------------------------------
vtkOpenGLBufferObject* vtkOpenGLInstanceCulling::GetLODBuffer(vtkIdType index)
{
  return this->CullingHelper.Program->GetTransformFeedback()->GetBuffer(index);
}

//------------------------------------------------------------------------------
vtkIdType vtkOpenGLInstanceCulling::GetNumberOfLOD()
{
  return static_cast<vtkIdType>(this->LODList.size());
}
