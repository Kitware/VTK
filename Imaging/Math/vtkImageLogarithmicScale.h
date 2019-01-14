/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageLogarithmicScale.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageLogarithmicScale
 * @brief   Passes each pixel through log function.
 *
 * vtkImageLogarithmicScale passes each pixel through the function
 * c*log(1+x).  It also handles negative values with the function
 * -c*log(1-x).
*/

#ifndef vtkImageLogarithmicScale_h
#define vtkImageLogarithmicScale_h


#include "vtkImagingMathModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

class VTKIMAGINGMATH_EXPORT vtkImageLogarithmicScale : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageLogarithmicScale *New();
  vtkTypeMacro(vtkImageLogarithmicScale,vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set/Get the scale factor for the logarithmic function.
   */
  vtkSetMacro(Constant,double);
  vtkGetMacro(Constant,double);
  //@}

protected:
  vtkImageLogarithmicScale();
  ~vtkImageLogarithmicScale() override {}

  double Constant;

  void ThreadedExecute (vtkImageData *inData, vtkImageData *outData,
                       int outExt[6], int id) override;
private:
  vtkImageLogarithmicScale(const vtkImageLogarithmicScale&) = delete;
  void operator=(const vtkImageLogarithmicScale&) = delete;
};

#endif



