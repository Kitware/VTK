/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSpatialFilter.h
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
// .NAME vtkImageSpatialFilter - Filters that operate on pixel neighborhoods.
// .SECTION Description
// vtkImageSpatialFilter is a super class for filters that operate on an
// input neighborhood for each output pixel. It handles even sized
// neighborhoods, but their can be a half pixel shift associated with
// processing.  This superclass has some logic for handling boundaries.  It
// can split regions into boundary and non-boundary pieces and call different
// execute methods.


#ifndef __vtkImageSpatialFilter_h
#define __vtkImageSpatialFilter_h


#include "vtkImageToImageFilter.h"

class VTK_IMAGING_EXPORT vtkImageSpatialFilter : public vtkImageToImageFilter
{
public:
  static vtkImageSpatialFilter *New();
  vtkTypeRevisionMacro(vtkImageSpatialFilter,vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the Kernel size.
  int *GetKernelSize() {return this->KernelSize;}
  
  // Description:
  // Get the Kernel middle.
  int *GetKernelMiddle() {return this->KernelMiddle;}

protected:
  vtkImageSpatialFilter();
  ~vtkImageSpatialFilter() {};

  int   KernelSize[3];
  int   KernelMiddle[3];      // Index of kernel origin
  int   Strides[3];      // Shrink factor
  int   HandleBoundaries;     // Output shrinks if boundaries aren't handled

  // Called by the superclass
  void ExecuteInformation();
  // Override this method if you have to.
  virtual void ExecuteInformation(vtkImageData *inData, vtkImageData *outData);

  void ComputeOutputWholeExtent(int extent[6], int handleBoundaries);
  void ComputeInputUpdateExtent(int extent[6], int wholeExtent[6]);

private:
  vtkImageSpatialFilter(const vtkImageSpatialFilter&);  // Not implemented.
  void operator=(const vtkImageSpatialFilter&);  // Not implemented.
};

#endif










