/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMapToColors.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageMapToColors - map the input image through a lookup table
// .SECTION Description
// The vtkImageMapToColors filter will take an input image of any valid
// scalar type, and map the first component of the image through a
// lookup table.  The result is an image of type VTK_UNSIGNED_CHAR.
// If the lookup table is not set, or is set to NULL, then the input
// data will be passed through if it is already of type UNSIGNED_CHAR.

// .SECTION See Also
// vtkLookupTable vtkScalarsToColors

#ifndef __vtkImageMapToColors_h
#define __vtkImageMapToColors_h


#include "vtkImageToImageFilter.h"
#include "vtkScalarsToColors.h"

class VTK_IMAGING_EXPORT vtkImageMapToColors : public vtkImageToImageFilter
{
public:
  static vtkImageMapToColors *New();
  vtkTypeRevisionMacro(vtkImageMapToColors,vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the lookup table.
  vtkSetObjectMacro(LookupTable,vtkScalarsToColors);
  vtkGetObjectMacro(LookupTable,vtkScalarsToColors);

  // Description:
  // Set the output format, the default is RGBA.  
  vtkSetMacro(OutputFormat,int);
  vtkGetMacro(OutputFormat,int);
  void SetOutputFormatToRGBA() { this->OutputFormat = VTK_RGBA; };
  void SetOutputFormatToRGB() { this->OutputFormat = VTK_RGB; };
  void SetOutputFormatToLuminanceAlpha() { this->OutputFormat = VTK_LUMINANCE_ALPHA; };
  void SetOutputFormatToLuminance() { this->OutputFormat = VTK_LUMINANCE; };

  // Description:
  // Set the component to map for multi-component images (default: 0)
  vtkSetMacro(ActiveComponent,int);
  vtkGetMacro(ActiveComponent,int);

  // Description:
  // Use the alpha component of the input when computing the alpha component
  // of the output (useful when converting monochrome+alpha data to RGBA)
  vtkSetMacro(PassAlphaToOutput,int);
  vtkBooleanMacro(PassAlphaToOutput,int);
  vtkGetMacro(PassAlphaToOutput,int);

  // Description:
  // We need to check the modified time of the lookup table too.
  unsigned long GetMTime();

protected:
  vtkImageMapToColors();
  ~vtkImageMapToColors();

  void ExecuteInformation(vtkImageData *inData, vtkImageData *outData);
  void ExecuteInformation() {
    this->vtkImageToImageFilter::ExecuteInformation(); };
  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData,
                       int extent[6], int id);
  
  void ExecuteData(vtkDataObject *output);

  vtkScalarsToColors *LookupTable;
  int OutputFormat;
  
  int ActiveComponent;
  int PassAlphaToOutput;

  int DataWasPassed;
private:
  vtkImageMapToColors(const vtkImageMapToColors&);  // Not implemented.
  void operator=(const vtkImageMapToColors&);  // Not implemented.
};

#endif







