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
  this->Superclass::PrintSelf(os, indent);
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
