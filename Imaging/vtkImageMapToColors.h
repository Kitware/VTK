/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMapToColors.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
  vtkTypeMacro(vtkImageMapToColors,vtkImageToImageFilter);
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







