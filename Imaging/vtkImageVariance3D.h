/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageVariance3D.h
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
// .NAME vtkImageVariance3D - Variance in a neighborhood.
// .SECTION Description
// vtkImageVariance3D replaces each pixel with a measurement of 
// pixel variance in a elliptical neighborhood centered on that pixel.
// The value computed is not exactly the variance.
// The difference between the neighbor values and center value is computed
// and squared for each neighbor.  These values are summed and divided by
// the total number of neighbors to produce the output value.


#ifndef __vtkImageVariance3D_h
#define __vtkImageVariance3D_h


#include "vtkImageSpatialFilter.h"

class vtkImageEllipsoidSource;

class VTK_IMAGING_EXPORT vtkImageVariance3D : public vtkImageSpatialFilter
{
public:
  static vtkImageVariance3D *New();
  vtkTypeRevisionMacro(vtkImageVariance3D,vtkImageSpatialFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // This method sets the size of the neighborhood.  It also sets the default
  // middle of the neighborhood and computes the Elliptical foot print.
  void SetKernelSize(int size0, int size1, int size2);
  
protected:
  vtkImageVariance3D();
  ~vtkImageVariance3D();

  vtkImageEllipsoidSource *Ellipse;
    
  void ExecuteInformation(vtkImageData *inData, vtkImageData *outData);
  void ExecuteInformation(){this->vtkImageToImageFilter::ExecuteInformation();};
  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData, 
                       int extent[6], int id);
private:
  vtkImageVariance3D(const vtkImageVariance3D&);  // Not implemented.
  void operator=(const vtkImageVariance3D&);  // Not implemented.
};

#endif



