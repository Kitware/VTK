/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageResliceToColors.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageResliceToColors
 * @brief   Reslice and produce color scalars.
 *
 * vtkImageResliceToColors is an extension of vtkImageReslice that
 * produces color scalars.  It should be provided with a lookup table
 * that defines the output colors and the desired range of input values
 * to map to those colors.  If the input has multiple components, then
 * you should use the SetVectorMode() method of the lookup table to
 * specify how the vectors will be colored.  If no lookup table is
 * provided, then the input must already be color scalars, but they
 * will be converted to the specified output format.
 * @sa
 * vtkImageMapToColors
*/

#ifndef vtkImageResliceToColors_h
#define vtkImageResliceToColors_h


#include "vtkImagingCoreModule.h" // For export macro
#include "vtkImageReslice.h"

class vtkScalarsToColors;

class VTKIMAGINGCORE_EXPORT vtkImageResliceToColors : public vtkImageReslice
{
public:
  static vtkImageResliceToColors *New();
  vtkTypeMacro(vtkImageResliceToColors, vtkImageReslice);

  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Set a lookup table to apply to the data.  Use the Range,
   * VectorMode, and VectorComponents of the table to control
   * the mapping of the input data to colors.  If any output
   * voxel is transformed to a point outside the input volume,
   * then that voxel will be set to the BackgroundColor.
   */
  virtual void SetLookupTable(vtkScalarsToColors *table);
  vtkGetObjectMacro(LookupTable,vtkScalarsToColors);
  //@}

  //@{
  /**
   * Set the output format, the default is RGBA.
   */
  vtkSetClampMacro(OutputFormat,int,VTK_LUMINANCE,VTK_RGBA);
  vtkGetMacro(OutputFormat,int);
  void SetOutputFormatToRGBA() {
    this->OutputFormat = VTK_RGBA; };
  void SetOutputFormatToRGB() {
    this->OutputFormat = VTK_RGB; };
  void SetOutputFormatToLuminanceAlpha() {
    this->OutputFormat = VTK_LUMINANCE_ALPHA; };
  void SetOutputFormatToLuminance() {
    this->OutputFormat = VTK_LUMINANCE; };
  //@}

  /**
   * Bypass the color mapping operation and output the scalar
   * values directly.  The output values will be float, rather
   * than the input data type.
   */
  void SetBypass(int bypass);
  void BypassOn() { this->SetBypass(1); }
  void BypassOff() { this->SetBypass(0); }
  int GetBypass() { return this->Bypass; }

  /**
   * When determining the modified time of the filter,
   * this check the modified time of the transform and matrix.
   */
  vtkMTimeType GetMTime() VTK_OVERRIDE;

protected:
  vtkImageResliceToColors();
  ~vtkImageResliceToColors();

  vtkScalarsToColors *LookupTable;
  vtkScalarsToColors *DefaultLookupTable;
  int OutputFormat;
  int Bypass;

  int ConvertScalarInfo(int &scalarType, int &numComponents) VTK_OVERRIDE;

  void ConvertScalars(void *inPtr, void *outPtr, int inputType,
                      int inputNumComponents, int count,
                      int idX, int idY, int idZ, int threadId) VTK_OVERRIDE;

private:
  vtkImageResliceToColors(const vtkImageResliceToColors&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImageResliceToColors&) VTK_DELETE_FUNCTION;
};

#endif
