/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageNormalize.h
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
// .NAME vtkImageNormalize - Normalizes that scalar components for each point.
// .SECTION Description
// For each point, vtkImageNormalize normalizes the vector defined by the 
// scalar components.  If the magnitude of this vector is zero, the output
// vector is zero also.


#ifndef __vtkImageNormalize_h
#define __vtkImageNormalize_h


#include "vtkImageToImageFilter.h"

class VTK_IMAGING_EXPORT vtkImageNormalize : public vtkImageToImageFilter
{
public:
  static vtkImageNormalize *New();
  vtkTypeRevisionMacro(vtkImageNormalize,vtkImageToImageFilter);

protected:
  vtkImageNormalize() {};
  ~vtkImageNormalize() {};

  void ExecuteInformation(vtkImageData *inData, vtkImageData *outData);
  void ExecuteInformation(){this->vtkImageToImageFilter::ExecuteInformation();};
  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData,
                       int extent[6], int id);
private:
  vtkImageNormalize(const vtkImageNormalize&);  // Not implemented.
  void operator=(const vtkImageNormalize&);  // Not implemented.
};

#endif










