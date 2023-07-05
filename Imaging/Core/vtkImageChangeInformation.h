// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageChangeInformation
 * @brief   modify spacing, origin and extent.
 *
 * vtkImageChangeInformation  modify the spacing, origin, or extent of
 * the data without changing the data itself.  The data is not resampled
 * by this filter, only the information accompanying the data is modified.
 */

#ifndef vtkImageChangeInformation_h
#define vtkImageChangeInformation_h

#include "vtkImageAlgorithm.h"
#include "vtkImagingCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkImageData;

class VTKIMAGINGCORE_EXPORT vtkImageChangeInformation : public vtkImageAlgorithm
{
public:
  static vtkImageChangeInformation* New();
  vtkTypeMacro(vtkImageChangeInformation, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Copy the information from another data set.  By default,
   * the information is copied from the input.
   */
  virtual void SetInformationInputData(vtkImageData*);
  virtual vtkImageData* GetInformationInput();
  ///@}

  ///@{
  /**
   * Specify new starting values for the extent explicitly.
   * These values are used as WholeExtent[0], WholeExtent[2] and
   * WholeExtent[4] of the output.  The default is to the
   * use the extent start of the Input, or of the InformationInput
   * if InformationInput is set.
   */
  vtkSetVector3Macro(OutputExtentStart, int);
  vtkGetVector3Macro(OutputExtentStart, int);
  ///@}

  ///@{
  /**
   * Specify a new data spacing explicitly.  The default is to
   * use the spacing of the Input, or of the InformationInput
   * if InformationInput is set.
   */
  vtkSetVector3Macro(OutputSpacing, double);
  vtkGetVector3Macro(OutputSpacing, double);
  ///@}

  ///@{
  /**
   * Specify a new direction matrix explicitly.  The default is to
   * use the direction of the Input, or of the InformationInput
   * if InformationInput is set.
   */
  vtkSetVectorMacro(OutputDirection, double, 9);
  vtkGetVectorMacro(OutputDirection, double, 9);
  ///@}

  ///@{
  /**
   * Specify a new data origin explicitly.  The default is to
   * use the origin of the Input, or of the InformationInput
   * if InformationInput is set.
   */
  vtkSetVector3Macro(OutputOrigin, double);
  vtkGetVector3Macro(OutputOrigin, double);
  ///@}

  ///@{
  /**
   * Set the Origin of the output so that image coordinate (0,0,0)
   * lies at the Center of the data set.  This will override
   * SetOutputOrigin.  This is often a useful operation to apply
   * before using vtkImageReslice to apply a transformation to an image.
   */
  vtkSetMacro(CenterImage, vtkTypeBool);
  vtkBooleanMacro(CenterImage, vtkTypeBool);
  vtkGetMacro(CenterImage, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Apply a translation to the extent.
   */
  vtkSetVector3Macro(ExtentTranslation, int);
  vtkGetVector3Macro(ExtentTranslation, int);
  ///@}

  ///@{
  /**
   * Apply a scale factor to the spacing.
   */
  vtkSetVector3Macro(SpacingScale, double);
  vtkGetVector3Macro(SpacingScale, double);
  ///@}

  ///@{
  /**
   * Apply a translation to the origin.
   */
  vtkSetVector3Macro(OriginTranslation, double);
  vtkGetVector3Macro(OriginTranslation, double);
  ///@}

  ///@{
  /**
   * Apply a scale to the origin.  The scale is applied
   * before the translation.
   */
  vtkSetVector3Macro(OriginScale, double);
  vtkGetVector3Macro(OriginScale, double);
  ///@}

protected:
  vtkImageChangeInformation();
  ~vtkImageChangeInformation() override;

  vtkTypeBool CenterImage;

  int OutputExtentStart[3];
  int ExtentTranslation[3];
  int FinalExtentTranslation[3];

  double OutputSpacing[3];
  double SpacingScale[3];

  double OutputDirection[9];

  double OutputOrigin[3];
  double OriginScale[3];
  double OriginTranslation[3];

  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkImageChangeInformation(const vtkImageChangeInformation&) = delete;
  void operator=(const vtkImageChangeInformation&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
