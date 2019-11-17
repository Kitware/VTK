/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSpheres.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSpheres.h"

#include "vtkDoubleArray.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkSphere.h"

#include <cmath>

vtkStandardNewMacro(vtkSpheres);
vtkCxxSetObjectMacro(vtkSpheres, Centers, vtkPoints);

//----------------------------------------------------------------------------
vtkSpheres::vtkSpheres()
{
  this->Centers = nullptr;
  this->Radii = nullptr;
  this->Sphere = vtkSphere::New();
}

//----------------------------------------------------------------------------
vtkSpheres::~vtkSpheres()
{
  if (this->Centers)
  {
    this->Centers->UnRegister(this);
  }
  if (this->Radii)
  {
    this->Radii->UnRegister(this);
  }
  this->Sphere->Delete();
}

//----------------------------------------------------------------------------
void vtkSpheres::SetRadii(vtkDataArray* radii)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting Radii to " << radii);

  if (radii && radii->GetNumberOfComponents() != 1)
  {
    vtkWarningMacro("This array does not have 1 components. Ignoring radii.");
    return;
  }

  if (this->Radii != radii)
  {
    if (this->Radii != nullptr)
    {
      this->Radii->UnRegister(this);
    }
    this->Radii = radii;
    if (this->Radii != nullptr)
    {
      this->Radii->Register(this);
    }
    this->Modified();
  }
}

//----------------------------------------------------------------------------
// Evaluate sphere equations. Return smallest absolute value.
double vtkSpheres::EvaluateFunction(double x[3])
{
  int numSpheres, i;
  double val, minVal;
  double radius[1], center[3];

  if (!this->Centers || !this->Radii)
  {
    vtkErrorMacro(<< "Please define points and/or radii!");
    return VTK_DOUBLE_MAX;
  }

  if ((numSpheres = this->Centers->GetNumberOfPoints()) != this->Radii->GetNumberOfTuples())
  {
    vtkErrorMacro(<< "Number of radii/points inconsistent!");
    return VTK_DOUBLE_MAX;
  }

  for (minVal = VTK_DOUBLE_MAX, i = 0; i < numSpheres; i++)
  {
    this->Radii->GetTuple(i, radius);
    this->Centers->GetPoint(i, center);
    val = this->Sphere->Evaluate(center, radius[0], x);
    if (val < minVal)
    {
      minVal = val;
    }
  }

  return minVal;
}

//----------------------------------------------------------------------------
// Evaluate spheres gradient.
void vtkSpheres::EvaluateGradient(double x[3], double n[3])
{
  int numSpheres, i;
  double val, minVal;
  double rTemp[1];
  double cTemp[3];

  if (!this->Centers || !this->Radii)
  {
    vtkErrorMacro(<< "Please define centers and radii!");
    return;
  }

  if ((numSpheres = this->Centers->GetNumberOfPoints()) != this->Radii->GetNumberOfTuples())
  {
    vtkErrorMacro(<< "Number of radii/centersinconsistent!");
    return;
  }

  for (minVal = VTK_DOUBLE_MAX, i = 0; i < numSpheres; i++)
  {
    this->Radii->GetTuple(i, rTemp);
    this->Centers->GetPoint(i, cTemp);
    val = this->Sphere->Evaluate(cTemp, rTemp[0], x);
    if (val < minVal)
    {
      minVal = val;
      n[0] = x[0] - cTemp[0];
      n[1] = x[1] - cTemp[1];
      n[2] = x[2] - cTemp[2];
    }
  }
}

//----------------------------------------------------------------------------
int vtkSpheres::GetNumberOfSpheres()
{
  if (this->Centers && this->Radii)
  {
    int npts = this->Centers->GetNumberOfPoints();
    int nradii = this->Radii->GetNumberOfTuples();
    return (npts <= nradii ? npts : nradii);
  }
  else
  {
    return 0;
  }
}

//----------------------------------------------------------------------------
vtkSphere* vtkSpheres::GetSphere(int i)
{
  double radius[1];
  double center[3];

  if (i >= 0 && i < this->GetNumberOfSpheres())
  {
    this->Radii->GetTuple(i, radius);
    this->Centers->GetPoint(i, center);
    this->Sphere->SetRadius(radius[0]);
    this->Sphere->SetCenter(center);
    return this->Sphere;
  }
  else
  {
    return nullptr;
  }
}

//----------------------------------------------------------------------------
void vtkSpheres::GetSphere(int i, vtkSphere* sphere)
{
  if (i >= 0 && i < this->GetNumberOfSpheres())
  {
    double radius[1];
    double center[3];
    this->Radii->GetTuple(i, radius);
    this->Centers->GetPoint(i, center);
    sphere->SetRadius(radius[0]);
    sphere->SetCenter(center);
  }
}

//----------------------------------------------------------------------------
void vtkSpheres::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  int numSpheres;
  if (this->Centers && (numSpheres = this->Centers->GetNumberOfPoints()) > 0)
  {
    os << indent << "Number of Spheres: " << numSpheres << "\n";
  }
  else
  {
    os << indent << "No Spheres Defined.\n";
  }

  if (this->Radii)
  {
    os << indent << "Radii: " << this->Radii << "\n";
  }
  else
  {
    os << indent << "Radii: (none)\n";
  }
}
