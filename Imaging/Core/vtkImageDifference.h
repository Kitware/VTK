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
 * then each pixel can be shifted by one pixel. Threshold is the allowable
 * error for each pixel.
 *
 * This is not a symetric filter and the difference computed is not symetric
 * when AllowShift is on. Specifically in that case a pixel in SetImage input
 * will be compared to the matching pixel in the input as well as to the
 * input's eight connected neighbors. BUT... the opposite is not true. So for
 * example if a valid image (SetImage) has a single white pixel in it, it
 * will not find a match in the input image if the input image is black
 * (because none of the nine suspect pixels are white). In contrast, if there
 * is a single white pixel in the input image and the valid image (SetImage)
 * is all black it will match with no error because all it has to do is find
 * black pixels and even though the input image has a white pixel, its
 * neighbors are not white.
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
  void PrintSelf(ostream& os, vtkIndent indent);

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
   * Specify whether the comparison will allow a shift of one
   * pixel between the images.  If set, then the minimum difference
   * between input images will be used to determine the difference.
   * Otherwise, the difference is computed directly between pixels
   * of identical row/column values.
   */
  vtkSetMacro(AllowShift,int);
  vtkGetMacro(AllowShift,int);
  vtkBooleanMacro(AllowShift,int);
  //@}

  //@{
  /**
   * Specify whether the comparison will include comparison of
   * averaged 3x3 data between the images. For graphics renderings
   * you normally would leave this on. For imaging operations it
   * should be off.
   */
  vtkSetMacro(Averaging,int);
  vtkGetMacro(Averaging,int);
  vtkBooleanMacro(Averaging,int);
  //@}

protected:
  vtkImageDifference();
  ~vtkImageDifference() {}

  // Parameters
  int AllowShift;
  int Threshold;
  int Averaging;

  // Outputs
  const char *ErrorMessage;
  double Error;
  double ThresholdedError;

  virtual int RequestInformation (vtkInformation *,
                                  vtkInformationVector **,
                                  vtkInformationVector *);
  virtual int RequestUpdateExtent(vtkInformation *,
                                  vtkInformationVector **,
                                  vtkInformationVector *);
  virtual int RequestData(vtkInformation *,
                          vtkInformationVector **,
                          vtkInformationVector *);

  virtual void ThreadedRequestData(vtkInformation *request,
                                   vtkInformationVector **inputVector,
                                   vtkInformationVector *outputVector,
                                   vtkImageData ***inData,
                                   vtkImageData **outData,
                                   int extent[6], int threadId);

  // Used for vtkMultiThreader operation.
  vtkImageDifferenceThreadData *ThreadData;

  // Used for vtkSMPTools operation.
  vtkImageDifferenceSMPThreadLocal *SMPThreadData;

private:
  vtkImageDifference(const vtkImageDifference&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImageDifference&) VTK_DELETE_FUNCTION;

  friend class vtkImageDifferenceSMPFunctor;
};

#endif


