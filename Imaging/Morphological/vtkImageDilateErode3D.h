/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDilateErode3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageDilateErode3D - Dilates one value and erodes another.
// .SECTION Description
// vtkImageDilateErode3D will dilate one value and erode another.
// It uses an elliptical foot print, and only erodes/dilates on the
// boundary of the two values.  The filter is restricted to the
// X, Y, and Z axes for now.  It can degenerate to a 2 or 1 dimensional
// filter by setting the kernel size to 1 for a specific axis.


#ifndef vtkImageDilateErode3D_h
#define vtkImageDilateErode3D_h


#include "vtkImagingMorphologicalModule.h" // For export macro
#include "vtkImageSpatialAlgorithm.h"

class vtkImageEllipsoidSource;

class VTKIMAGINGMORPHOLOGICAL_EXPORT vtkImageDilateErode3D : public vtkImageSpatialAlgorithm
{
public:
  // Description:
  // Construct an instance of vtkImageDilateErode3D filter.
  // By default zero values are dilated.
  static vtkImageDilateErode3D *New();
  vtkTypeMacro(vtkImageDilateErode3D,vtkImageSpatialAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This method sets the size of the neighborhood.  It also sets the
  // default middle of the neighborhood and computes the elliptical foot print.
  void SetKernelSize(int size0, int size1, int size2);


  // Description:
  // Set/Get the Dilate and Erode values to be used by this filter.
  vtkSetMacro(DilateValue, double);
  vtkGetMacro(DilateValue, double);
  vtkSetMacro(ErodeValue, double);
  vtkGetMacro(ErodeValue, double);

protected:
  vtkImageDilateErode3D();
  ~vtkImageDilateErode3D();

  vtkImageEllipsoidSource *Ellipse;
  double DilateValue;
  double ErodeValue;

  void ThreadedRequestData(vtkInformation *request,
                           vtkInformationVector **inputVector,
                           vtkInformationVector *outputVector,
                           vtkImageData ***inData, vtkImageData **outData,
                           int extent[6], int id);
  virtual int RequestData(vtkInformation *request,
                          vtkInformationVector **inputVector,
                          vtkInformationVector *outputVector);

private:
  vtkImageDilateErode3D(const vtkImageDilateErode3D&);  // Not implemented.
  void operator=(const vtkImageDilateErode3D&);  // Not implemented.
};

#endif
