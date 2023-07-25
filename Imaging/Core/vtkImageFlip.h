// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageFlip
 * @brief   This flips an axis of an image. Right becomes left ...
 *
 * vtkImageFlip will reflect the data along the filtered axis.  This filter is
 * actually a thin wrapper around vtkImageReslice.
 */

#ifndef vtkImageFlip_h
#define vtkImageFlip_h

#include "vtkImageReslice.h"
#include "vtkImagingCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKIMAGINGCORE_EXPORT vtkImageFlip : public vtkImageReslice
{
public:
  static vtkImageFlip* New();

  vtkTypeMacro(vtkImageFlip, vtkImageReslice);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify which axis will be flipped.  This must be an integer
   * between 0 (for x) and 2 (for z). Initial value is 0.
   */
  vtkSetMacro(FilteredAxis, int);
  vtkGetMacro(FilteredAxis, int);
  ///@}

  ///@{
  /**
   * By default the image will be flipped about its center, and the
   * Origin, Spacing and Extent of the output will be identical to
   * the input.  However, if you have a coordinate system associated
   * with the image and you want to use the flip to convert +ve values
   * along one axis to -ve values (and vice versa) then you actually
   * want to flip the image about coordinate (0,0,0) instead of about
   * the center of the image.  This method will adjust the Origin of
   * the output such that the flip occurs about (0,0,0).  Note that
   * this method only changes the Origin (and hence the coordinate system)
   * the output data: the actual pixel values are the same whether or not
   * this method is used.  Also note that the Origin in this method name
   * refers to (0,0,0) in the coordinate system associated with the image,
   * it does not refer to the Origin ivar that is associated with a
   * vtkImageData.
   */
  vtkSetMacro(FlipAboutOrigin, vtkTypeBool);
  vtkGetMacro(FlipAboutOrigin, vtkTypeBool);
  vtkBooleanMacro(FlipAboutOrigin, vtkTypeBool);
  ///@}

  /**
   * Keep the mis-named Axes variations around for compatibility with old
   * scripts. Axis is singular, not plural...
   */
  void SetFilteredAxes(int axis) { this->SetFilteredAxis(axis); }
  int GetFilteredAxes() { return this->GetFilteredAxis(); }

  ///@{
  /**
   * PreserveImageExtentOff wasn't covered by test scripts and its
   * implementation was broken.  It is deprecated now and it has
   * no effect (i.e. the ImageExtent is always preserved).
   */
  vtkSetMacro(PreserveImageExtent, vtkTypeBool);
  vtkGetMacro(PreserveImageExtent, vtkTypeBool);
  vtkBooleanMacro(PreserveImageExtent, vtkTypeBool);
  ///@}

protected:
  vtkImageFlip();
  ~vtkImageFlip() override = default;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int FilteredAxis;
  vtkTypeBool FlipAboutOrigin;
  vtkTypeBool PreserveImageExtent;

private:
  vtkImageFlip(const vtkImageFlip&) = delete;
  void operator=(const vtkImageFlip&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
