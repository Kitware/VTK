// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageIslandRemoval2D
 * @brief   Removes small clusters in masks.
 *
 * vtkImageIslandRemoval2D computes the area of separate islands in
 * a mask image.  It removes any island that has less than AreaThreshold
 * pixels.  Output has the same ScalarType as input.  It generates
 * the whole 2D output image for any output request.
 */

#ifndef vtkImageIslandRemoval2D_h
#define vtkImageIslandRemoval2D_h

#include "vtkImageAlgorithm.h"
#include "vtkImagingMorphologicalModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
struct vtkImage2DIslandPixel_t
{
  void* inPtr;
  void* outPtr;
  int idx0;
  int idx1;
};
using vtkImage2DIslandPixel = struct vtkImage2DIslandPixel_t;

class VTKIMAGINGMORPHOLOGICAL_EXPORT vtkImageIslandRemoval2D : public vtkImageAlgorithm
{
public:
  ///@{
  /**
   * Constructor: Sets default filter to be identity.
   */
  static vtkImageIslandRemoval2D* New();
  vtkTypeMacro(vtkImageIslandRemoval2D, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Set/Get the cutoff area for removal
   */
  vtkSetMacro(AreaThreshold, int);
  vtkGetMacro(AreaThreshold, int);
  ///@}

  ///@{
  /**
   * Set/Get whether to use 4 or 8 neighbors
   */
  vtkSetMacro(SquareNeighborhood, vtkTypeBool);
  vtkGetMacro(SquareNeighborhood, vtkTypeBool);
  vtkBooleanMacro(SquareNeighborhood, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get the value to remove.
   */
  vtkSetMacro(IslandValue, double);
  vtkGetMacro(IslandValue, double);
  ///@}

  ///@{
  /**
   * Set/Get the value to put in the place of removed pixels.
   */
  vtkSetMacro(ReplaceValue, double);
  vtkGetMacro(ReplaceValue, double);
  ///@}

protected:
  vtkImageIslandRemoval2D();
  ~vtkImageIslandRemoval2D() override = default;

  int AreaThreshold;
  vtkTypeBool SquareNeighborhood;
  double IslandValue;
  double ReplaceValue;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkImageIslandRemoval2D(const vtkImageIslandRemoval2D&) = delete;
  void operator=(const vtkImageIslandRemoval2D&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
