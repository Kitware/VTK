/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageIdealHighPass.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageIdealHighPass - Simple frequency domain band pass.
// .SECTION Description
// This filter only works on an image after it has been converted to
// frequency domain by a vtkImageFFT filter.  A vtkImageRFFT filter
// can be used to convert the output back into the spatial domain.
// vtkImageIdealHighPass just sets a portion of the image to zero.  The sharp
// cutoff in the frequence domain produces ringing in the spatial domain.
// Input and Output must be floats.  Dimensionality is set when the axes are
// set.  Defaults to 2D on X and Y axes.

// .SECTION See Also
// vtkImageButterworthHighPass vtkImageIdealLowPass vtkImageFFT vtkImageRFFT


#ifndef __vtkImageIdealHighPass_h
#define __vtkImageIdealHighPass_h


#include "vtkImageToImageFilter.h"

class VTK_IMAGING_EXPORT vtkImageIdealHighPass : public vtkImageToImageFilter
{
public:
  static vtkImageIdealHighPass *New();
  vtkTypeRevisionMacro(vtkImageIdealHighPass,vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the cutoff frequency for each axis.
  // The values are specified in the order X, Y, Z, Time.
  // Units: Cycles per world unit (as defined by the data spacing).
  vtkSetVector3Macro(CutOff,float);
  void SetCutOff(float v) {this->SetCutOff(v, v, v);}
  void SetXCutOff(float v);
  void SetYCutOff(float v);
  void SetZCutOff(float v);
  vtkGetVector3Macro(CutOff,float);
  float GetXCutOff() {return this->CutOff[0];}
  float GetYCutOff() {return this->CutOff[1];}
  float GetZCutOff() {return this->CutOff[2];}

protected:
  vtkImageIdealHighPass();
  ~vtkImageIdealHighPass() {};

  float CutOff[3];
  
  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData,
                       int outExt[6], int id);
private:
  vtkImageIdealHighPass(const vtkImageIdealHighPass&);  // Not implemented.
  void operator=(const vtkImageIdealHighPass&);  // Not implemented.
};

#endif



