// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageMapToColors
 * @brief   map the input image through a lookup table
 *
 * The vtkImageMapToColors filter will take an input image of any valid
 * scalar type, and map the first component of the image through a
 * lookup table.  The result is an image of type VTK_UNSIGNED_CHAR.
 * If the lookup table is not set, or is set to nullptr, then the input
 * data will be passed through if it is already of type VTK_UNSIGNED_CHAR.
 *
 * @sa
 * vtkLookupTable vtkScalarsToColors
 */

#ifndef vtkImageMapToColors_h
#define vtkImageMapToColors_h

#include "vtkImagingCoreModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkScalarsToColors;

class VTKIMAGINGCORE_EXPORT VTK_MARSHALAUTO vtkImageMapToColors : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageMapToColors* New();
  vtkTypeMacro(vtkImageMapToColors, vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set the lookup table.
   */
  virtual void SetLookupTable(vtkScalarsToColors*);
  vtkGetObjectMacro(LookupTable, vtkScalarsToColors);
  ///@}

  ///@{
  /**
   * Set the output format, the default is RGBA.
   */
  vtkSetMacro(OutputFormat, int);
  vtkGetMacro(OutputFormat, int);
  void SetOutputFormatToRGBA() { this->OutputFormat = VTK_RGBA; }
  void SetOutputFormatToRGB() { this->OutputFormat = VTK_RGB; }
  void SetOutputFormatToLuminanceAlpha() { this->OutputFormat = VTK_LUMINANCE_ALPHA; }
  void SetOutputFormatToLuminance() { this->OutputFormat = VTK_LUMINANCE; }
  ///@}

  ///@{
  /**
   * Set the component to map for multi-component images (default: 0)
   */
  vtkSetMacro(ActiveComponent, int);
  vtkGetMacro(ActiveComponent, int);
  ///@}

  ///@{
  /**
   * Use the alpha component of the input when computing the alpha component
   * of the output (useful when converting monochrome+alpha data to RGBA)
   */
  vtkSetMacro(PassAlphaToOutput, vtkTypeBool);
  vtkBooleanMacro(PassAlphaToOutput, vtkTypeBool);
  vtkGetMacro(PassAlphaToOutput, vtkTypeBool);
  ///@}

  /**
   * We need to check the modified time of the lookup table too.
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Set/Get Color that should be used in case of UnMatching
   * data.
   */
  vtkSetVector4Macro(NaNColor, unsigned char);
  vtkGetVector4Macro(NaNColor, unsigned char);
  ///@}

protected:
  vtkImageMapToColors();
  ~vtkImageMapToColors() override;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  void ThreadedRequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector, vtkImageData*** inData, vtkImageData** outData,
    int outExt[6], int id) override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  vtkScalarsToColors* LookupTable;
  int OutputFormat;

  int ActiveComponent;
  vtkTypeBool PassAlphaToOutput;

  int DataWasPassed;

  unsigned char NaNColor[4];

private:
  vtkImageMapToColors(const vtkImageMapToColors&) = delete;
  void operator=(const vtkImageMapToColors&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
