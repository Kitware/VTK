/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageGaussianSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageGaussianSource - Create an image with Gaussian pixel values.
// .SECTION Description
// vtkImageGaussianSource just produces images with pixel values determined 
// by a Gaussian.


#ifndef __vtkImageGaussianSource_h
#define __vtkImageGaussianSource_h

#include "vtkImageSource.h"

class VTK_IMAGING_EXPORT vtkImageGaussianSource : public vtkImageSource
{
public:
  static vtkImageGaussianSource *New();
  vtkTypeRevisionMacro(vtkImageGaussianSource,vtkImageSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the extent of the whole output image.
  void SetWholeExtent(int xMinx, int xMax, int yMin, int yMax,
                      int zMin, int zMax);
  
  // Description:
  // Set/Get the center of the Gaussian.
  vtkSetVector3Macro(Center, double);
  vtkGetVector3Macro(Center, double);

  // Description:
  // Set/Get the Maximum value of the gaussian
  vtkSetMacro(Maximum, double);
  vtkGetMacro(Maximum, double);

  // Description:
  // Set/Get the standard deviation of the gaussian
  vtkSetMacro(StandardDeviation, double);
  vtkGetMacro(StandardDeviation, double);

protected:
  vtkImageGaussianSource();
  ~vtkImageGaussianSource() {};

  double StandardDeviation;
  int WholeExtent[6];
  double Center[3];
  double Maximum;

  virtual void ExecuteInformation();
  virtual void ExecuteData(vtkDataObject *data);
private:
  vtkImageGaussianSource(const vtkImageGaussianSource&);  // Not implemented.
  void operator=(const vtkImageGaussianSource&);  // Not implemented.
};


#endif
