/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageHybridMedian2D.h
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
// .NAME vtkImageHybridMedian2D - Median filter that preserves lines and 
// corners.
// .SECTION Description
// vtkImageHybridMedian2D is a median filter that preserves thin lines and
// corners.  It operates on a 5x5 pixel neighborhood.  It computes two values
// initially: the median of the + neighbors and the median of the x
// neighbors.  It the computes the median of these two values plus the center
// pixel.  This result of this second median is the output pixel value.



#ifndef __vtkImageHybridMedian2D_h
#define __vtkImageHybridMedian2D_h


#include "vtkImageSpatialFilter.h"

class VTK_IMAGING_EXPORT vtkImageHybridMedian2D : public vtkImageSpatialFilter
{
public:
  static vtkImageHybridMedian2D *New();
  vtkTypeRevisionMacro(vtkImageHybridMedian2D,vtkImageSpatialFilter);

protected:
  vtkImageHybridMedian2D();
  ~vtkImageHybridMedian2D() {};

  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData,
                       int outExt[6], int id);
  float ComputeMedian(float *array, int size);
private:
  vtkImageHybridMedian2D(const vtkImageHybridMedian2D&);  // Not implemented.
  void operator=(const vtkImageHybridMedian2D&);  // Not implemented.
};

#endif



