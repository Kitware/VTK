/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImage2dDecomposedFilter.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

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
// .NAME vtkImage2dDecomposedFilter - Contains 2 1d filters.
// .SECTION Description
// vtkImage2dDecomposedFilter is a super class for filters that break
// their 2d processing into two 1d steps.  They contain a sub pipeline
// that contains two 1d filters in series.


#ifndef __vtkImage2dDecomposedFilter_h
#define __vtkImage2dDecomposedFilter_h


#include "vtkImageFilter.hh"

class vtkImage2dDecomposedFilter : public vtkImageFilter
{
public:
  vtkImage2dDecomposedFilter();
  ~vtkImage2dDecomposedFilter();
  char *GetClassName() {return "vtkImage2dDecomposedFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Forward Object messages to filter1 and fitler2
  void DebugOn();
  void Modified();
  // Foward Source messages to filter2
  void SetCache(vtkImageCache *cache);
  vtkImageCache *GetCache();
  vtkImageSource *GetOutput();
  unsigned long GetPipelineMTime();
  // Foward filter messages to fitler1
  void SetInput(vtkImageSource *Input);

  void SetAxes2d(int axis1, int axis2);

protected:

  vtkImageFilter *Filter0;
  vtkImageFilter *Filter1;
};

#endif



