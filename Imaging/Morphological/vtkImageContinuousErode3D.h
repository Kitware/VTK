/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageContinuousErode3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageContinuousErode3D
 * @brief   Erosion implemented as a minimum.
 *
 * vtkImageContinuousErode3D replaces a pixel with the minimum over
 * an ellipsoidal neighborhood.  If KernelSize of an axis is 1, no processing
 * is done on that axis.
*/

#ifndef vtkImageContinuousErode3D_h
#define vtkImageContinuousErode3D_h


#include "vtkImagingMorphologicalModule.h" // For export macro
#include "vtkImageSpatialAlgorithm.h"

class vtkImageEllipsoidSource;

class VTKIMAGINGMORPHOLOGICAL_EXPORT vtkImageContinuousErode3D : public vtkImageSpatialAlgorithm
{
public:
  //@{
  /**
   * Construct an instance of vtkImageContinuousErode3D filter.
   * By default zero values are eroded.
   */
  static vtkImageContinuousErode3D *New();
  vtkTypeMacro(vtkImageContinuousErode3D,vtkImageSpatialAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  /**
   * This method sets the size of the neighborhood.  It also sets the
   * default middle of the neighborhood and computes the elliptical foot print.
   */
  void SetKernelSize(int size0, int size1, int size2);

protected:
  vtkImageContinuousErode3D();
  ~vtkImageContinuousErode3D() override;

  vtkImageEllipsoidSource *Ellipse;

  void ThreadedRequestData(vtkInformation *request,
                           vtkInformationVector **inputVector,
                           vtkInformationVector *outputVector,
                           vtkImageData ***inData, vtkImageData **outData,
                           int extent[6], int id) override;
  int RequestData(vtkInformation *request,
                          vtkInformationVector **inputVector,
                          vtkInformationVector *outputVector) override;

private:
  vtkImageContinuousErode3D(const vtkImageContinuousErode3D&) = delete;
  void operator=(const vtkImageContinuousErode3D&) = delete;
};

#endif
