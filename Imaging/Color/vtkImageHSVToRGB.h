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
/**
 * @class   vtkImageHSVToRGB
 * @brief   Converts HSV components to RGB.
 *
 * For each pixel with hue, saturation and value components this filter
 * outputs the color coded as red, green, blue.  Output type must be the same
 * as input type.
 *
 * @sa
 * vtkImageRGBToHSV
*/

#ifndef vtkImageHSVToRGB_h
#define vtkImageHSVToRGB_h


#include "vtkImagingColorModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

class VTKIMAGINGCOLOR_EXPORT vtkImageHSVToRGB : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageHSVToRGB *New();
  vtkTypeMacro(vtkImageHSVToRGB,vtkThreadedImageAlgorithm);

  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Hue is an angle. Maximum specifies when it maps back to 0.
   * HueMaximum defaults to 255 instead of 2PI, because unsigned char
   * is expected as input.
   * Maximum also specifies the maximum of the Saturation, and R, G, B.
   */
  vtkSetMacro(Maximum,double);
  vtkGetMacro(Maximum,double);
  //@}

protected:
  vtkImageHSVToRGB();
  ~vtkImageHSVToRGB() {}

  double Maximum;

  void ThreadedExecute (vtkImageData *inData, vtkImageData *outData,
                       int ext[6], int id);
private:
  vtkImageHSVToRGB(const vtkImageHSVToRGB&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImageHSVToRGB&) VTK_DELETE_FUNCTION;
};

#endif



