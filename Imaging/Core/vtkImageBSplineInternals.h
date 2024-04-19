// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageBSplineInternals
 * @brief   BSpline code from P. Thevenaz
 *
 * vtkImageBSplineInternals provides code for image interpolation with
 * b-splines of various degrees.  This code computes the coefficients
 * from the image, and computes the weights for the b-spline kernels.
 *
 * This class is based on code provided by Philippe Thevenaz of
 * EPFL, Lausanne, Switzerland.  Please acknowledge his contribution
 * by citing the following paper:
 * [1] P. Thevenaz, T. Blu, M. Unser, "Interpolation Revisited,"
 *     IEEE Transactions on Medical Imaging 19(7):739-758, 2000.
 *
 * The clamped boundary condition (which is the default) is taken
 * from code presented in the following paper:
 * [2] D. Ruijters, P. Thevenaz,
 *     "GPU Prefilter for Accurate Cubic B-spline Interpolation,"
 *     The Computer Journal, doi: 10.1093/comjnl/bxq086, 2010.
 */

#ifndef vtkImageBSplineInternals_h
#define vtkImageBSplineInternals_h

#include "vtkAbstractImageInterpolator.h" // For vtkImageBorderMode
#include "vtkImagingCoreModule.h"         // For export macro
#include "vtkSystemIncludes.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKIMAGINGCORE_EXPORT vtkImageBSplineInternals
{
public:
  /**
   * Internal method.  Get the poles for spline of given degree.
   * Returns zero if an illegal degree is given (allowed range 2 to 9).
   * The parameter numPoles will be set to a value between 1 and 4.
   */
  static int GetPoleValues(double poles[4], long& numPoles, long degree);

  /**
   * Internal method.  Compute the coefficients for one row of data.
   */
  static void ConvertToInterpolationCoefficients(double data[], long size,
    vtkImageBorderMode border, double poles[4], long numPoles, double tol) VTK_SIZEHINT(data, size);

  ///@{
  /**
   * Internal method.  Get interpolation weights for offset w, where
   * w is between 0 and 1.  You must provide the degree of the spline.
   */
  static int GetInterpolationWeights(double weights[10], double w, long degree);
  static int GetInterpolationWeights(float weights[10], double w, long degree);
  ///@}

  ///@{
  /**
   * Internal method.  Interpolate a value from the supplied 3D array
   * of coefficients with dimensions width x height x slices.
   */
  static int InterpolatedValue(const double* coeffs, double* value, long width, long height,
    long slices, long depth, double x, double y, double z, long degree, vtkImageBorderMode border);
  static int InterpolatedValue(const float* coeffs, float* value, long width, long height,
    long slices, long depth, double x, double y, double z, long degree, vtkImageBorderMode border);

protected:
  vtkImageBSplineInternals() = default;
  ~vtkImageBSplineInternals() = default;

  static double InitialCausalCoefficient(
    double data[], long size, vtkImageBorderMode border, double pole, double tol);

  static double InitialAntiCausalCoefficient(
    double data[], long size, vtkImageBorderMode border, double pole, double tol);

private:
  vtkImageBSplineInternals(const vtkImageBSplineInternals&) = delete;
  void operator=(const vtkImageBSplineInternals&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkImageBSplineInternals.h
