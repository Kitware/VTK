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

// .NAME vtkHighestDensityRegionsStatistics - Compute a random vector of
// density f from input observations points. f is computed using a smooth
// kernel method.
//
// .SECTION Description
// Given a selection of pairs of columns of interest, this class provides the
// following functionalities, depending on the chosen execution options:
// * Learn: calculates density estimator f of a random vector using a smooth
//   gaussian kernel. The output metadata on port OUTPUT_MODEL is a multiblock
//   dataset containing at one vtkTable holding three columns which are for the
//   first columns the input columns of interest and for the last columns the
//   density estimators of each input pair of columns of interest.
// * Derive: calculate normalized (as a percentage) quantiles coming from
//   Learn output. The second block of the multibloc dataset contains a
//   vtkTable holding some pairs of columns which are for the second one the
//   quantiles ordered from the stronger to the lower and for the first one
//   the correspondand quantile index.
// * Assess: not implemented.
// * Test: not implemented.

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
  virtual void PrintSelf( ostream& os, vtkIndent indent );
  static vtkHighestDensityRegionsStatistics* New();

  // Description: (Not implemented)
  // Given a collection of models, calculate aggregate model
  virtual void Aggregate(vtkDataObjectCollection*,
                         vtkMultiBlockDataSet*) { return; }

  // Description:
  // H is a positive matrix that defines the smooth direction.
  // In a classical HDR, we don't set a specific smooth direction for the
  // H matrix parameter (SmoothHC1, SmoothHC2). That mean H will be in a
  // diagonal form and equal to sigma * Id.
  void SetSigma(double sigma);

  // Description:
  // Get Smooth H matrix parameter of the HDR.
  vtkGetVectorMacro(SmoothHC1, double, 2);
  vtkSetVectorMacro(SmoothHC1, double, 2);
  vtkGetVectorMacro(SmoothHC2, double, 2);
  vtkSetVectorMacro(SmoothHC2, double, 2);

protected:
  vtkHighestDensityRegionsStatistics();
  ~vtkHighestDensityRegionsStatistics();

  // Description:
  // Execute the calculations required by the Learn option.
  virtual void Learn(vtkTable*,
                     vtkTable*,
                     vtkMultiBlockDataSet*);

  // Description:
  // Execute the calculations required by the Derive option.
  virtual void Derive(vtkMultiBlockDataSet*);

  // Description: (Not implemented)
  // Execute the calculations required by the Assess option.
  virtual void Assess(vtkTable*,
                      vtkMultiBlockDataSet*,
                      vtkTable*) { return; }

  // Description: (Not implemented)
  // Execute the calculations required by the Test option.
  virtual void Test(vtkTable*,
                    vtkMultiBlockDataSet*,
                    vtkTable*) { return; }

//BTX
  // Description: (Not implemented)
  // Provide the appropriate assessment functor.
  virtual void SelectAssessFunctor(vtkTable*,
                                   vtkDataObject*,
                                   vtkStringArray*,
                                   AssessFunctor*&) { return; }
//ETX

  // Description:
  // Fill outDensity with density vector that is computed from
  // inObservations values. This method uses a Gaussian kernel.
  // For n observations and with X an observation point:
  // f(X) = (1 / n) * Sum(KH(X -Xi)) for (i = 1 to n).
  // Look ComputeSmoothGaussianKernel for KH kernel definition.
  double ComputeHDR(vtkDataArray *inObservations, vtkDataArray *outDensity);

  // Description:
  // Store the smooth matrix parameter H. Specify a smooth direction
  // for the Gaussian kernel.
  double SmoothHC1[2];
  double SmoothHC2[2];

  // Description:
  // Store the number of requested columns pair computed by learn method.
  vtkIdType NumberOfRequestedColumnsPair;

private :
  // Description:
  // Helper that returns a smooth gaussian kernel of a vector of dimension two,
  // using its coordinates. For X = [khx, khy] and H a positive matrix of dim 2,
  // KH(X) = sqrt(det(H)) * K((1 / sqrt(H)) * X).
  // Look ComputeStandardGaussianKernel for the K kernel definition.
  double ComputeSmoothGaussianKernel(int dimension, double khx, double khy);

  // Description:
  // Helper that returns a standard gaussian kernel of a vector of dimension two,
  // using its coordinates. For X = [kx, ky],
  // K(X) = ( 1 / 2 * PI) * exp(-sqrt<X,X>).
  double ComputeStandardGaussianKernel(int dimension, double kx, double ky);

private:
  vtkHighestDensityRegionsStatistics(const vtkHighestDensityRegionsStatistics&); // Not implemented
  void operator = (const vtkHighestDensityRegionsStatistics&);  // Not implemented
};

#endif
