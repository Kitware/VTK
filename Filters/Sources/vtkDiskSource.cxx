/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDiskSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDiskSource.h"

#include "vtkCellArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkTransform.h"

vtkStandardNewMacro(vtkDiskSource);

//------------------------------------------------------------------------------
vtkDiskSource::vtkDiskSource()
{
  this->InnerRadius = 0.25;
  this->OuterRadius = 0.5;
  this->Center[0] = 0.0;
  this->Center[1] = 0.0;
  this->Center[2] = 0.0;
  this->Normal[0] = 0.0;
  this->Normal[1] = 0.0;
  this->Normal[2] = 1.0;
  this->RadialResolution = 1;
  this->CircumferentialResolution = 6;
  this->OutputPointsPrecision = SINGLE_PRECISION;

  this->SetNumberOfInputPorts(0);
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkTransform> vtkDiskSource::GetTransformation()
{
  double n[3] = { this->Normal[0], this->Normal[1], this->Normal[2] };
  // normalize normal vector
  if (vtkMath::Normalize(n) == 0.0)
  {
    vtkErrorMacro(<< "Specified zero normal");
    return nullptr;
  }

  double rotationVector[3];
  double defaultNormal[3] = { 0.0, 0.0, 1.0 };

  // calculate angle and rotation vector
  double dp = vtkMath::Dot(defaultNormal, n);
  vtkMath::Cross(defaultNormal, n, rotationVector);
  double angle = vtkMath::DegreesFromRadians(std::acos(dp));

  // set up transformation
  auto transform = vtkSmartPointer<vtkTransform>::New();
  transform->PostMultiply();
  transform->Translate(-this->Center[0], -this->Center[1], -this->Center[2]);
  transform->RotateWXYZ(angle, rotationVector[0], rotationVector[1], rotationVector[2]);
  transform->Translate(this->Center[0], this->Center[1], this->Center[2]);
  transform->Update();

  return transform;
}

//------------------------------------------------------------------------------
int vtkDiskSource::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  // get the info object
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the output
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType numPolys, numPts;
  double x[3];
  int i, j;
  vtkIdType pts[4];
  double theta, deltaRadius;
  double cosTheta, sinTheta;
  vtkNew<vtkPoints> newPoints;
  vtkNew<vtkCellArray> newPolys;

  // Set things up; allocate memory
  numPts = (this->RadialResolution + 1) * (this->CircumferentialResolution + 1);
  numPolys = this->RadialResolution * this->CircumferentialResolution;

  // Set the desired precision for the points in the output.
  if (this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
  {
    newPoints->SetDataType(VTK_DOUBLE);
  }
  else
  {
    newPoints->SetDataType(VTK_FLOAT);
  }

  newPoints->Allocate(numPts);
  newPolys->AllocateEstimate(numPolys, 4);

  auto transform = this->GetTransformation();
  // check if normal is zero
  if (transform == nullptr)
  {
    return 1;
  }
  // Create disk
  theta = 2.0 * vtkMath::Pi() / this->CircumferentialResolution;
  deltaRadius = (this->OuterRadius - this->InnerRadius) / this->RadialResolution;

  for (i = 0; i < this->CircumferentialResolution; i++)
  {
    cosTheta = std::cos(i * theta);
    sinTheta = std::sin(i * theta);
    for (j = 0; j <= this->RadialResolution; j++)
    {
      x[0] = this->Center[0] + (this->InnerRadius + j * deltaRadius) * cosTheta;
      x[1] = this->Center[1] + (this->InnerRadius + j * deltaRadius) * sinTheta;
      x[2] = this->Center[2];

      transform->TransformPoint(x, x);
      newPoints->InsertNextPoint(x);
    }
  }

  //  Create connectivity
  for (i = 0; i < this->CircumferentialResolution; i++)
  {
    for (j = 0; j < this->RadialResolution; j++)
    {
      pts[0] = i * (this->RadialResolution + 1) + j;
      pts[1] = pts[0] + 1;
      if (i < (this->CircumferentialResolution - 1))
      {
        pts[2] = pts[1] + this->RadialResolution + 1;
      }
      else
      {
        pts[2] = j + 1;
      }
      pts[3] = pts[2] - 1;
      newPolys->InsertNextCell(4, pts);
    }
  }

  // Set points and polys
  output->SetPoints(newPoints);
  output->SetPolys(newPolys);

  return 1;
}

//------------------------------------------------------------------------------
void vtkDiskSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "InnerRadius: " << this->InnerRadius << "\n";
  os << indent << "OuterRadius: " << this->OuterRadius << "\n";
  os << indent << "RadialResolution: " << this->RadialResolution << "\n";
  os << indent << "CircumferentialResolution: " << this->CircumferentialResolution << "\n";
  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
}
