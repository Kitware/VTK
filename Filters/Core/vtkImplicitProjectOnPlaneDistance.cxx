/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitProjectOnPlaneDistance.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImplicitProjectOnPlaneDistance.h"

#include "vtkCellData.h"
#include "vtkGenericCell.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkPolyData.h"
#include "vtkSetGet.h"
#include "vtkSmartPointer.h"
#include "vtkStaticCellLocator.h"
#include "vtkTriangle.h"

vtkStandardNewMacro(vtkImplicitProjectOnPlaneDistance);

//-----------------------------------------------------------------------------
vtkImplicitProjectOnPlaneDistance::vtkImplicitProjectOnPlaneDistance()
  : Tolerance(0.01)
  , Norm(NormType::L2)
  , Input(nullptr)
  , Locator(nullptr)
  , ProjectionPlane(nullptr)
  , UnusedCell(vtkSmartPointer<vtkGenericCell>::New())
{
}

//-----------------------------------------------------------------------------
void vtkImplicitProjectOnPlaneDistance::SetInput(vtkPolyData* input)
{
  if (this->Input != input)
  {
    // If we wanted to check that the user input is really planar,
    // we would do it here.

    if (input->GetNumberOfPoints() < 3)
    {
      vtkErrorMacro("Invalid input, need at least three points to define a plane.");
      return;
    }

    this->Input = input;
    this->Input->BuildLinks();
    this->CreateDefaultLocator();
    this->Locator->SetDataSet(this->Input);
    this->Locator->SetTolerance(this->Tolerance);
    this->Locator->CacheCellBoundsOn();
    this->Locator->BuildLocator();

    // Define the projection plane using the three first vertices of the input.
    this->ProjectionPlane = vtkSmartPointer<vtkPlane>::New();
    double i1[3], i2[3], i3[3], n[3];
    this->Input->GetPoint(0, i1);
    this->Input->GetPoint(1, i2);
    this->Input->GetPoint(2, i3);
    this->ProjectionPlane->SetOrigin(i1);
    vtkTriangle::ComputeNormal(i1, i2, i3, n);
    this->ProjectionPlane->SetNormal(n);

    // Store the bounds to reduce L0 computation
    this->Input->GetBounds(this->Bounds);
  }
}

//-----------------------------------------------------------------------------
vtkMTimeType vtkImplicitProjectOnPlaneDistance::GetMTime()
{
  vtkMTimeType mTime = this->vtkImplicitFunction::GetMTime();
  vtkMTimeType inputMTime;

  if (this->Input != nullptr)
  {
    inputMTime = this->Input->GetMTime();
    mTime = (inputMTime > mTime ? inputMTime : mTime);
  }

  return mTime;
}

//----------------------------------------------------------------------------
void vtkImplicitProjectOnPlaneDistance::CreateDefaultLocator()
{
  if (this->Locator == nullptr)
  {
    this->Locator = vtkSmartPointer<vtkStaticCellLocator>::New();
  }
}

//-----------------------------------------------------------------------------
double vtkImplicitProjectOnPlaneDistance::EvaluateFunction(double x[3])
{
  if (!this->Input)
  {
    vtkErrorMacro("No input defined.");
    return -1;
  }

  double projected[3];
  this->ProjectionPlane->ProjectPoint(x, projected);

  if (this->Norm == NormType::L0)
  {
    // avoid costly FindClosestPoint if the projection point
    // is outside the bounding box of the polydata
    double tolerandAlongEachAxis[3] = { this->Tolerance, this->Tolerance, this->Tolerance };
    if (!vtkMath::PointIsWithinBounds(projected, this->Bounds, tolerandAlongEachAxis))
    {
      return 1;
    }
  }

  double unusedProjection[3];
  vtkIdType unusedCellId;
  int unusedSubId;
  double distanceToCell;
  this->Locator->FindClosestPoint(
    projected, unusedProjection, this->UnusedCell, unusedCellId, unusedSubId, distanceToCell);
  if (this->Norm == NormType::L0)
  {
    return distanceToCell > this->Tolerance;
  }
  return distanceToCell;
}

//-----------------------------------------------------------------------------
void vtkImplicitProjectOnPlaneDistance::EvaluateGradient(
  double vtkNotUsed(x)[3], double vtkNotUsed(g)[3])
{
  assert("This method is not implemented as it is of no use in the context of "
         "vtkImplicitProjectOnPlaceDistance" &&
    false);
}

//-----------------------------------------------------------------------------
void vtkImplicitProjectOnPlaneDistance::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImplicitFunction::PrintSelf(os, indent);

  os << indent << "Tolerance : " << this->Tolerance << "\n";
  os << indent << "Norm : " << (this->Norm == NormType::L0 ? "NormType::L0" : "NormType::L2");
  os << '\n';
  if (this->Norm == NormType::L0)
  {
    os << indent << "Bounds :";
    for (short i = 0; i < 6; i++)
    {
      os << " " << this->Bounds[i];
    }
    os << '\n';
  }

  if (this->Input)
  {
    os << indent << "Input : " << this->Input << "\n";
  }
  else
  {
    os << indent << "Input : (none)\n";
  }

  if (this->Locator)
  {
    os << indent << "Locator : " << this->Locator << "\n";
  }
  else
  {
    os << indent << "Locator : (none)\n";
  }

  if (this->ProjectionPlane)
  {
    os << indent << "ProjectionPlane : " << this->ProjectionPlane << "\n";
  }
  else
  {
    os << indent << "ProjectionPlane : (none)\n";
  }
}
