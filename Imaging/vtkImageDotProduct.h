/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDotProduct.h
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
// .NAME vtkImageDotProduct - Dot product of two vector images.
// .SECTION Description
// vtkImageDotProduct interprets the scalar components of two images
// as vectors and takes the dot product vector by vector (pixel by pixel).

#ifndef __vtkImageDotProduct_h
#define __vtkImageDotProduct_h



#include "vtkImageTwoInputFilter.h"

class VTK_IMAGING_EXPORT vtkImageDotProduct : public vtkImageTwoInputFilter
{
public:
  static vtkImageDotProduct *New();
  vtkTypeRevisionMacro(vtkImageDotProduct,vtkImageTwoInputFilter);

protected:
  vtkImageDotProduct() {};
  ~vtkImageDotProduct() {};

  void ExecuteInformation(vtkImageData **inDatas, vtkImageData *outData);
  void ExecuteInformation(){this->vtkImageTwoInputFilter::ExecuteInformation();};
  void ThreadedExecute(vtkImageData **inDatas, vtkImageData *outData,
                       int extent[6], int id);
private:
  vtkImageDotProduct(const vtkImageDotProduct&);  // Not implemented.
  void operator=(const vtkImageDotProduct&);  // Not implemented.
};

#endif



