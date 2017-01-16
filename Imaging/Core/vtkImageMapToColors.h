/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMapToColors.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageMapToColors
 * @brief   map the input image through a lookup table
 *
 * The vtkImageMapToColors filter will take an input image of any valid
 * scalar type, and map the first component of the image through a
 * lookup table.  The result is an image of type VTK_UNSIGNED_CHAR.
 * If the lookup table is not set, or is set to NULL, then the input
 * data will be passed through if it is already of type VTK_UNSIGNED_CHAR.
 *
 * @sa
 * vtkLookupTable vtkScalarsToColors
*/

#ifndef vtkImageMapToColors_h
#define vtkImageMapToColors_h


#include "vtkImagingCoreModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

class vtkScalarsToColors;

class VTKIMAGINGCORE_EXPORT vtkImageMapToColors : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageMapToColors *New();
  vtkTypeMacro(vtkImageMapToColors,vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Set the lookup table.
   */
  virtual void SetLookupTable(vtkScalarsToColors*);
  vtkGetObjectMacro(LookupTable,vtkScalarsToColors);
  //@}

  //@{
  /**
   * Set the output format, the default is RGBA.
   */
  vtkSetMacro(OutputFormat,int);
  vtkGetMacro(OutputFormat,int);
  void SetOutputFormatToRGBA() { this->OutputFormat = VTK_RGBA; };
  void SetOutputFormatToRGB() { this->OutputFormat = VTK_RGB; };
  void SetOutputFormatToLuminanceAlpha() { this->OutputFormat = VTK_LUMINANCE_ALPHA; };
  void SetOutputFormatToLuminance() { this->OutputFormat = VTK_LUMINANCE; };
  //@}

  //@{
  /**
   * Set the component to map for multi-component images (default: 0)
   */
  vtkSetMacro(ActiveComponent,int);
  vtkGetMacro(ActiveComponent,int);
  //@}

  //@{
  /**
   * Use the alpha component of the input when computing the alpha component
   * of the output (useful when converting monochrome+alpha data to RGBA)
   */
  vtkSetMacro(PassAlphaToOutput,int);
  vtkBooleanMacro(PassAlphaToOutput,int);
  vtkGetMacro(PassAlphaToOutput,int);
  //@}

  /**
   * We need to check the modified time of the lookup table too.
   */
  vtkMTimeType GetMTime() VTK_OVERRIDE;

  //@{
  /**
   * Set/Get Color that should be used in case of UnMatching
   * data.
   */
  vtkSetVector4Macro(NaNColor, unsigned char);
  vtkGetVector4Macro(NaNColor, unsigned char);
  //@}

protected:
  vtkImageMapToColors();
  ~vtkImageMapToColors() VTK_OVERRIDE;

  int RequestInformation(vtkInformation *,
                                 vtkInformationVector **,
                                 vtkInformationVector *) VTK_OVERRIDE;

  void ThreadedRequestData(vtkInformation *request,
                           vtkInformationVector **inputVector,
                           vtkInformationVector *outputVector,
                           vtkImageData ***inData, vtkImageData **outData,
                           int extent[6], int id) VTK_OVERRIDE;

  int RequestData(vtkInformation *request,
                          vtkInformationVector **inputVector,
                          vtkInformationVector *outputVector) VTK_OVERRIDE;

  vtkScalarsToColors *LookupTable;
  int OutputFormat;

  int ActiveComponent;
  int PassAlphaToOutput;

  int DataWasPassed;

  unsigned char NaNColor[4];
private:
  vtkImageMapToColors(const vtkImageMapToColors&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImageMapToColors&) VTK_DELETE_FUNCTION;
};

#endif







