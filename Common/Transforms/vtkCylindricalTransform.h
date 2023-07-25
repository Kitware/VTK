// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONTRANSFORMS_EXPORT vtkCylindricalTransform : public vtkWarpTransform
{
public:
  static vtkCylindricalTransform* New();
  vtkTypeMacro(vtkCylindricalTransform, vtkWarpTransform);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Make another transform of the same type.
   */
  vtkAbstractTransform* MakeTransform() override;

protected:
  vtkCylindricalTransform();
  ~vtkCylindricalTransform() override;

  /**
   * Copy this transform from another of the same type.
   */
  void InternalDeepCopy(vtkAbstractTransform* transform) override;

  ///@{
  /**
   * Internal functions for calculating the transformation.
   */
  void ForwardTransformPoint(const float in[3], float out[3]) override;
  void ForwardTransformPoint(const double in[3], double out[3]) override;
  ///@}

  void ForwardTransformDerivative(const float in[3], float out[3], float derivative[3][3]) override;
  void ForwardTransformDerivative(
    const double in[3], double out[3], double derivative[3][3]) override;

  void InverseTransformPoint(const float in[3], float out[3]) override;
  void InverseTransformPoint(const double in[3], double out[3]) override;

  void InverseTransformDerivative(const float in[3], float out[3], float derivative[3][3]) override;
  void InverseTransformDerivative(
    const double in[3], double out[3], double derivative[3][3]) override;

private:
  vtkCylindricalTransform(const vtkCylindricalTransform&) = delete;
  void operator=(const vtkCylindricalTransform&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
