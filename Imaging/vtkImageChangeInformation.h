/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageChangeInformation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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

class vtkImageData;

class VTK_IMAGING_EXPORT vtkImageChangeInformation : public vtkImageToImageFilter
{
public:
  static vtkImageChangeInformation *New();
  vtkTypeRevisionMacro(vtkImageChangeInformation, vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Copy the information from another data set.  By default,
  // the information is copied from the input.
  virtual void SetInformationInput(vtkImageData*);
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
  vtkSetVector3Macro(OutputSpacing, double);
  vtkGetVector3Macro(OutputSpacing, double);

  // Description:
  // Specify a new data origin explicitly.  The default is to
  // use the origin of the Input, or of the InformationInput
  // if InformationInput is set.
  vtkSetVector3Macro(OutputOrigin, double);
  vtkGetVector3Macro(OutputOrigin, double);

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
  vtkSetVector3Macro(SpacingScale, double);
  vtkGetVector3Macro(SpacingScale, double);

  // Description:
  // Apply a translation to the origin.
  vtkSetVector3Macro(OriginTranslation, double);
  vtkGetVector3Macro(OriginTranslation, double);

  // Description:
  // Apply a scale to the origin.  The scale is applied
  // before the translation.
  vtkSetVector3Macro(OriginScale, double);
  vtkGetVector3Macro(OriginScale, double);

protected:
  vtkImageChangeInformation();
  ~vtkImageChangeInformation();

  vtkImageData *InformationInput;
  int CenterImage;

  int OutputExtentStart[3];
  int ExtentTranslation[3];
  int FinalExtentTranslation[3];
  
  double OutputSpacing[3];
  double SpacingScale[3];

  double OutputOrigin[3];
  double OriginScale[3];
  double OriginTranslation[3];
  
  void ComputeInputUpdateExtent(int extent[6], int wholeExtent[6]);
  void ExecuteInformation(vtkImageData *inData, vtkImageData *outData);
  void ExecuteInformation() {
    this->vtkImageToImageFilter::ExecuteInformation(); };
  void ExecuteData(vtkDataObject *data);
private:
  vtkImageChangeInformation(const vtkImageChangeInformation&);  // Not implemented.
  void operator=(const vtkImageChangeInformation&);  // Not implemented.
};



#endif



