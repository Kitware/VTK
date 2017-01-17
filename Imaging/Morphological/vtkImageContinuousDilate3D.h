/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageContinuousDilate3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageContinuousDilate3D
 * @brief   Dilate implemented as a maximum.
 *
 * vtkImageContinuousDilate3D replaces a pixel with the maximum over
 * an ellipsoidal neighborhood.  If KernelSize of an axis is 1, no processing
 * is done on that axis.
*/

#ifndef vtkImageContinuousDilate3D_h
#define vtkImageContinuousDilate3D_h


#include "vtkImagingMorphologicalModule.h" // For export macro
#include "vtkImageSpatialAlgorithm.h"

class vtkImageEllipsoidSource;

class VTKIMAGINGMORPHOLOGICAL_EXPORT vtkImageContinuousDilate3D : public vtkImageSpatialAlgorithm
{
public:

  //@{
  /**
   * Construct an instance of vtkImageContinuousDilate3D filter.
   * By default zero values are dilated.
   */
  static vtkImageContinuousDilate3D *New();
  vtkTypeMacro(vtkImageContinuousDilate3D,vtkImageSpatialAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

  /**
   * This method sets the size of the neighborhood.  It also sets the
   * default middle of the neighborhood and computes the elliptical foot print.
   */
  void SetKernelSize(int size0, int size1, int size2);

protected:
  vtkImageContinuousDilate3D();
  ~vtkImageContinuousDilate3D() VTK_OVERRIDE;

  vtkImageEllipsoidSource *Ellipse;

  void ThreadedRequestData(vtkInformation *request,
                           vtkInformationVector **inputVector,
                           vtkInformationVector *outputVector,
                           vtkImageData ***inData, vtkImageData **outData,
                           int extent[6], int id) VTK_OVERRIDE;
  int RequestData(vtkInformation *request,
                          vtkInformationVector **inputVector,
                          vtkInformationVector *outputVector) VTK_OVERRIDE;

private:
  vtkImageContinuousDilate3D(const vtkImageContinuousDilate3D&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImageContinuousDilate3D&) VTK_DELETE_FUNCTION;
};

#endif
