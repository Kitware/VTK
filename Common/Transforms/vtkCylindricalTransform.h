/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCylindricalTransform.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCylindricalTransform - cylindrical to rectangular coords and back
// .SECTION Description
// vtkCylindricalTransform will convert (r,theta,z) coordinates to
// (x,y,z) coordinates and back again.  The angles are given in radians.
// By default, it converts cylindrical coordinates to rectangular, but
// GetInverse() returns a transform that will do the opposite.  The
// equation that is used is x = r*cos(theta), y = r*sin(theta), z = z.
// .SECTION Caveats
// This transform is not well behaved along the line x=y=0 (i.e. along
// the z-axis)
// .SECTION see also
// vtkSphericalTransform vtkGeneralTransform

#ifndef vtkCylindricalTransform_h
#define vtkCylindricalTransform_h

#include "vtkCommonTransformsModule.h" // For export macro
#include "vtkWarpTransform.h"

class VTKCOMMONTRANSFORMS_EXPORT vtkCylindricalTransform : public vtkWarpTransform
{
public:
  static vtkCylindricalTransform *New();
  vtkTypeMacro(vtkCylindricalTransform,vtkWarpTransform);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Make another transform of the same type.
  vtkAbstractTransform *MakeTransform();

protected:
  vtkCylindricalTransform();
  ~vtkCylindricalTransform();

  // Description:
  // Copy this transform from another of the same type.
  void InternalDeepCopy(vtkAbstractTransform *transform);

  // Description:
  // Internal functions for calculating the transformation.
  void ForwardTransformPoint(const float in[3], float out[3]);
  void ForwardTransformPoint(const double in[3], double out[3]);

  void ForwardTransformDerivative(const float in[3], float out[3],
                                  float derivative[3][3]);
  void ForwardTransformDerivative(const double in[3], double out[3],
                                  double derivative[3][3]);

  void InverseTransformPoint(const float in[3], float out[3]);
  void InverseTransformPoint(const double in[3], double out[3]);

  void InverseTransformDerivative(const float in[3], float out[3],
                                  float derivative[3][3]);
  void InverseTransformDerivative(const double in[3], double out[3],
                                  double derivative[3][3]);

private:
  vtkCylindricalTransform(const vtkCylindricalTransform&); // Not implemented.
  void operator=(const vtkCylindricalTransform&); // Not implemented.
};

#endif

