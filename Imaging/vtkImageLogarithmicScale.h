/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageLogarithmicScale.h
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
// .NAME vtkImageLogarithmicScale - Passes each pixel through log function.
// .SECTION Description
// vtkImageLogarithmicScale passes each pixel through the function
// c*log(1+x).  It also handles negative values with the function
// -c*log(1-x).



#ifndef __vtkImageLogarithmicScale_h
#define __vtkImageLogarithmicScale_h


#include "vtkImageToImageFilter.h"

class VTK_IMAGING_EXPORT vtkImageLogarithmicScale : public vtkImageToImageFilter
{
public:
  static vtkImageLogarithmicScale *New();
  vtkTypeRevisionMacro(vtkImageLogarithmicScale,vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the scale factor for the logarithmic function.
  vtkSetMacro(Constant,float);
  vtkGetMacro(Constant,float);
  
protected:
  vtkImageLogarithmicScale();
  ~vtkImageLogarithmicScale() {};

  float Constant;
  
  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData,
                       int outExt[6], int id);
private:
  vtkImageLogarithmicScale(const vtkImageLogarithmicScale&);  // Not implemented.
  void operator=(const vtkImageLogarithmicScale&);  // Not implemented.
};

#endif



