/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageRGBToYIQ.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageRGBToYIQ
 * @brief   Converts RGB components to YIQ.
 *
 * For each pixel with red, blue, and green components this
 * filter output the color coded as YIQ.
 * Output type must be the same as input type.
*/

#ifndef vtkImageRGBToYIQ_h
#define vtkImageRGBToYIQ_h


#include "vtkImagingColorModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

class VTKIMAGINGCOLOR_EXPORT vtkImageRGBToYIQ : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageRGBToYIQ *New();
  vtkTypeMacro(vtkImageRGBToYIQ,vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  vtkSetMacro(Maximum,double);
  vtkGetMacro(Maximum,double);

protected:
  vtkImageRGBToYIQ();
  ~vtkImageRGBToYIQ() VTK_OVERRIDE {}

  double Maximum;      // Maximum value of pixel intensity allowed

  void ThreadedExecute (vtkImageData *inData, vtkImageData *outData,
                       int ext[6], int id) VTK_OVERRIDE;
private:
  vtkImageRGBToYIQ(const vtkImageRGBToYIQ&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImageRGBToYIQ&) VTK_DELETE_FUNCTION;
};

#endif
