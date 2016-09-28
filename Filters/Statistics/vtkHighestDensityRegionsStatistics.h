/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkHighestDensityRegionsStatistics.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkHighestDensityRegionsStatistics
 * @brief   Compute a random vector of
 * density f from input observations points. f is computed using a smooth
 * kernel method.
 *
 *
 * Given a selection of pairs of columns of interest, this class provides the
 * following functionalities, depending on the chosen execution options:
 * * Learn: calculates density estimator f of a random vector using a smooth
 *   gaussian kernel. The output metadata on port OUTPUT_MODEL is a multiblock
 *   dataset containing at one vtkTable holding three columns which are for the
 *   first columns the input columns of interest and for the last columns the
 *   density estimators of each input pair of columns of interest.
 * * Derive: calculate normalized (as a percentage) quantiles coming from
 *   Learn output. The second block of the multibloc dataset contains a
 *   vtkTable holding some pairs of columns which are for the second one the
 *   quantiles ordered from the stronger to the lower and for the first one
 *   the correspondand quantile index.
 * * Assess: not implemented.
 * * Test: not implemented.
*/

#ifndef vtkHighestDensityRegionsStatistics_h
#define vtkHighestDensityRegionsStatistics_h

#include "vtkFiltersStatisticsModule.h" // For export macro
#include "vtkStatisticsAlgorithm.h"

class vtkMultiBlockDataSet;
class vtkVariant;

class VTKFILTERSSTATISTICS_EXPORT vtkHighestDensityRegionsStatistics :
  public vtkStatisticsAlgorithm
{
public:
  vtkTypeMacro(vtkHighestDensityRegionsStatistics, vtkStatisticsAlgorithm);
  void PrintSelf( ostream& os, vtkIndent indent ) VTK_OVERRIDE;
  static vtkHighestDensityRegionsStatistics* New();

  /**
   * Given a collection of models, calculate aggregate model
   */
  void Aggregate(vtkDataObjectCollection*,
                 vtkMultiBlockDataSet*) VTK_OVERRIDE { return; }

  /**
   * Set the width of the gaussian kernel.
   */
  void SetSigma(double sigma);

  /**
   * Set the gaussian kernel matrix.
   */
  void SetSigmaMatrix(double s11, double s12, double s21, double s22);

  /**
   * Fill outDensity with density vector that is computed from
   * inObservations values. This method uses a Gaussian kernel.
   * For n observations and with X an observation point:
   * f(X) = (1 / n) * Sum(KH(X -Xi)) for (i = 1 to n).
   * Look ComputeSmoothGaussianKernel for KH kernel definition.
   */
  double ComputeHDR(vtkDataArray *inObservations, vtkDataArray *outDensity);

  /**
   * Fill outDensity with density vector defined by inPOI and computed from
   * the inObs values. This method uses a Gaussian kernel.
   * For n observations and with X an observation point:
   * f(X) = (1 / n) * Sum(KH(X -Xi)) for (i = 1 to n).
   * Look ComputeSmoothGaussianKernel for KH kernel definition.
   */
  double ComputeHDR(vtkDataArray *inObs, vtkDataArray* inPOI,
                    vtkDataArray *outDensity);

protected:
  vtkHighestDensityRegionsStatistics();
  ~vtkHighestDensityRegionsStatistics() VTK_OVERRIDE;

  /**
   * Execute the calculations required by the Learn option.
   */
  void Learn(vtkTable*,
             vtkTable*,
             vtkMultiBlockDataSet*) VTK_OVERRIDE;

  /**
   * Execute the calculations required by the Derive option.
   */
  void Derive(vtkMultiBlockDataSet*) VTK_OVERRIDE;

  /**
   * Execute the calculations required by the Assess option.
   */
  void Assess(vtkTable*,
              vtkMultiBlockDataSet*,
              vtkTable*) VTK_OVERRIDE { return; }

  /**
   * Execute the calculations required by the Test option.
   */
  void Test(vtkTable*,
            vtkMultiBlockDataSet*,
            vtkTable*) VTK_OVERRIDE { return; }

  /**
   * Provide the appropriate assessment functor.
   */
  void SelectAssessFunctor(vtkTable*,
                           vtkDataObject*,
                           vtkStringArray*,
                           AssessFunctor*&) VTK_OVERRIDE { return; }

  //@{
  /**
   * Store the smooth matrix parameter H. Specify a smooth direction
   * for the Gaussian kernel.
   */
  double SmoothHC1[2];
  double SmoothHC2[2];
  double InvSigmaC1[2];
  double InvSigmaC2[2];
  double Determinant;
  //@}

  /**
   * Store the number of requested columns pair computed by learn method.
   */
  vtkIdType NumberOfRequestedColumnsPair;

private :
  /**
   * Helper that returns a smooth gaussian kernel of a vector of dimension two,
   * using its coordinates. For X = [khx, khy] and H a positive matrix of dim 2,
   * KH(X) = sqrt(det(H)) * K((1 / sqrt(H)) * X).
   * Look ComputeStandardGaussianKernel for the K kernel definition.
   */
  double ComputeSmoothGaussianKernel(int dimension, double khx, double khy);

private:
  vtkHighestDensityRegionsStatistics(const vtkHighestDensityRegionsStatistics&) VTK_DELETE_FUNCTION;
  void operator = (const vtkHighestDensityRegionsStatistics&) VTK_DELETE_FUNCTION;
};

#endif
