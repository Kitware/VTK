/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageVariance3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageVariance3D
 * @brief   Variance in a neighborhood.
 *
 * vtkImageVariance3D replaces each pixel with a measurement of
 * pixel variance in a elliptical neighborhood centered on that pixel.
 * The value computed is not exactly the variance.
 * The difference between the neighbor values and center value is computed
 * and squared for each neighbor.  These values are summed and divided by
 * the total number of neighbors to produce the output value.
*/

#ifndef vtkImageVariance3D_h
#define vtkImageVariance3D_h


#include "vtkImagingGeneralModule.h" // For export macro
#include "vtkImageSpatialAlgorithm.h"

class vtkImageEllipsoidSource;

class VTKIMAGINGGENERAL_EXPORT vtkImageVariance3D : public vtkImageSpatialAlgorithm
{
public:
  static vtkImageVariance3D *New();
  vtkTypeMacro(vtkImageVariance3D,vtkImageSpatialAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * This method sets the size of the neighborhood.  It also sets the default
   * middle of the neighborhood and computes the Elliptical foot print.
   */
  void SetKernelSize(int size0, int size1, int size2);

protected:
  vtkImageVariance3D();
  ~vtkImageVariance3D() override;

  vtkImageEllipsoidSource *Ellipse;

  int RequestInformation (vtkInformation *request,
                                  vtkInformationVector **inputVector,
                                  vtkInformationVector *outputVector) override;

  void ThreadedRequestData(vtkInformation *request,
                           vtkInformationVector **inputVector,
                           vtkInformationVector *outputVector,
                           vtkImageData ***inData, vtkImageData **outData,
                           int extent[6], int id) override;
  int RequestData(vtkInformation *request,
                          vtkInformationVector **inputVector,
                          vtkInformationVector *outputVector) override;

private:
  vtkImageVariance3D(const vtkImageVariance3D&) = delete;
  void operator=(const vtkImageVariance3D&) = delete;
};

#endif
