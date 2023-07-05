// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSubPixelPositionEdgels
 * @brief   adjust edgel locations based on gradients.
 *
 * vtkSubPixelPositionEdgels is a filter that takes a series of linked
 * edgels (digital curves) and gradient maps as input. It then adjusts
 * the edgel locations based on the gradient data. Specifically, the
 * algorithm first determines the neighboring gradient magnitudes of
 * an edgel using simple interpolation of its neighbors. It then fits
 * the following three data points: negative gradient direction
 * gradient magnitude, edgel gradient magnitude and positive gradient
 * direction gradient magnitude to a quadratic function. It then
 * solves this quadratic to find the maximum gradient location along
 * the gradient orientation.  It then modifies the edgels location
 * along the gradient orientation to the calculated maximum
 * location. This algorithm does not adjust an edgel in the direction
 * orthogonal to its gradient vector.
 *
 * @sa
 * vtkImageData vtkImageGradient vtkLinkEdgels
 */

#ifndef vtkSubPixelPositionEdgels_h
#define vtkSubPixelPositionEdgels_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkStructuredPoints;
class vtkDataArray;

class VTKFILTERSGENERAL_EXPORT vtkSubPixelPositionEdgels : public vtkPolyDataAlgorithm
{
public:
  static vtkSubPixelPositionEdgels* New();
  vtkTypeMacro(vtkSubPixelPositionEdgels, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the gradient data for doing the position adjustments.
   */
  void SetGradMapsData(vtkStructuredPoints* gm);
  vtkStructuredPoints* GetGradMaps();
  ///@}

  ///@{
  /**
   * These methods can make the positioning look for a target scalar value
   * instead of looking for a maximum.
   */
  vtkSetMacro(TargetFlag, vtkTypeBool);
  vtkGetMacro(TargetFlag, vtkTypeBool);
  vtkBooleanMacro(TargetFlag, vtkTypeBool);
  vtkSetMacro(TargetValue, double);
  vtkGetMacro(TargetValue, double);
  ///@}

protected:
  vtkSubPixelPositionEdgels();
  ~vtkSubPixelPositionEdgels() override;

  // Usual data generation method
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  void Move(int xdim, int ydim, int zdim, int x, int y, float* img, vtkDataArray* inVecs,
    double* result, int z, double* aspect, double* resultNormal);
  void Move(int xdim, int ydim, int zdim, int x, int y, double* img, vtkDataArray* inVecs,
    double* result, int z, double* aspect, double* resultNormal);
  // extension for target instead of maximum
  vtkTypeBool TargetFlag;
  double TargetValue;

private:
  vtkSubPixelPositionEdgels(const vtkSubPixelPositionEdgels&) = delete;
  void operator=(const vtkSubPixelPositionEdgels&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
