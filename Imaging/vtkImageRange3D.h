/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageRange3D.h
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
// .NAME vtkImageRange3D - Max - min of a circular neighborhood.
// .SECTION Description
// vtkImageRange3D replaces a pixel with the maximum minus minimum over
// an ellipsoidal neighborhood.  If KernelSize of an axis is 1, no processing
// is done on that axis.


#ifndef __vtkImageRange3D_h
#define __vtkImageRange3D_h


#include "vtkImageSpatialFilter.h"

class vtkImageEllipsoidSource;

class VTK_IMAGING_EXPORT vtkImageRange3D : public vtkImageSpatialFilter
{
public:
  static vtkImageRange3D *New();
  vtkTypeRevisionMacro(vtkImageRange3D,vtkImageSpatialFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // This method sets the size of the neighborhood.  It also sets the 
  // default middle of the neighborhood and computes the elliptical foot print.
  void SetKernelSize(int size0, int size1, int size2);
  
protected:
  vtkImageRange3D();
  ~vtkImageRange3D();

  vtkImageEllipsoidSource *Ellipse;
    
  void ExecuteInformation(vtkImageData *inData, vtkImageData *outData);
  void ExecuteInformation(){this->vtkImageToImageFilter::ExecuteInformation();};
  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData, 
                       int extent[6], int id);
private:
  vtkImageRange3D(const vtkImageRange3D&);  // Not implemented.
  void operator=(const vtkImageRange3D&);  // Not implemented.
};

#endif



