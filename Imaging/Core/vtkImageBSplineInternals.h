/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageBSplineInternals.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageBSplineInternals
 * @brief   BSpline code from P. Thevenaz
 *
 * vtkImageBSplineInternals provides code for image interpolation with
 * b-splines of various degrees.  This code computes the coefficents
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

#include "vtkImagingCoreModule.h" // For export macro
#include "vtkSystemIncludes.h"

class VTKIMAGINGCORE_EXPORT vtkImageBSplineInternals
{
public:
  /**
   * Internal method.  Get the poles for spline of given degree.
   * Returns zero if an illegal degree is given (allowed range 2 to 9).
   * The parameter numPoles will be set to a value between 1 and 4.
   */
  static int GetPoleValues(double poles[4], long &numPoles, long degree);

  /**
   * Internal method.  Compute the coefficients for one row of data.
   */
  static void ConvertToInterpolationCoefficients(
    double data[], long size, long border, double poles[4], long numPoles,
    double tol);

  //@{
  /**
   * Internal method.  Get interpolation weights for offset w, where
   * w is between 0 and 1.  You must provide the degree of the spline.
   */
  static int GetInterpolationWeights(
    double weights[10], double w, long degree);
  static int GetInterpolationWeights(
    float weights[10], double w, long degree);
  //@}

  //@{
  /**
   * Internal method.  Interpolate a value from the supplied 3D array
   * of coefficients with dimensions width x height x slices.
   */
  static int InterpolatedValue(
    const double *coeffs, double *value,
    long width, long height, long slices, long depth,
    double x, double y, double z, long degree, long border);
  static int InterpolatedValue(
    const float *coeffs, float *value,
    long width, long height, long slices, long depth,
    double x, double y, double z, long degree, long border);
  //@}

protected:
  vtkImageBSplineInternals() {}
  ~vtkImageBSplineInternals() {}

  static double InitialCausalCoefficient(
    double data[], long size, long border, double pole, double tol);

  static double InitialAntiCausalCoefficient(
    double data[], long size, long border, double pole, double tol);

private:
  vtkImageBSplineInternals(const vtkImageBSplineInternals&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImageBSplineInternals&) VTK_DELETE_FUNCTION;
};

#endif
// VTK-HeaderTest-Exclude: vtkImageBSplineInternals.h
