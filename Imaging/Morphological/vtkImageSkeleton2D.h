/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSkeleton2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageSkeleton2D - Skeleton of 2D images.
// .SECTION Description
// vtkImageSkeleton2D should leave only single pixel width lines
// of non-zero-valued pixels (values of 1 are not allowed).
// It works by erosion on a 3x3 neighborhood with special rules.
// The number of iterations determines how far the filter can erode.
// There are three pruning levels:
//  prune == 0 will leave traces on all angles...
//  prune == 1 will not leave traces on 135 degree angles, but will on 90.
//  prune == 2 does not leave traces on any angles leaving only closed loops.
// Prune defaults to zero. The output scalar type is the same as the input.



#ifndef __vtkImageSkeleton2D_h
#define __vtkImageSkeleton2D_h

#include "vtkImagingMorphologicalModule.h" // For export macro
#include "vtkImageIterateFilter.h"

class VTKIMAGINGMORPHOLOGICAL_EXPORT vtkImageSkeleton2D : public vtkImageIterateFilter
{
public:
  static vtkImageSkeleton2D *New();
  vtkTypeMacro(vtkImageSkeleton2D,vtkImageIterateFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // When prune is on, only closed loops are left unchanged.
  vtkSetMacro(Prune,int);
  vtkGetMacro(Prune,int);
  vtkBooleanMacro(Prune,int);

  // Description:
  // Sets the number of cycles in the erosion.
  void SetNumberOfIterations(int num);

protected:
  vtkImageSkeleton2D();
  ~vtkImageSkeleton2D() {};

  int Prune;

  virtual int IterativeRequestUpdateExtent(vtkInformation* in,
                                           vtkInformation* out);
  void ThreadedRequestData(
    vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector,
    vtkImageData ***inDataV,
    vtkImageData **outDataV,
    int outExt[6],
    int id);
private:
  vtkImageSkeleton2D(const vtkImageSkeleton2D&);  // Not implemented.
  void operator=(const vtkImageSkeleton2D&);  // Not implemented.
};

#endif



