/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageOpenClose.h
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
// .NAME vtkImageOpenClose - Will perform opening or closing.
// .SECTION Description
// vtkImageOpenClose performs opening or closing by having two 
// vtkImageErodeDilates in series.  The size of operation
// is determined by the method SetKernelSize, and the operator is an elispe.
// OpenValue and CloseValue determine how the filter behaves.  For binary
// images Opening and closing behaves as expected.
// Close value is first dilated, and then eroded.
// Open value is first eroded, and then dilated.


#ifndef __vtkImageOpenClose_h
#define __vtkImageOpenClose_h


#include "vtkImageFilter.h"
#include "vtkImageDilateErode.h"

class vtkImageOpenClose : public vtkImageFilter
{
public:
  vtkImageOpenClose();
  ~vtkImageOpenClose();
  char *GetClassName() {return "vtkImageOpenClose";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Forward Object messages to filter0 and fitler1
  void DebugOn();
  void Modified();
  // Foward Source messages to filter1
  void SetCache(vtkImageCache *cache);
  vtkImageCache *GetCache();
  vtkImageSource *GetOutput();
  unsigned long GetPipelineMTime();
  // Foward filter messages
  void SetInput(vtkImageSource *Input);
  void SetAxes(int axis0, int axis1, int axis2);
  // Forward dilateErode messages to both filters.
  void SetKernelSize(int size);
  void SetKernelSize(int size0, int size1, int size2);
  void SetOpenValue(float value);
  void SetCloseValue(float value);
  float GetOpenValue();
  float GetCloseValue();

protected:
  
  vtkImageDilateErode *Filter0;
  vtkImageDilateErode *Filter1;
};

#endif



