// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkBatchedSurfaceLICMapper.h"
#include "vtkCompositeSurfaceLICMapper.h"
#include "vtkDataObject.h"
#include "vtkDataSetAttributes.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLVertexBufferObjectGroup.h"
#include "vtkPolyData.h"
#include "vtkShaderProgram.h"
#include "vtkSurfaceLICInterface.h"

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
vtkObjectFactoryNewMacro(vtkBatchedSurfaceLICMapper);

//------------------------------------------------------------------------------
vtkBatchedSurfaceLICMapper::vtkBatchedSurfaceLICMapper()
{
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS, vtkDataSetAttributes::VECTORS);
}

//------------------------------------------------------------------------------
vtkBatchedSurfaceLICMapper::~vtkBatchedSurfaceLICMapper() = default;

//------------------------------------------------------------------------------
void vtkBatchedSurfaceLICMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkBatchedSurfaceLICMapper::ReplaceShaderValues(
  std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* actor)
{
  std::string VSSource = shaders[vtkShader::Vertex]->GetSource();
  std::string FSSource = shaders[vtkShader::Fragment]->GetSource();

  // add some code to handle the LIC vectors and mask
  vtkShaderProgram::Substitute(VSSource, "//VTK::TCoord::Dec",
    "in vec3 vecsMC;\n"
    "out vec3 tcoordVCVSOutput;\n");

  vtkShaderProgram::Substitute(VSSource, "//VTK::TCoord::Impl", "tcoordVCVSOutput = vecsMC;");

  vtkShaderProgram::Substitute(FSSource, "//VTK::TCoord::Dec",
    // 0/1, when 1 V is projected to surface for |V| computation.
    "uniform int uMaskOnSurface;\n"
    "in vec3 tcoordVCVSOutput;\n"
    "//VTK::TCoord::Dec");

  // We need to create the uniform normalMatrix here as it will not be done in the superclass
  // if the data does not contains normals or if drawing spheres / tubes is enabled.
  if (this->VBOs->GetNumberOfComponents("normalMC") != 3 ||
    this->DrawingSpheres(*this->LastBoundBO, actor) ||
    this->DrawingTubes(*this->LastBoundBO, actor))
  {
    vtkShaderProgram::Substitute(FSSource, "//VTK::Normal::Dec",
      "//VTK::Normal::Dec\n"
      "uniform mat3 normalMatrix;");
  }

  if (this->PrimitiveInfo[this->LastBoundBO].LastLightComplexity > 0)
  {
    vtkShaderProgram::Substitute(FSSource, "//VTK::TCoord::Impl",
      // projected vectors
      "  vec3 tcoordLIC = normalMatrix * tcoordVCVSOutput;\n"
      "  vec3 normN = normalize(normalVCVSOutput);\n"
      "  float k = dot(tcoordLIC, normN);\n"
      "  tcoordLIC = (tcoordLIC - k*normN);\n"
      "  gl_FragData[1] = vec4(tcoordLIC.x, tcoordLIC.y, 0.0 , gl_FragCoord.z);\n"
      //   "  gl_FragData[1] = vec4(tcoordVC.xyz, gl_FragCoord.z);\n"
      // vectors for fragment masking
      "  if (uMaskOnSurface == 0)\n"
      "    {\n"
      "    gl_FragData[2] = vec4(tcoordVCVSOutput, gl_FragCoord.z);\n"
      "    }\n"
      "  else\n"
      "    {\n"
      "    gl_FragData[2] = vec4(tcoordLIC.x, tcoordLIC.y, 0.0 , gl_FragCoord.z);\n"
      "    }\n"
      //   "  gl_FragData[2] = vec4(19.0, 19.0, tcoordVC.x, gl_FragCoord.z);\n"
      ,
      false);
  }

  shaders[vtkShader::Vertex]->SetSource(VSSource);
  shaders[vtkShader::Fragment]->SetSource(FSSource);

  this->Superclass::ReplaceShaderValues(shaders, ren, actor);
}

void vtkBatchedSurfaceLICMapper::SetMapperShaderParameters(
  vtkOpenGLHelper& cellBO, vtkRenderer* ren, vtkActor* actor)
{
  this->Superclass::SetMapperShaderParameters(cellBO, ren, actor);
  cellBO.Program->SetUniformi("uMaskOnSurface",
    static_cast<vtkCompositeSurfaceLICMapper*>(this->Parent)
      ->GetLICInterface()
      ->GetMaskOnSurface());
}

//------------------------------------------------------------------------------
void vtkBatchedSurfaceLICMapper::AppendOneBufferObject(vtkRenderer* ren, vtkActor* act,
  GLBatchElement* glBatchElement, vtkIdType& voffset, std::vector<unsigned char>& newColors,
  std::vector<float>& newNorms)
{
  vtkPolyData* poly = glBatchElement->Parent.PolyData;
  vtkDataArray* vectors = this->GetInputArrayToProcess(0, poly);
  if (vectors)
  {
    this->VBOs->AppendDataArray("vecsMC", vectors, VTK_FLOAT);
  }

  this->Superclass::AppendOneBufferObject(ren, act, glBatchElement, voffset, newColors, newNorms);
}
VTK_ABI_NAMESPACE_END
