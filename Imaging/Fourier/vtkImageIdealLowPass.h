/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageIdealLowPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageIdealLowPass - Simple frequency domain band pass.
// .SECTION Description
// This filter only works on an image after it has been converted to
// frequency domain by a vtkImageFFT filter.  A vtkImageRFFT filter
// can be used to convert the output back into the spatial domain.
// vtkImageIdealLowPass just sets a portion of the image to zero.  The result
// is an image with a lot of ringing.  Input and Output must be doubles.
// Dimensionality is set when the axes are set.  Defaults to 2D on X and Y
// axes.

// .SECTION See Also
// vtkImageButterworthLowPass vtkImageIdealHighPass vtkImageFFT vtkImageRFFT



#ifndef __vtkImageIdealLowPass_h
#define __vtkImageIdealLowPass_h


#include "vtkImagingFourierModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

class VTKIMAGINGFOURIER_EXPORT vtkImageIdealLowPass : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageIdealLowPass *New();
  vtkTypeMacro(vtkImageIdealLowPass,vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the cutoff frequency for each axis.
  // The values are specified in the order X, Y, Z, Time.
  // Units: Cycles per world unit (as defined by the data spacing).
  vtkSetVector3Macro(CutOff,double);
  void SetCutOff(double v) {this->SetCutOff(v, v, v);}
  void SetXCutOff(double v);
  void SetYCutOff(double v);
  void SetZCutOff(double v);
  vtkGetVector3Macro(CutOff,double);
  double GetXCutOff() {return this->CutOff[0];}
  double GetYCutOff() {return this->CutOff[1];}
  double GetZCutOff() {return this->CutOff[2];}

protected:
  vtkImageIdealLowPass();
  ~vtkImageIdealLowPass() {};

  double CutOff[3];

  void ThreadedRequestData(vtkInformation *request,
                           vtkInformationVector **inputVector,
                           vtkInformationVector *outputVector,
                           vtkImageData ***inData, vtkImageData **outData,
                           int outExt[6], int id);
private:
  vtkImageIdealLowPass(const vtkImageIdealLowPass&);  // Not implemented.
  void operator=(const vtkImageIdealLowPass&);  // Not implemented.
};

#endif
