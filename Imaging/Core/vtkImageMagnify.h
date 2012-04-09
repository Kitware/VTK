/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkImageMagnify.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageMagnify - magnify an image by an integer value
// .SECTION Description
// vtkImageMagnify maps each pixel of the input onto a nxmx... region
// of the output.  Location (0,0,...) remains in the same place. The
// magnification occurs via pixel replication, or if Interpolate is on,
// by bilinear interpolation. Initially, interpolation is off and magnification
// factors are set to 1 in all directions.

#ifndef __vtkImageMagnify_h
#define __vtkImageMagnify_h

#include "vtkThreadedImageAlgorithm.h"

class VTK_IMAGING_EXPORT vtkImageMagnify : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageMagnify *New();
  vtkTypeMacro(vtkImageMagnify,vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set/Get the integer magnification factors in the i-j-k directions.
  // Initially, factors are set to 1 in all directions.
  vtkSetVector3Macro(MagnificationFactors,int);
  vtkGetVector3Macro(MagnificationFactors,int);
  
  // Description:
  // Turn interpolation on and off (pixel replication is used when off).
  // Initially, interpolation is off.
  vtkSetMacro(Interpolate,int);
  vtkGetMacro(Interpolate,int);
  vtkBooleanMacro(Interpolate,int);
  
protected:
  vtkImageMagnify();
  ~vtkImageMagnify() {};
  
  int MagnificationFactors[3];
  int Interpolate;
  virtual int RequestUpdateExtent(vtkInformation *,
                                  vtkInformationVector **,
                                  vtkInformationVector *);
  virtual int RequestInformation(vtkInformation *,
                                 vtkInformationVector **,
                                 vtkInformationVector *);
  
  void ThreadedRequestData(vtkInformation *request,
                           vtkInformationVector **inputVector,
                           vtkInformationVector *outputVector,
                           vtkImageData ***inData,
                           vtkImageData **outData,
                           int outExt[6],
                           int id);

  void InternalRequestUpdateExtent(int *inExt, int *outExt);

private:
  vtkImageMagnify(const vtkImageMagnify&);  // Not implemented.
  void operator=(const vtkImageMagnify&);  // Not implemented.
};

#endif




