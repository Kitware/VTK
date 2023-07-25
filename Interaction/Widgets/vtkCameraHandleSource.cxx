// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCameraHandleSource.h"

#include "vtkAppendPolyData.h"
#include "vtkArrowSource.h"
#include "vtkCamera.h"
#include "vtkConeSource.h"
#include "vtkSphereSource.h"
#include "vtkTransform.h"
#include "vtkTransformFilter.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkCameraHandleSource);

//------------------------------------------------------------------------------
vtkCameraHandleSource::vtkCameraHandleSource()
{
  this->UpArrow->SetShaftRadius(0.1);
  this->FrontArrow->SetShaftRadius(0.1);

  this->UpArrow->SetTipRadius(0.2);
  this->FrontArrow->SetTipRadius(0.2);

  this->UpTransform->PostMultiply();
  this->FrontTransform->PostMultiply();

  this->UpTransformFilter->SetTransform(UpTransform);
  this->UpTransformFilter->SetInputConnection(UpArrow->GetOutputPort());

  this->FrontTransformFilter->SetTransform(FrontTransform);
  this->FrontTransformFilter->SetInputConnection(FrontArrow->GetOutputPort());

  this->ArrowsAppend->AddInputConnection(this->UpTransformFilter->GetOutputPort());
  this->ArrowsAppend->AddInputConnection(this->FrontTransformFilter->GetOutputPort());
}

//------------------------------------------------------------------------------
vtkCameraHandleSource::~vtkCameraHandleSource() = default;

//------------------------------------------------------------------------------
void vtkCameraHandleSource::SetCamera(vtkCamera* cam)
{
  if (!cam || this->Camera == cam)
  {
    return;
  }

  this->Camera = cam;
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkCameraHandleSource::SetPosition(double xPos, double yPos, double zPos)
{
  if ((this->GetPosition()[0] != xPos) || (this->GetPosition()[1] != yPos) ||
    (this->GetPosition()[2] != zPos))
  {
    this->Camera->SetPosition(xPos, yPos, zPos);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
double* vtkCameraHandleSource::GetPosition()
{
  return this->Camera->GetPosition();
}

//------------------------------------------------------------------------------
void vtkCameraHandleSource::SetDirection(double xTarget, double yTarget, double zTarget)
{
  if ((this->GetDirection()[0] != xTarget) || (this->GetDirection()[1] != yTarget) ||
    (this->GetDirection()[2] != zTarget))
  {
    this->Camera->SetFocalPoint(xTarget, yTarget, zTarget);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
double* vtkCameraHandleSource::GetDirection()
{
  return this->Camera->GetFocalPoint();
}

//------------------------------------------------------------------------------
int vtkCameraHandleSource::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  auto output = vtkPolyData::GetData(outputVector);
  // check type of representation
  if (!this->Directional)
  {
    this->RecomputeSphere();
    output->ShallowCopy(this->PositionSphere->GetOutput());
  }
  else
  {
    this->RecomputeArrows();
    output->ShallowCopy(this->ArrowsAppend->GetOutput());
  }
  return 1;
}

//------------------------------------------------------------------------------
void vtkCameraHandleSource::RecomputeArrows()
{
  double arrowOrigin[3];
  this->GetPosition(arrowOrigin);
  double rotationAxis[3];
  double rotationAngle;
  double baseVector[3] = { 1, 0, 0 };

  this->UpTransform->Identity();
  this->FrontTransform->Identity();

  vtkMatrix4x4* matrix = this->Camera->GetModelViewTransformMatrix();
  double ViewUp[3];

  ViewUp[0] = matrix->GetElement(1, 0);
  ViewUp[1] = matrix->GetElement(1, 1);
  ViewUp[2] = matrix->GetElement(1, 2);

  vtkMath::Normalize(ViewUp);
  rotationAngle = vtkMath::AngleBetweenVectors(baseVector, ViewUp);
  vtkMath::Cross(baseVector, ViewUp, rotationAxis);
  vtkMath::Normalize(rotationAxis);

  this->UpTransform->Scale(this->Size * 2, this->Size * 3, this->Size * 3);
  this->UpTransform->RotateWXYZ(vtkMath::DegreesFromRadians(rotationAngle), rotationAxis);
  this->UpTransform->Translate(arrowOrigin);

  double* ViewFront = this->Camera->GetDirectionOfProjection();
  vtkMath::Normalize(ViewFront);
  rotationAngle = vtkMath::AngleBetweenVectors(baseVector, ViewFront);
  vtkMath::Cross(baseVector, ViewFront, rotationAxis);
  vtkMath::Normalize(rotationAxis);

  this->FrontTransform->Scale(this->Size * 4, this->Size * 4, this->Size * 4);
  this->FrontTransform->RotateWXYZ(vtkMath::DegreesFromRadians(rotationAngle), rotationAxis);
  this->FrontTransform->Translate(arrowOrigin);

  this->ArrowsAppend->Update();
}

//------------------------------------------------------------------------------
void vtkCameraHandleSource::RecomputeSphere()
{
  this->PositionSphere->SetRadius(this->Size / 2);
  this->PositionSphere->SetCenter(this->GetPosition());
  this->PositionSphere->SetThetaResolution(16);
  this->PositionSphere->SetPhiResolution(8);
  this->PositionSphere->Update();
}

//------------------------------------------------------------------------------
void vtkCameraHandleSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->Directional)
  {
    os << indent << "UpArrow: (" << this->UpArrow << "\n";
    if (this->UpArrow)
    {
      this->UpArrow->PrintSelf(os, indent.GetNextIndent());
      os << indent << ")\n";
    }
    else
    {
      os << "none)\n";
    }

    os << indent << "UpTransform: (" << this->UpTransform << "\n";
    if (this->UpTransform)
    {
      this->UpTransform->PrintSelf(os, indent.GetNextIndent());
      os << indent << ")\n";
    }
    else
    {
      os << "none)\n";
    }

    os << indent << "FrontArrow: (" << this->FrontArrow << "\n";
    if (this->FrontArrow)
    {
      this->FrontArrow->PrintSelf(os, indent.GetNextIndent());
      os << indent << ")\n";
    }
    else
    {
      os << "none)\n";
    }

    os << indent << "FrontTransform: (" << this->FrontTransform << "\n";
    if (this->FrontTransform)
    {
      this->FrontTransform->PrintSelf(os, indent.GetNextIndent());
      os << indent << ")\n";
    }
    else
    {
      os << "none)\n";
    }
  }
  else
  {
    os << indent << "PositionSphere: (" << this->PositionSphere << "\n";
    if (this->PositionSphere)
    {
      this->PositionSphere->PrintSelf(os, indent.GetNextIndent());
      os << indent << ")\n";
    }
    else
    {
      os << "none)\n";
    }
  }
}
VTK_ABI_NAMESPACE_END
