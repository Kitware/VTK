/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageContinuousErode3D.h
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
// .NAME vtkImageContinuousErode3D - Erosion implemented as a minimum.
// .SECTION Description
// vtkImageContinuousErode3D replaces a pixel with the minimum over
// an ellipsoidal neighborhood.  If KernelSize of an axis is 1, no processing
// is done on that axis.


#ifndef __vtkImageContinuousErode3D_h
#define __vtkImageContinuousErode3D_h


#include "vtkImageSpatialFilter.h"

class vtkImageEllipsoidSource;

class VTK_IMAGING_EXPORT vtkImageContinuousErode3D : public vtkImageSpatialFilter
{
public:
  // Description:
  // Construct an instance of vtkImageContinuousErode3D filter.
  // By default zero values are eroded.
  static vtkImageContinuousErode3D *New();
  vtkTypeRevisionMacro(vtkImageContinuousErode3D,vtkImageSpatialFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // This method sets the size of the neighborhood.  It also sets the 
  // default middle of the neighborhood and computes the elliptical foot print.
  void SetKernelSize(int size0, int size1, int size2);

  
protected:
  vtkImageContinuousErode3D();
  ~vtkImageContinuousErode3D();

  vtkImageEllipsoidSource *Ellipse;
    
  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData, 
                       int extent[6], int id);
private:
  vtkImageContinuousErode3D(const vtkImageContinuousErode3D&);  // Not implemented.
  void operator=(const vtkImageContinuousErode3D&);  // Not implemented.
};

#endif



