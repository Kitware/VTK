/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMagnitude.h
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
// .NAME vtkImageMagnitude - Colapses components with magnitude function..
// .SECTION Description
// vtkImageMagnitude takes the magnitude of the components.


#ifndef __vtkImageMagnitude_h
#define __vtkImageMagnitude_h


#include "vtkImageToImageFilter.h"

class VTK_IMAGING_EXPORT vtkImageMagnitude : public vtkImageToImageFilter
{
public:
  static vtkImageMagnitude *New();
  vtkTypeRevisionMacro(vtkImageMagnitude,vtkImageToImageFilter);

protected:
  vtkImageMagnitude() {};
  ~vtkImageMagnitude() {};

  void ExecuteInformation(vtkImageData *inData, vtkImageData *outData);
  void ExecuteInformation(){this->vtkImageToImageFilter::ExecuteInformation();};
  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData,
                       int extent[6], int id);
private:
  vtkImageMagnitude(const vtkImageMagnitude&);  // Not implemented.
  void operator=(const vtkImageMagnitude&);  // Not implemented.
};

#endif










