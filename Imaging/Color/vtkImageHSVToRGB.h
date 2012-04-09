/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageHSVToRGB.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageHSVToRGB - Converts HSV components to RGB.
// .SECTION Description
// For each pixel with hue, saturation and value components this filter
// outputs the color coded as red, green, blue.  Output type must be the same
// as input type.

// .SECTION See Also
// vtkImageRGBToHSV

#ifndef __vtkImageHSVToRGB_h
#define __vtkImageHSVToRGB_h


#include "vtkThreadedImageAlgorithm.h"

class VTK_IMAGING_EXPORT vtkImageHSVToRGB : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageHSVToRGB *New();
  vtkTypeMacro(vtkImageHSVToRGB,vtkThreadedImageAlgorithm);

  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Hue is an angle. Maximum specifies when it maps back to 0.
  // HueMaximum defaults to 255 instead of 2PI, because unsigned char
  // is expected as input.
  // Maximum also specifies the maximum of the Saturation, and R, G, B.
  vtkSetMacro(Maximum,double);
  vtkGetMacro(Maximum,double);
  
protected:
  vtkImageHSVToRGB();
  ~vtkImageHSVToRGB() {};

  double Maximum;
  
  void ThreadedExecute (vtkImageData *inData, vtkImageData *outData,
                       int ext[6], int id);
private:
  vtkImageHSVToRGB(const vtkImageHSVToRGB&);  // Not implemented.
  void operator=(const vtkImageHSVToRGB&);  // Not implemented.
};

#endif



