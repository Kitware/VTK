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
/**
 * @class   vtkCylindricalTransform
 * @brief   cylindrical to rectangular coords and back
 *
 * vtkCylindricalTransform will convert (r,theta,z) coordinates to
 * (x,y,z) coordinates and back again.  The angles are given in radians.
 * By default, it converts cylindrical coordinates to rectangular, but
 * GetInverse() returns a transform that will do the opposite.  The
 * equation that is used is x = r*cos(theta), y = r*sin(theta), z = z.
 * @warning
 * This transform is not well behaved along the line x=y=0 (i.e. along
 * the z-axis)
 * @sa
 * vtkSphericalTransform vtkGeneralTransform
*/

#ifndef vtkCylindricalTransform_h
#define vtkCylindricalTransform_h

#include "vtkCommonTransformsModule.h" // For export macro
#include "vtkWarpTransform.h"

class VTKCOMMONTRANSFORMS_EXPORT vtkCylindricalTransform : public vtkWarpTransform
{
public:
  static vtkCylindricalTransform *New();
  vtkTypeMacro(vtkCylindricalTransform,vtkWarpTransform);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Make another transform of the same type.
   */
  vtkAbstractTransform *MakeTransform() VTK_OVERRIDE;

protected:
  vtkCylindricalTransform();
  ~vtkCylindricalTransform() VTK_OVERRIDE;

  /**
   * Copy this transform from another of the same type.
   */
  void InternalDeepCopy(vtkAbstractTransform *transform) VTK_OVERRIDE;

  //@{
  /**
   * Internal functions for calculating the transformation.
   */
  void ForwardTransformPoint(const float in[3], float out[3]) VTK_OVERRIDE;
  void ForwardTransformPoint(const double in[3], double out[3]) VTK_OVERRIDE;
  //@}

  void ForwardTransformDerivative(const float in[3], float out[3],
                                  float derivative[3][3]) VTK_OVERRIDE;
  void ForwardTransformDerivative(const double in[3], double out[3],
                                  double derivative[3][3]) VTK_OVERRIDE;

  void InverseTransformPoint(const float in[3], float out[3]) VTK_OVERRIDE;
  void InverseTransformPoint(const double in[3], double out[3]) VTK_OVERRIDE;

  void InverseTransformDerivative(const float in[3], float out[3],
                                  float derivative[3][3]) VTK_OVERRIDE;
  void InverseTransformDerivative(const double in[3], double out[3],
                                  double derivative[3][3]) VTK_OVERRIDE;

private:
  vtkCylindricalTransform(const vtkCylindricalTransform&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCylindricalTransform&) VTK_DELETE_FUNCTION;
};

#endif

