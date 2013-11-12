/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageRGBToHSV.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageRGBToHSV - Converts RGB components to HSV.
// .SECTION Description
// For each pixel with red, blue, and green components this
// filter output the color coded as hue, saturation and value.
// Output type must be the same as input type.


#ifndef __vtkImageRGBToHSV_h
#define __vtkImageRGBToHSV_h


#include "vtkImagingColorModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

class VTKIMAGINGCOLOR_EXPORT vtkImageRGBToHSV : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageRGBToHSV *New();
  vtkTypeMacro(vtkImageRGBToHSV,vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:

  // Hue is an angle. Maximum specifies when it maps back to 0.  HueMaximum
  // defaults to 255 instead of 2PI, because unsigned char is expected as
  // input.  Maximum also specifies the maximum of the Saturation.
  vtkSetMacro(Maximum,double);
  vtkGetMacro(Maximum,double);

protected:
  vtkImageRGBToHSV();
  ~vtkImageRGBToHSV() {}

  double Maximum;

  void ThreadedExecute (vtkImageData *inData, vtkImageData *outData,
                       int ext[6], int id);
private:
  vtkImageRGBToHSV(const vtkImageRGBToHSV&);  // Not implemented.
  void operator=(const vtkImageRGBToHSV&);  // Not implemented.
};

#endif



