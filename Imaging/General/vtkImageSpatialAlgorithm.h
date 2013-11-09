/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSpatialAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageSpatialAlgorithm - Filters that operate on pixel neighborhoods.
// .SECTION Description
// vtkImageSpatialAlgorithm is a super class for filters that operate on an
// input neighborhood for each output pixel. It handles even sized
// neighborhoods, but their can be a half pixel shift associated with
// processing.  This superclass has some logic for handling boundaries.  It
// can split regions into boundary and non-boundary pieces and call different
// execute methods.


#ifndef __vtkImageSpatialAlgorithm_h
#define __vtkImageSpatialAlgorithm_h


#include "vtkImagingGeneralModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

class VTKIMAGINGGENERAL_EXPORT vtkImageSpatialAlgorithm : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageSpatialAlgorithm *New();
  vtkTypeMacro(vtkImageSpatialAlgorithm,vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the Kernel size.
  vtkGetVector3Macro(KernelSize,int);

  // Description:
  // Get the Kernel middle.
  vtkGetVector3Macro(KernelMiddle,int);

protected:
  vtkImageSpatialAlgorithm();
  ~vtkImageSpatialAlgorithm() {}

  int   KernelSize[3];
  int   KernelMiddle[3];      // Index of kernel origin
  int   HandleBoundaries;     // Output shrinks if boundaries aren't handled

  virtual int RequestInformation (vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  void ComputeOutputWholeExtent(int extent[6], int handleBoundaries);
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  void InternalRequestUpdateExtent(int *extent, int *inExtent, int *wholeExtent);

private:
  vtkImageSpatialAlgorithm(const vtkImageSpatialAlgorithm&);  // Not implemented.
  void operator=(const vtkImageSpatialAlgorithm&);  // Not implemented.
};

#endif










