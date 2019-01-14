/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageHSIToRGB.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageHSIToRGB
 * @brief   Converts HSI components to RGB.
 *
 * For each pixel with hue, saturation and intensity components this filter
 * outputs the color coded as red, green, blue.  Output type must be the same
 * as input type.
 *
 * @sa
 * vtkImageRGBToHSI
*/

#ifndef vtkImageHSIToRGB_h
#define vtkImageHSIToRGB_h

#include "vtkImagingColorModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

class VTKIMAGINGCOLOR_EXPORT vtkImageHSIToRGB : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageHSIToRGB *New();
  vtkTypeMacro(vtkImageHSIToRGB,vtkThreadedImageAlgorithm);

  void PrintSelf(ostream& os, vtkIndent indent) override;

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
  vtkImageHSIToRGB();
  ~vtkImageHSIToRGB() override {}

  double Maximum;

  void ThreadedExecute (vtkImageData *inData, vtkImageData *outData,
                       int ext[6], int id) override;
private:
  vtkImageHSIToRGB(const vtkImageHSIToRGB&) = delete;
  void operator=(const vtkImageHSIToRGB&) = delete;
};

#endif



