/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageHybridMedian2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageHybridMedian2D
 * @brief   Median filter that preserves lines and
 * corners.
 *
 * vtkImageHybridMedian2D is a median filter that preserves thin lines and
 * corners.  It operates on a 5x5 pixel neighborhood.  It computes two values
 * initially: the median of the + neighbors and the median of the x
 * neighbors.  It then computes the median of these two values plus the center
 * pixel.  This result of this second median is the output pixel value.
*/

#ifndef vtkImageHybridMedian2D_h
#define vtkImageHybridMedian2D_h


#include "vtkImagingGeneralModule.h" // For export macro
#include "vtkImageSpatialAlgorithm.h"

class VTKIMAGINGGENERAL_EXPORT vtkImageHybridMedian2D : public vtkImageSpatialAlgorithm
{
public:
  static vtkImageHybridMedian2D *New();
  vtkTypeMacro(vtkImageHybridMedian2D,vtkImageSpatialAlgorithm);

protected:
  vtkImageHybridMedian2D();
  ~vtkImageHybridMedian2D() override {}

  void ThreadedRequestData(vtkInformation *request,
                           vtkInformationVector **inputVector,
                           vtkInformationVector *outputVector,
                           vtkImageData ***inData, vtkImageData **outData,
                           int outExt[6], int id) override;
private:
  vtkImageHybridMedian2D(const vtkImageHybridMedian2D&) = delete;
  void operator=(const vtkImageHybridMedian2D&) = delete;
};

#endif
// VTK-HeaderTest-Exclude: vtkImageHybridMedian2D.h
