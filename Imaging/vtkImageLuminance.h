/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageLuminance.h
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
// .NAME vtkImageLuminance - Computes the luminance of the input
// .SECTION Description
// vtkImageLuminance calculates luminance from an rgb input.

#ifndef __vtkImageLuminance_h
#define __vtkImageLuminance_h


#include "vtkImageToImageFilter.h"

class VTK_IMAGING_EXPORT vtkImageLuminance : public vtkImageToImageFilter
{
public:
  static vtkImageLuminance *New();
  vtkTypeRevisionMacro(vtkImageLuminance,vtkImageToImageFilter);

protected:
  vtkImageLuminance() {};
  ~vtkImageLuminance() {};
  
  void ExecuteInformation(vtkImageData *inData, vtkImageData *outData);
  void ExecuteInformation(){this->vtkImageToImageFilter::ExecuteInformation();};
  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData, 
                       int ext[6], int id);
private:
  vtkImageLuminance(const vtkImageLuminance&);  // Not implemented.
  void operator=(const vtkImageLuminance&);  // Not implemented.
};

#endif










