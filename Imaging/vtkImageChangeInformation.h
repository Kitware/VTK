/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageChangeInformation.h
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
// .NAME vtkImageChangeInformation - modify spacing, origin and extent.
// .SECTION Description
// vtkImageChangeInformation  modify the spacing, origin, or extent of
// the data without changing the data itself.  The data is not resampled
// by this filter, only the information accompanying the data is modified.

#ifndef __vtkImageChangeInformation_h
#define __vtkImageChangeInformation_h

#include "vtkImageToImageFilter.h"

class VTK_IMAGING_EXPORT vtkImageChangeInformation : public vtkImageToImageFilter
{
public:
  static vtkImageChangeInformation *New();
  vtkTypeRevisionMacro(vtkImageChangeInformation, vtkImageToImageFilter);
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

protected:
  vtkImageChangeInformation();
  ~vtkImageChangeInformation();

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
  void ExecuteData(vtkDataObject *data);
private:
  vtkImageChangeInformation(const vtkImageChangeInformation&) {};  // Not implemented.
  void operator=(const vtkImageChangeInformation&) {};  // Not implemented.
};



#endif



