// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageSpatialAlgorithm
 * @brief   Filters that operate on pixel neighborhoods.
 *
 * vtkImageSpatialAlgorithm is a super class for filters that operate on an
 * input neighborhood for each output pixel. It handles even sized
 * neighborhoods, but their can be a half pixel shift associated with
 * processing.  This superclass has some logic for handling boundaries.  It
 * can split regions into boundary and non-boundary pieces and call different
 * execute methods.
 */

#ifndef vtkImageSpatialAlgorithm_h
#define vtkImageSpatialAlgorithm_h

#include "vtkImagingGeneralModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKIMAGINGGENERAL_EXPORT vtkImageSpatialAlgorithm : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageSpatialAlgorithm* New();
  vtkTypeMacro(vtkImageSpatialAlgorithm, vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the Kernel size.
   */
  vtkGetVector3Macro(KernelSize, int);
  ///@}

  ///@{
  /**
   * Get the Kernel middle.
   */
  vtkGetVector3Macro(KernelMiddle, int);
  ///@}

protected:
  vtkImageSpatialAlgorithm();
  ~vtkImageSpatialAlgorithm() override = default;

  int KernelSize[3];
  int KernelMiddle[3];  // Index of kernel origin
  int HandleBoundaries; // Output shrinks if boundaries aren't handled

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  void ComputeOutputWholeExtent(int extent[6], int handleBoundaries);
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  void InternalRequestUpdateExtent(int* extent, int* inExtent, int* wholeExtent);

private:
  vtkImageSpatialAlgorithm(const vtkImageSpatialAlgorithm&) = delete;
  void operator=(const vtkImageSpatialAlgorithm&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
