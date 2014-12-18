/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageRange3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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


#ifndef vtkImageRange3D_h
#define vtkImageRange3D_h


#include "vtkImagingGeneralModule.h" // For export macro
#include "vtkImageSpatialAlgorithm.h"

class vtkImageEllipsoidSource;

class VTKIMAGINGGENERAL_EXPORT vtkImageRange3D : public vtkImageSpatialAlgorithm
{
public:
  static vtkImageRange3D *New();
  vtkTypeMacro(vtkImageRange3D,vtkImageSpatialAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This method sets the size of the neighborhood.  It also sets the
  // default middle of the neighborhood and computes the elliptical foot print.
  void SetKernelSize(int size0, int size1, int size2);

protected:
  vtkImageRange3D();
  ~vtkImageRange3D();

  vtkImageEllipsoidSource *Ellipse;

  virtual int RequestInformation (vtkInformation *request,
                                  vtkInformationVector **inputVector,
                                  vtkInformationVector *outputVector);
  void ThreadedRequestData(vtkInformation *request,
                           vtkInformationVector **inputVector,
                           vtkInformationVector *outputVector,
                           vtkImageData ***inData, vtkImageData **outData,
                           int extent[6], int id);
  virtual int RequestData(vtkInformation *request,
                          vtkInformationVector **inputVector,
                          vtkInformationVector *outputVector);

private:
  vtkImageRange3D(const vtkImageRange3D&);  // Not implemented.
  void operator=(const vtkImageRange3D&);  // Not implemented.
};

#endif
