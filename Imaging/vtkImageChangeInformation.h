/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageChangeInformation.h
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
// .NAME vtkImageChangeInformation - modify spacing, origin and extent.
// .SECTION Description
// vtkImageChangeInformation  modify the spacing, origin, or extent of
// the data without changing the data itself.  The data is not resampled
// by this filter, only the information accompanying the data is modified.

#ifndef __vtkImageChangeInformation_h
#define __vtkImageChangeInformation_h

#include "vtkImageToImageFilter.h"

class VTK_EXPORT vtkImageChangeInformation : public vtkImageToImageFilter
{
public:
  static vtkImageChangeInformation *New();
  vtkTypeMacro(vtkImageChangeInformation, vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Copy the information from another data set.  By default,
  // the information is copied from the input.
  vtkSetObjectMacro(InformationInput, vtkImageData);
  vtkGetObjectMacro(InformationInput, vtkImageData);

  // Description:
  // Specify new starting values for the extent explicitly.
  // These values are used as WholeExtent[0], WholeExtent[2] and
  // WholeExtent[4] of the output.  The default is to the
  // use the extent start of the Input, or of the InformationInput
  // if InformationInput is set.
  vtkSetVector3Macro(OutputExtentStart, int);
  vtkGetVector3Macro(OutputExtentStart, int);

  // Description:
  // Specify a new data spacing explicitly.  The default is to
  // use the spacing of the Input, or of the InformationInput
  // if InformationInput is set.
  vtkSetVector3Macro(OutputSpacing, float);
  vtkGetVector3Macro(OutputSpacing, float);

  // Description:
  // Specify a new data origin explicitly.  The default is to
  // use the origin of the Input, or of the InformationInput
  // if InformationInput is set.
  vtkSetVector3Macro(OutputOrigin, float);
  vtkGetVector3Macro(OutputOrigin, float);

  // Description:
  // Set the Origin of the output so that image coordinate (0,0,0)
  // lies at the Center of the data set.  This will override 
  // SetOutputOrigin.  This is often a useful operation to apply 
  // before using vtkImageReslice to apply a transformation to an image. 
  vtkSetMacro(CenterImage, int);
  vtkBooleanMacro(CenterImage, int);
  vtkGetMacro(CenterImage, int);

  // Description:
  // Apply a translation to the extent.
  vtkSetVector3Macro(ExtentTranslation, int);
  vtkGetVector3Macro(ExtentTranslation, int);

  // Description:
  // Apply a scale factor to the spacing. 
  vtkSetVector3Macro(SpacingScale, float);
  vtkGetVector3Macro(SpacingScale, float);

  // Description:
  // Apply a translation to the origin.
  vtkSetVector3Macro(OriginTranslation, float);
  vtkGetVector3Macro(OriginTranslation, float);

  // Description:
  // Apply a scale to the origin.  The scale is applied
  // before the translation.
  vtkSetVector3Macro(OriginScale, float);
  vtkGetVector3Macro(OriginScale, float);

  // Description:
  // This method simply copies by reference the input data to the output.
  void UpdateData(vtkDataObject *data);

protected:
  vtkImageChangeInformation();
  ~vtkImageChangeInformation();
  vtkImageChangeInformation(const vtkImageChangeInformation&) {};
  void operator=(const vtkImageChangeInformation&) {};

  vtkImageData *InformationInput;
  int CenterImage;

  int OutputExtentStart[3];
  int ExtentTranslation[3];
  int FinalExtentTranslation[3];
  
  float OutputSpacing[3];
  float SpacingScale[3];

  float OutputOrigin[3];
  float OriginScale[3];
  float OriginTranslation[3];
  
  void ComputeInputUpdateExtent(int extent[6], int wholeExtent[6]);
  void ExecuteInformation(vtkImageData *inData, vtkImageData *outData);
  void ExecuteInformation() {
    this->vtkImageToImageFilter::ExecuteInformation(); };
};



#endif



