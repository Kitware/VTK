/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageClip.h
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
// .NAME vtkImageClip - Reduces the image extent of the input.
// .SECTION Description
// vtkImageClip  will make an image smaller.  The output must have
// an image extent which is the subset of the input.  The filter has two 
// modes of operation: 
// 1: By default, the data is not copied in this filter. 
// Only the whole extent is modified.  
// 2: If ClipDataOn is set, then you will get no more that the clipped
// extent.
#ifndef __vtkImageClip_h
#define __vtkImageClip_h

// I did not make this a subclass of in place filter because
// the references on the data do not matter. I make no modifiactions
// to the data.
#include "vtkImageToImageFilter.h"

class VTK_EXPORT vtkImageClip : public vtkImageToImageFilter
{
public:
  static vtkImageClip *New() {return new vtkImageClip;};
  const char *GetClassName() {return "vtkImageClip";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The whole extent of the output has to be set explicitely.
  void SetOutputWholeExtent(int extent[6]);
  void SetOutputWholeExtent(int minX, int maxX, int minY, int maxY, 
			    int minZ, int maxZ);
  void GetOutputWholeExtent(int extent[6]);
  int *GetOutputWholeExtent() {return this->OutputWholeExtent;}

  void ResetOutputWholeExtent();
  void InternalUpdate(vtkDataObject *outData);

  // Description:
  // By default, ClipData is off, and only the WholeExtent is modified.
  // the data's extent may actually be larger.  When this flag is on,
  // the data extent will be no more than the OutputWholeExtent.
  vtkSetMacro(ClipData, int);
  vtkGetMacro(ClipData, int);
  vtkBooleanMacro(ClipData, int);

protected:
  vtkImageClip();
  ~vtkImageClip() {};

  // Time when OutputImageExtent was computed.
  vtkTimeStamp CTime;
  int Initialized; // Set the OutputImageExtent for the first time.
  int OutputWholeExtent[6];

  int ClipData;
  
  void ExecuteInformation(vtkImageData *inData, vtkImageData *outData);
  void CopyData(vtkImageData *inData, vtkImageData *outData, int *ext);
};



#endif



