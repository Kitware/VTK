/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageButterworthHighPass.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkImageButterworthHighPass - Frequency domain high pass.
// .SECTION Description
// vtkImageButterworthHighPass  the frequency components around 0 are
// attenuated.  Input and output are in floats, with two components
// (complex numbers).
// out(i, j) = 1 / (1 + pow(CutOff/Freq(i,j), 2*Order));



#ifndef __vtkImageButterworthHighPass_h
#define __vtkImageButterworthHighPass_h


#include "vtkImageFilter.h"

class VTK_EXPORT vtkImageButterworthHighPass : public vtkImageFilter
{
public:
  vtkImageButterworthHighPass();
  static vtkImageButterworthHighPass *New() 
    {return new vtkImageButterworthHighPass;};
  const char *GetClassName() {return "vtkImageButterworthHighPass";};

  // Description:
  // Set/Get the cutoff frequency for each axis.
  // The values are specified in the order X, Y, Z, Time.
  // Units: Cycles per world unit (as defined by the data spacing).
  vtkSetVector4Macro(CutOff,float);
  void SetCutOff(float v) {this->SetCutOff(v, v, v, v);}
  void SetXCutOff(float v);
  void SetYCutOff(float v);
  void SetZCutOff(float v);
  void SetTimeCutOff(float v);
  vtkGetVector4Macro(CutOff,float);
  float GetXCutOff() {return this->CutOff[0];}
  float GetYCutOff() {return this->CutOff[1];}
  float GetZCutOff() {return this->CutOff[2];}
  float GetTimeCutOff() {return this->CutOff[3];}

  // Description:
  // The order determines sharpness of the cutoff curve.
  vtkSetMacro(Order, int);
  vtkGetMacro(Order, int);
  
  
protected:
  int Order;
  float CutOff[4];
  
  void Execute(vtkImageRegion *inRegion, vtkImageRegion *outRegion);
};

#endif



