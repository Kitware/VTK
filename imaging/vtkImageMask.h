/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMask.h
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
// .NAME vtkImageMask - Combines a mask and an image.
// .SECTION Description
// vtkImageMask combines a mask with an image.  Non zero mask
// implies the output pixel will be the same as the image.
// If a mask pixel is zero,  the the output pixel
// is set to "MaskedValue".  The filter also has the option to pass
// the mask through a boolean not operation before processing the image.
// This reverses the passed and replaced pixels.
// The two inputs should have the same "WholeExtent".
// The mask input should be unsigned char, and the image scalar type
// is the same as the output scalar type.


#ifndef __vtkImageMask_h
#define __vtkImageMask_h


#include "vtkImageTwoInputFilter.h"

class VTK_EXPORT vtkImageMask : public vtkImageTwoInputFilter
{
public:
  vtkImageMask();
  ~vtkImageMask();
  static vtkImageMask *New() {return new vtkImageMask;};
  const char *GetClassName() {return "vtkImageMask";};

  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // SetGet the value of the output pixel replaced by mask.
  void SetMaskedOutputValue(int num, float *v);
  void SetMaskedOutputValue(float v) {this->SetMaskedOutputValue(1, &v);}
  void SetMaskedOutputValue(float v1, float v2) 
    {float v[2]; v[0]=v1; v[1]=v2; this->SetMaskedOutputValue(2, v);}
  void SetMaskedOutputValue(float v1, float v2, float v3) 
    {float v[3]; v[0]=v1; v[1]=v2; v[2]=v3; this->SetMaskedOutputValue(3, v);}
  float *GetMaskedOutputValue() {return this->MaskedOutputValue;}
  int GetMaskedOutputValueLength() {return this->MaskedOutputValueLength;}

  void SetImageInput(vtkImageCache *in) {this->SetInput1(in);}
  void SetImageInput(vtkStructuredPoints *in) {this->SetInput1(in);}
  void SetMaskInput(vtkImageCache *in) {this->SetInput2(in);}
  void SetMaskInput(vtkStructuredPoints *in) {this->SetInput2(in);}
  
  // Description:
  // When Nopt Mask is on, the mask is passed through a boolean not
  // before it is used to mask the image.  The effect is to pass the
  // pixels where the input mask is zero, and replace the pixels
  // where the input value is non zero.
  vtkSetMacro(NotMask,int);
  vtkGetMacro(NotMask,int);
  vtkBooleanMacro(NotMask, int);
  
protected:
  float *MaskedOutputValue;
  int MaskedOutputValueLength;
  int NotMask;
  
  void ThreadedExecute(vtkImageData **inDatas, vtkImageData *outData,
		       int extent[6], int id);
};

#endif



