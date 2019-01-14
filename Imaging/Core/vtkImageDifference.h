/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDifference.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageDifference
 * @brief   Compares images for regression tests.
 *
 * vtkImageDifference takes two rgb unsigned char images and compares them.
 * It allows the images to be slightly different.  If AllowShift is on,
 * then each pixel can be shifted by two pixels. Threshold is the allowable
 * error for each pixel.
 *
 * This is a symmetric filter and the difference computed is symmetric.
 * The resulting value is the maximum error of the two directions
 * A->B and B->A
*/

#ifndef vtkImageDifference_h
#define vtkImageDifference_h

#include "vtkImagingCoreModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

class vtkImageDifferenceThreadData;
class vtkImageDifferenceSMPThreadLocal;

class VTKIMAGINGCORE_EXPORT vtkImageDifference : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageDifference *New();
  vtkTypeMacro(vtkImageDifference,vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Specify the Image to compare the input to.
   */
  void SetImageConnection(vtkAlgorithmOutput* output)
  {
    this->SetInputConnection(1, output);
  }
  void SetImageData(vtkDataObject *image) {this->SetInputData(1,image);}
  vtkImageData *GetImage();
  //@}

  /**
   * Return the total error in comparing the two images.
   */
  double GetError() { return this->Error; }
  void GetError(double *e) { *e = this->GetError(); };

  /**
   * Return the total thresholded error in comparing the two images.
   * The thresholded error is the error for a given pixel minus the
   * threshold and clamped at a minimum of zero.
   */
  double GetThresholdedError() { return this->ThresholdedError; }
  void GetThresholdedError(double *e) { *e = this->GetThresholdedError(); };

  //@{
  /**
   * Specify a threshold tolerance for pixel differences.
   */
  vtkSetMacro(Threshold,int);
  vtkGetMacro(Threshold,int);
  //@}

  //@{
  /**
   * Specify whether the comparison will allow a shift of two
   * pixels between the images.  If set, then the minimum difference
   * between input images will be used to determine the difference.
   * Otherwise, the difference is computed directly between pixels
   * of identical row/column values.
   */
  vtkSetMacro(AllowShift,vtkTypeBool);
  vtkGetMacro(AllowShift,vtkTypeBool);
  vtkBooleanMacro(AllowShift,vtkTypeBool);
  //@}

  //@{
  /**
   * Specify whether the comparison will include comparison of
   * averaged 3x3 data between the images. For graphics renderings
   * you normally would leave this on. For imaging operations it
   * should be off.
   */
  vtkSetMacro(Averaging,vtkTypeBool);
  vtkGetMacro(Averaging,vtkTypeBool);
  vtkBooleanMacro(Averaging,vtkTypeBool);
  //@}

  //@{
  /**
   * When doing Averaging, adjust the threshold for the average
   * by this factor. Defaults to 0.5 requiring a better match
   */
  vtkSetMacro(AverageThresholdFactor,double);
  vtkGetMacro(AverageThresholdFactor,double);
  //@}

protected:
  vtkImageDifference();
  ~vtkImageDifference() override {}

  // Parameters
  vtkTypeBool AllowShift;
  int Threshold;
  vtkTypeBool Averaging;

  // Outputs
  const char *ErrorMessage;
  double Error;
  double ThresholdedError;
  double AverageThresholdFactor;

  int RequestInformation (vtkInformation *,
                                  vtkInformationVector **,
                                  vtkInformationVector *) override;
  int RequestUpdateExtent(vtkInformation *,
                                  vtkInformationVector **,
                                  vtkInformationVector *) override;
  int RequestData(vtkInformation *,
                          vtkInformationVector **,
                          vtkInformationVector *) override;

  void ThreadedRequestData(vtkInformation *request,
                                   vtkInformationVector **inputVector,
                                   vtkInformationVector *outputVector,
                                   vtkImageData ***inData,
                                   vtkImageData **outData,
                                   int extent[6], int threadId) override;

  // Used for vtkMultiThreader operation.
  vtkImageDifferenceThreadData *ThreadData;

  // Used for vtkSMPTools operation.
  vtkImageDifferenceSMPThreadLocal *SMPThreadData;

private:
  vtkImageDifference(const vtkImageDifference&) = delete;
  void operator=(const vtkImageDifference&) = delete;

  friend class vtkImageDifferenceSMPFunctor;
};

#endif


