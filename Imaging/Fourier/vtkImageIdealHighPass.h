/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageIdealHighPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageIdealHighPass
 * @brief   Simple frequency domain band pass.
 *
 * This filter only works on an image after it has been converted to
 * frequency domain by a vtkImageFFT filter.  A vtkImageRFFT filter
 * can be used to convert the output back into the spatial domain.
 * vtkImageIdealHighPass just sets a portion of the image to zero.  The sharp
 * cutoff in the frequence domain produces ringing in the spatial domain.
 * Input and Output must be doubles.  Dimensionality is set when the axes are
 * set.  Defaults to 2D on X and Y axes.
 *
 * @sa
 * vtkImageButterworthHighPass vtkImageIdealLowPass vtkImageFFT vtkImageRFFT
*/

#ifndef vtkImageIdealHighPass_h
#define vtkImageIdealHighPass_h


#include "vtkImagingFourierModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

class VTKIMAGINGFOURIER_EXPORT vtkImageIdealHighPass : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageIdealHighPass *New();
  vtkTypeMacro(vtkImageIdealHighPass,vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set/Get the cutoff frequency for each axis.
   * The values are specified in the order X, Y, Z, Time.
   * Units: Cycles per world unit (as defined by the data spacing).
   */
  vtkSetVector3Macro(CutOff,double);
  void SetCutOff(double v) {this->SetCutOff(v, v, v);}
  void SetXCutOff(double v);
  void SetYCutOff(double v);
  void SetZCutOff(double v);
  vtkGetVector3Macro(CutOff,double);
  double GetXCutOff() {return this->CutOff[0];}
  double GetYCutOff() {return this->CutOff[1];}
  double GetZCutOff() {return this->CutOff[2];}
  //@}

protected:
  vtkImageIdealHighPass();
  ~vtkImageIdealHighPass() override {}

  double CutOff[3];

  void ThreadedRequestData(vtkInformation *request,
                           vtkInformationVector **inputVector,
                           vtkInformationVector *outputVector,
                           vtkImageData ***inData, vtkImageData **outData,
                           int outExt[6], int id) override;
private:
  vtkImageIdealHighPass(const vtkImageIdealHighPass&) = delete;
  void operator=(const vtkImageIdealHighPass&) = delete;
};

#endif
