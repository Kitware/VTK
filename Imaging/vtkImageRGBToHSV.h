/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageRGBToHSV.h
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
// .NAME vtkImageRGBToHSV - Converts RGB components to HSV.
// .SECTION Description
// For each pixel with red, blue, and green components this
// filter output the color coded as hue, saturation and value.
// Output type must be the same as input type.


#ifndef __vtkImageRGBToHSV_h
#define __vtkImageRGBToHSV_h


#include "vtkImageToImageFilter.h"

class VTK_IMAGING_EXPORT vtkImageRGBToHSV : public vtkImageToImageFilter
{
public:
  static vtkImageRGBToHSV *New();
  vtkTypeRevisionMacro(vtkImageRGBToHSV,vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:

  // Hue is an angle. Maximum specifies when it maps back to 0.  HueMaximum
  // defaults to 255 instead of 2PI, because unsigned char is expected as
  // input.  Maximum also specifies the maximum of the Saturation.
  vtkSetMacro(Maximum,float);
  vtkGetMacro(Maximum,float);
  
protected:
  vtkImageRGBToHSV();
  ~vtkImageRGBToHSV() {};

  float Maximum;
  
  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData,
                       int ext[6], int id);
private:
  vtkImageRGBToHSV(const vtkImageRGBToHSV&);  // Not implemented.
  void operator=(const vtkImageRGBToHSV&);  // Not implemented.
};

#endif



