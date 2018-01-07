/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageShrink3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageShrink3D
 * @brief   Subsamples an image.
 *
 * vtkImageShrink3D shrinks an image by sub sampling on a
 * uniform grid (integer multiples).
*/

#ifndef vtkImageShrink3D_h
#define vtkImageShrink3D_h


#include "vtkImagingCoreModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

class VTKIMAGINGCORE_EXPORT vtkImageShrink3D : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageShrink3D *New();
  vtkTypeMacro(vtkImageShrink3D,vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set/Get the shrink factors
   */
  vtkSetVector3Macro(ShrinkFactors,int);
  vtkGetVector3Macro(ShrinkFactors,int);
  //@}

  //@{
  /**
   * Set/Get the pixel to use as origin.
   */
  vtkSetVector3Macro(Shift,int);
  vtkGetVector3Macro(Shift,int);
  //@}

  //@{
  /**
   * Choose Mean, Minimum, Maximum, Median or sub sampling.
   * The neighborhood operations are not centered on the sampled pixel.
   * This may cause a half pixel shift in your output image.
   * You can changed "Shift" to get around this.
   * vtkImageGaussianSmooth or vtkImageMean with strides.
   */
  void SetAveraging(vtkTypeBool);
  vtkTypeBool GetAveraging() {return this->GetMean();};
  vtkBooleanMacro(Averaging,vtkTypeBool);
  //@}

  void SetMean(vtkTypeBool);
  vtkGetMacro(Mean,vtkTypeBool);
  vtkBooleanMacro(Mean,vtkTypeBool);

  void SetMinimum(vtkTypeBool);
  vtkGetMacro(Minimum,vtkTypeBool);
  vtkBooleanMacro(Minimum,vtkTypeBool);

  void SetMaximum(vtkTypeBool);
  vtkGetMacro(Maximum,vtkTypeBool);
  vtkBooleanMacro(Maximum,vtkTypeBool);

  void SetMedian(vtkTypeBool);
  vtkGetMacro(Median,vtkTypeBool);
  vtkBooleanMacro(Median,vtkTypeBool);

protected:
  vtkImageShrink3D();
  ~vtkImageShrink3D() override {}

  int ShrinkFactors[3];
  int Shift[3];
  int Mean;
  vtkTypeBool Minimum;
  vtkTypeBool Maximum;
  vtkTypeBool Median;

  int RequestInformation (vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;
  int RequestUpdateExtent (vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;

  void ThreadedRequestData(vtkInformation *request,
                           vtkInformationVector **inputVector,
                           vtkInformationVector *outputVector,
                           vtkImageData ***inData, vtkImageData **outData,
                           int ext[6], int id) override;

  void InternalRequestUpdateExtent(int *inExt, int *outExt);

private:
  vtkImageShrink3D(const vtkImageShrink3D&) = delete;
  void operator=(const vtkImageShrink3D&) = delete;
};

#endif



