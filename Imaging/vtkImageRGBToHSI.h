/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageRGBToHSI.h
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
// .NAME vtkImageRGBToHSI - Converts RGB components to HSI.
// .SECTION Description
// For each pixel with red, blue, and green components this
// filter output the color coded as hue, saturation and intensity.
// Output type must be the same as input type.

#ifndef __vtkImageRGBToHSI_h
#define __vtkImageRGBToHSI_h

#include "vtkImageToImageFilter.h"

class VTK_IMAGING_EXPORT vtkImageRGBToHSI : public vtkImageToImageFilter
{
public:
  static vtkImageRGBToHSI *New();
  vtkTypeRevisionMacro(vtkImageRGBToHSI,vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Hue is an angle. Maximum specifies when it maps back to 0.  HueMaximum
  // defaults to 255 instead of 2PI, because unsigned char is expected as
  // input.  Maximum also specifies the maximum of the Saturation.
  vtkSetMacro(Maximum,float);
  vtkGetMacro(Maximum,float);
  
protected:
  vtkImageRGBToHSI();
  ~vtkImageRGBToHSI() {};

  float Maximum;
  
  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData,
                       int ext[6], int id);
private:
  vtkImageRGBToHSI(const vtkImageRGBToHSI&);  // Not implemented.
  void operator=(const vtkImageRGBToHSI&);  // Not implemented.
};

#endif



