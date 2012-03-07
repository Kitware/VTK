/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCylindricalTransform.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCylindricalTransform.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"
#include <cmath>
#include <stdlib.h>

vtkStandardNewMacro(vtkCylindricalTransform);

//----------------------------------------------------------------------------
vtkCylindricalTransform::vtkCylindricalTransform()
{
}

vtkCylindricalTransform::~vtkCylindricalTransform()
{
}

void vtkCylindricalTransform::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkWarpTransform::PrintSelf(os,indent);
}

void vtkCylindricalTransform::InternalDeepCopy(vtkAbstractTransform *transform)
{
  vtkCylindricalTransform *cylindricalTransform = 
    static_cast<vtkCylindricalTransform *>(transform);

  // copy these even though they aren't used
  this->SetInverseTolerance(cylindricalTransform->InverseTolerance);
  this->SetInverseIterations(cylindricalTransform->InverseIterations);

  // copy the inverse flag, which is used
  if (this->InverseFlag != cylindricalTransform->InverseFlag)
    {
    this->InverseFlag = cylindricalTransform->InverseFlag;
    this->Modified();
    }
}

vtkAbstractTransform *vtkCylindricalTransform::MakeTransform()
{
  return vtkCylindricalTransform::New();
}

template<class T>
void vtkCylindricalToRectangular(const T inPoint[3], T outPoint[3],
                                        T derivative[3][3])
{
  T r = inPoint[0];
  T sintheta = static_cast<T>(sin(inPoint[1]));
  T costheta = static_cast<T>(cos(inPoint[1]));
  T z = inPoint[2];

  outPoint[0] = r*costheta;
  outPoint[1] = r*sintheta;
  outPoint[2] = z;

  if (derivative)
    {
    derivative[0][0] =    costheta;
    derivative[0][1] = -r*sintheta;
    derivative[0][2] =           0;

    derivative[1][0] =    sintheta;
    derivative[1][1] =  r*costheta;
    derivative[1][2] =           0;

    derivative[2][0] =           0;
    derivative[2][1] =           0;
    derivative[2][2] =           1;
    }
}

template<class T>
void vtkRectangularToCylindrical(const T inPoint[3], T outPoint[3])
{
  T x = inPoint[0];
  T y = inPoint[1];
  T z = inPoint[2];
  
  T rr = x*x + y*y;

  outPoint[0] = static_cast<T>(sqrt(rr));
  if (rr == 0)
    {
    outPoint[1] = 0;
    }
  else
    {
    // Change range to [0, 2*Pi], otherwise the same as atan2(y, x)
    outPoint[1] = T(vtkMath::DoublePi()) + atan2(-y, -x);
    }
  outPoint[2] = z;
}

void vtkCylindricalTransform::ForwardTransformPoint(const float inPoint[3],
                                                    float outPoint[3])
{
  vtkCylindricalToRectangular(inPoint, outPoint, static_cast<float (*)[3]>(0));
}

void vtkCylindricalTransform::ForwardTransformPoint(const double inPoint[3],
                                                    double outPoint[3])
{
  vtkCylindricalToRectangular(inPoint, outPoint,
                              static_cast<double (*)[3]>(0));
}

void vtkCylindricalTransform::ForwardTransformDerivative(
                                                       const float inPoint[3],
                                                       float outPoint[3],
                                                       float derivative[3][3])
{
  vtkCylindricalToRectangular(inPoint, outPoint, derivative);
}

void vtkCylindricalTransform::ForwardTransformDerivative(
                                                       const double inPoint[3],
                                                       double outPoint[3],
                                                       double derivative[3][3])
{
  vtkCylindricalToRectangular(inPoint, outPoint, derivative);
}

void vtkCylindricalTransform::InverseTransformPoint(const float inPoint[3],
                                                    float outPoint[3])
{
  vtkRectangularToCylindrical(inPoint, outPoint);
}

void vtkCylindricalTransform::InverseTransformPoint(const double inPoint[3],
                                                    double outPoint[3])
{
  vtkRectangularToCylindrical(inPoint, outPoint);
}

void vtkCylindricalTransform::InverseTransformDerivative(
                                                       const float inPoint[3],
                                                       float outPoint[3],
                                                       float derivative[3][3])
{
  float tmp[3];
  vtkRectangularToCylindrical(inPoint, outPoint);
  vtkCylindricalToRectangular(outPoint, tmp, derivative);
}

void vtkCylindricalTransform::InverseTransformDerivative(
                                                       const double inPoint[3],
                                                       double outPoint[3],
                                                       double derivative[3][3])
{
  double tmp[3];
  vtkRectangularToCylindrical(inPoint, outPoint);
  vtkCylindricalToRectangular(outPoint, tmp, derivative);
}

