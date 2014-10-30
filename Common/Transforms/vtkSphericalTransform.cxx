/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSphericalTransform.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSphericalTransform.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"
#include <cmath>
#include <cstdlib>

vtkStandardNewMacro(vtkSphericalTransform);

//----------------------------------------------------------------------------
vtkSphericalTransform::vtkSphericalTransform()
{
}

vtkSphericalTransform::~vtkSphericalTransform()
{
}

void vtkSphericalTransform::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkWarpTransform::PrintSelf(os,indent);
}

void vtkSphericalTransform::InternalDeepCopy(vtkAbstractTransform *transform)
{
  vtkSphericalTransform *sphericalTransform =
    static_cast<vtkSphericalTransform *>(transform);

  // copy these even though they aren't used
  this->SetInverseTolerance(sphericalTransform->InverseTolerance);
  this->SetInverseIterations(sphericalTransform->InverseIterations);

  // copy the inverse flag, which is used
  if (this->InverseFlag != sphericalTransform->InverseFlag)
    {
    this->InverseFlag = sphericalTransform->InverseFlag;
    this->Modified();
    }
}

vtkAbstractTransform *vtkSphericalTransform::MakeTransform()
{
  return vtkSphericalTransform::New();
}

template<class T>
void vtkSphericalToRectangular(const T inPoint[3], T outPoint[3],
                               T derivative[3][3])
{
  T r = inPoint[0];
  T sinphi = sin(inPoint[1]);
  T cosphi = cos(inPoint[1]);
  T sintheta = sin(inPoint[2]);
  T costheta = cos(inPoint[2]);

  outPoint[0] = r*sinphi*costheta;
  outPoint[1] = r*sinphi*sintheta;
  outPoint[2] = r*cosphi;

  if (derivative)
    {
    derivative[0][0] =    sinphi*costheta;
    derivative[0][1] =  r*cosphi*costheta;
    derivative[0][2] = -r*sinphi*sintheta;

    derivative[1][0] =    sinphi*sintheta;
    derivative[1][1] =  r*cosphi*sintheta;
    derivative[1][2] =  r*sinphi*costheta;

    derivative[2][0] =    cosphi;
    derivative[2][1] = -r*sinphi;
    derivative[2][2] =    0;
    }
}

template<class T>
void vtkRectangularToSpherical(const T inPoint[3], T outPoint[3])
{
  T x = inPoint[0];
  T y = inPoint[1];
  T z = inPoint[2];

  T RR = x*x + y*y;
  T r = sqrt(RR + z*z);

  outPoint[0] = r;
  if (r == 0)
    {
    outPoint[1] = 0;
    }
  else
    {
    outPoint[1] = acos(z/r);
    }
  if (RR == 0)
    {
    outPoint[2] = 0;
    }
  else
    {
    // Change range to [0, 2*Pi], otherwise the same as atan2(y, x)
    outPoint[2] = T(vtkMath::Pi()) + atan2(-y, -x);
    }
}

void vtkSphericalTransform::ForwardTransformPoint(const float inPoint[3],
                                                  float outPoint[3])
{
  vtkSphericalToRectangular(inPoint, outPoint, static_cast<float (*)[3]>(0));
}

void vtkSphericalTransform::ForwardTransformPoint(const double inPoint[3],
                                                  double outPoint[3])
{
  vtkSphericalToRectangular(inPoint, outPoint, static_cast<double (*)[3]>(0));
}

void vtkSphericalTransform::ForwardTransformDerivative(const float inPoint[3],
                                                       float outPoint[3],
                                                       float derivative[3][3])
{
  vtkSphericalToRectangular(inPoint, outPoint, derivative);
}

void vtkSphericalTransform::ForwardTransformDerivative(const double inPoint[3],
                                                       double outPoint[3],
                                                       double derivative[3][3])
{
  vtkSphericalToRectangular(inPoint, outPoint, derivative);
}

void vtkSphericalTransform::InverseTransformPoint(const float inPoint[3],
                                                  float outPoint[3])
{
  vtkRectangularToSpherical(inPoint, outPoint);
}

void vtkSphericalTransform::InverseTransformPoint(const double inPoint[3],
                                                  double outPoint[3])
{
  vtkRectangularToSpherical(inPoint, outPoint);
}

void vtkSphericalTransform::InverseTransformDerivative(const float inPoint[3],
                                                       float outPoint[3],
                                                       float derivative[3][3])
{
  float tmp[3];
  vtkRectangularToSpherical(inPoint, outPoint);
  vtkSphericalToRectangular(outPoint, tmp, derivative);
}

void vtkSphericalTransform::InverseTransformDerivative(const double inPoint[3],
                                                       double outPoint[3],
                                                       double derivative[3][3])
{
  double tmp[3];
  vtkRectangularToSpherical(inPoint, outPoint);
  vtkSphericalToRectangular(outPoint, tmp, derivative);
}

