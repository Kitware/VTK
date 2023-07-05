// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCoordinateFrame
 * @brief   implicit function for a right-handed coordinate system
 *
 * vtkCoordinateFrame computes an implicit function and function gradient
 * for a set of 3 orthogonal planes.
 *
 * The function evaluates to a combination of quartic spherical harmonic
 * basis functions:
 * \f$\sqrt(\frac{7}{12})*Y_{4,0} + \sqrt(\frac{5}{12})*Y_{4,4}\f$
 * that – when evaluated on a unit sphere centered at the coordinate frame's
 * origin – form a 6-lobed function with a maximum along each of the
 * 6 axes (3 positive, 3 negative).
 * This function is frequently used in frame-field design.
 *
 * See the paper "On Smooth Frame Field Design" by Nicolas Ray and
 * Dmitry Sokolov (2016, hal-01245657,
 * https://hal.inria.fr/hal-01245657/file/framefield.pdf ) for more
 * information.
 */

#ifndef vtkCoordinateFrame_h
#define vtkCoordinateFrame_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkImplicitFunction.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkPlane;
class vtkDataArray;

class VTKCOMMONDATAMODEL_EXPORT vtkCoordinateFrame : public vtkImplicitFunction
{
public:
  ///@{
  /**
   * Standard methods for instantiation, type information, and printing.
   */
  static vtkCoordinateFrame* New();
  vtkTypeMacro(vtkCoordinateFrame, vtkImplicitFunction);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Evaluate plane equations. Return largest value (i.e., an intersection
   * operation between all planes).
   */
  using vtkImplicitFunction::EvaluateFunction;
  double EvaluateFunction(double x[3]) override;
  ///@}

  /**
   * Evaluate coordinate frame gradient.
   *
   * \a n is the output gradient evaluated at point \a x.
   */
  void EvaluateGradient(double x[3], double n[3]) override;

  ///@{
  /**
   * Specify the point through which all 3 planes pass.
   */
  vtkSetVector3Macro(Origin, double);
  vtkGetVector3Macro(Origin, double);
  ///@}

  ///@{
  /**
   * Specify a list of unit-length normal vectors for each plane.
   */
  vtkSetVector3Macro(XAxis, double);
  vtkGetVector3Macro(XAxis, double);
  vtkSetVector3Macro(YAxis, double);
  vtkGetVector3Macro(YAxis, double);
  vtkSetVector3Macro(ZAxis, double);
  vtkGetVector3Macro(ZAxis, double);
  ///@}

protected:
  vtkCoordinateFrame() = default;
  ~vtkCoordinateFrame() override = default;

  double Origin[3] = { 0, 0, 0 };
  double XAxis[3] = { 1, 0, 0 };
  double YAxis[3] = { 0, 1, 0 };
  double ZAxis[3] = { 0, 0, 1 };

private:
  vtkCoordinateFrame(const vtkCoordinateFrame&) = delete;
  void operator=(const vtkCoordinateFrame&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
