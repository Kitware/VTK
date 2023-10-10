// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkGLSLModCamera.h"
#include "vtkActor.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLActor.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLRenderer.h"
#include "vtkShaderProgram.h"
#include <sstream>

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkGLSLModCamera);

//------------------------------------------------------------------------------
vtkGLSLModCamera::vtkGLSLModCamera() = default;

//------------------------------------------------------------------------------
vtkGLSLModCamera::~vtkGLSLModCamera() = default;

//------------------------------------------------------------------------------
void vtkGLSLModCamera::PrintSelf(ostream& os, vtkIndent indent)
{
  os << "TempMatrix3: \n";
  this->TempMatrix3->PrintSelf(os, indent.GetNextIndent());
  os << "TempMatrix4: \n";
  this->TempMatrix4->PrintSelf(os, indent.GetNextIndent());
  os << " SSMatrix: \n";
  this->SSMatrix->PrintSelf(os, indent);
  os << "CoordinateShiftAndScaleInUse: " << this->CoordinateShiftAndScaleInUse << std::endl;
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkGLSLModCamera::EnableShiftScale(bool coordShiftAndScaleInUse, vtkMatrix4x4* ssMatrix)
{
  this->CoordinateShiftAndScaleInUse = coordShiftAndScaleInUse;
  this->SSMatrix = ssMatrix;
}
//------------------------------------------------------------------------------
void vtkGLSLModCamera::DisableShiftScale()
{
  this->CoordinateShiftAndScaleInUse = false;
  this->SSMatrix = nullptr;
}

//------------------------------------------------------------------------------
bool vtkGLSLModCamera::ReplaceShaderValues(vtkOpenGLRenderer*, std::string& vertexShader,
  std::string&, std::string& fragmentShader, vtkAbstractMapper*, vtkActor*)
{
  std::ostringstream oss;
  oss << "uniform mat4 MCDCMatrix;\n";
  oss << "uniform mat4 MCVCMatrix;\n";
  oss << "uniform mat3 normalMatrix;\n";
  oss << "uniform highp int cameraParallel;\n";
  vtkShaderProgram::Substitute(vertexShader, "//VTK::Camera::Dec", oss.str());
  vtkShaderProgram::Substitute(fragmentShader, "//VTK::Camera::Dec", oss.str());
  return true;
}

//------------------------------------------------------------------------------
bool vtkGLSLModCamera::SetShaderParameters(vtkOpenGLRenderer* renderer, vtkShaderProgram* program,
  vtkAbstractMapper* vtkNotUsed(mapper), vtkActor* actor,
  vtkOpenGLVertexArrayObject* vtkNotUsed(VAO) /*=nullptr*/)
{
  vtkOpenGLCamera* cam = (vtkOpenGLCamera*)(renderer->GetActiveCamera());
  // [WMVDP]C == {world, model, view, display, physical} coordinates
  // E.g., WCDC == world to display coordinate transformation
  vtkMatrix4x4* wcdc;
  vtkMatrix4x4* wcvc;
  vtkMatrix3x3* norms;
  vtkMatrix4x4* vcdc;
  cam->GetKeyMatrices(renderer, wcvc, norms, vcdc, wcdc);

  // FIXME: Add replacements in ReplaceShaderValues?
  // if (program->IsUniformUsed("ZCalcR"))
  // {
  //   if (cam->GetParallelProjection())
  //   {
  //     program->SetUniformf("ZCalcS", vcdc->GetElement(2, 2));
  //   }
  //   else
  //   {
  //     program->SetUniformf("ZCalcS", -0.5 * vcdc->GetElement(2, 2) + 0.5);
  //   }
  //   if (this->DrawingSpheres(cellBO, actor))
  //   {
  //     program->SetUniformf("ZCalcR",
  //       actor->GetProperty()->GetPointSize() / (ren->GetSize()[0] * vcdc->GetElement(0, 0)));
  //   }
  //   else
  //   {
  //     program->SetUniformf("ZCalcR",
  //       actor->GetProperty()->GetLineWidth() / (ren->GetSize()[0] * vcdc->GetElement(0, 0)));
  //   }
  // }

  // // handle coincident
  // float factor = 0.0;
  // float offset = 0.0;
  // this->GetCoincidentParameters(ren, actor, factor, offset);
  // if ((factor != 0.0 || offset != 0.0) && cellBO.Program->IsUniformUsed("cOffset") &&
  //   cellBO.Program->IsUniformUsed("cFactor"))
  // {
  //   cellBO.Program->SetUniformf("cOffset", offset);
  //   cellBO.Program->SetUniformf("cFactor", factor);
  // }

  vtkNew<vtkMatrix3x3> env;
  if (program->IsUniformUsed("envMatrix"))
  {
    double up[3];
    double right[3];
    double front[3];
    renderer->GetEnvironmentUp(up);
    renderer->GetEnvironmentRight(right);
    vtkMath::Cross(right, up, front);
    for (int i = 0; i < 3; i++)
    {
      env->SetElement(i, 0, right[i]);
      env->SetElement(i, 1, up[i]);
      env->SetElement(i, 2, front[i]);
    }
    vtkMatrix3x3::Invert(norms, this->TempMatrix3);
    vtkMatrix3x3::Multiply3x3(this->TempMatrix3, env, this->TempMatrix3);
    program->SetUniformMatrix("envMatrix", this->TempMatrix3);
  }

  if (this->CoordinateShiftAndScaleInUse && this->SSMatrix)
  {
    if (!actor->GetIsIdentity())
    {
      vtkMatrix4x4* mcwc;
      vtkMatrix3x3* anorms;
      static_cast<vtkOpenGLActor*>(actor)->GetKeyMatrices(mcwc, anorms);
      vtkMatrix4x4::Multiply4x4(this->SSMatrix, mcwc, this->TempMatrix4);
      if (program->IsUniformUsed("MCWCMatrix"))
      {
        program->SetUniformMatrix("MCWCMatrix", this->TempMatrix4);
      }
      if (program->IsUniformUsed("MCWCNormalMatrix"))
      {
        program->SetUniformMatrix("MCWCNormalMatrix", anorms);
      }
      vtkMatrix4x4::Multiply4x4(this->TempMatrix4, wcdc, this->TempMatrix4);
      program->SetUniformMatrix("MCDCMatrix", this->TempMatrix4);
      if (program->IsUniformUsed("MCVCMatrix"))
      {
        vtkMatrix4x4::Multiply4x4(this->SSMatrix, mcwc, this->TempMatrix4);
        vtkMatrix4x4::Multiply4x4(this->TempMatrix4, wcvc, this->TempMatrix4);
        program->SetUniformMatrix("MCVCMatrix", this->TempMatrix4);
      }
      if (program->IsUniformUsed("normalMatrix"))
      {
        vtkMatrix3x3::Multiply3x3(anorms, norms, this->TempMatrix3);
        program->SetUniformMatrix("normalMatrix", this->TempMatrix3);
      }
    }
    else
    {
      vtkMatrix4x4::Multiply4x4(this->SSMatrix, wcdc, this->TempMatrix4);
      program->SetUniformMatrix("MCDCMatrix", this->TempMatrix4);
      if (program->IsUniformUsed("MCVCMatrix"))
      {
        vtkMatrix4x4::Multiply4x4(this->SSMatrix, wcvc, this->TempMatrix4);
        program->SetUniformMatrix("MCVCMatrix", this->TempMatrix4);
      }
      if (program->IsUniformUsed("normalMatrix"))
      {
        program->SetUniformMatrix("normalMatrix", norms);
      }
    }
  }
  else
  {
    if (!actor->GetIsIdentity())
    {
      vtkMatrix4x4* mcwc;
      vtkMatrix3x3* anorms;
      ((vtkOpenGLActor*)actor)->GetKeyMatrices(mcwc, anorms);
      if (program->IsUniformUsed("MCWCMatrix"))
      {
        program->SetUniformMatrix("MCWCMatrix", mcwc);
      }
      if (program->IsUniformUsed("MCWCNormalMatrix"))
      {
        program->SetUniformMatrix("MCWCNormalMatrix", anorms);
      }
      vtkMatrix4x4::Multiply4x4(mcwc, wcdc, this->TempMatrix4);
      program->SetUniformMatrix("MCDCMatrix", this->TempMatrix4);
      if (program->IsUniformUsed("MCVCMatrix"))
      {
        vtkMatrix4x4::Multiply4x4(mcwc, wcvc, this->TempMatrix4);
        program->SetUniformMatrix("MCVCMatrix", this->TempMatrix4);
      }
      if (program->IsUniformUsed("normalMatrix"))
      {
        vtkMatrix3x3::Multiply3x3(anorms, norms, this->TempMatrix3);
        program->SetUniformMatrix("normalMatrix", this->TempMatrix3);
      }
    }
    else
    {
      program->SetUniformMatrix("MCDCMatrix", wcdc);
      if (program->IsUniformUsed("MCVCMatrix"))
      {
        program->SetUniformMatrix("MCVCMatrix", wcvc);
      }
      if (program->IsUniformUsed("normalMatrix"))
      {
        program->SetUniformMatrix("normalMatrix", norms);
      }
    }
  }
  if (program->IsUniformUsed("cameraParallel"))
  {
    program->SetUniformi("cameraParallel", cam->GetParallelProjection());
  }
  return true;
}

VTK_ABI_NAMESPACE_END
