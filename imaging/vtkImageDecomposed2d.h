/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDecomposed2d.h
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
// .NAME vtkImageDecomposed2d - Contains 2 1d filters.
// .SECTION Description
// vtkImageDecomposed2d is a super class for filters that break
// their 2d processing into two 1d steps.  They contain a sub pipeline
// that contains two 1d filters in series.


#ifndef __vtkImageDecomposed2d_h
#define __vtkImageDecomposed2d_h


#include "vtkImageFilter.h"

class vtkImageDecomposed2d : public vtkImageFilter
{
public:
  vtkImageDecomposed2d();
  ~vtkImageDecomposed2d();
  char *GetClassName() {return "vtkImageDecomposed2d";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Forward Object messages to filter1 and fitler2
  void DebugOn();
  void Modified();
  // Foward Source messages to filter2
  vtkImageSource *GetOutput();
  void SetCache(vtkImageCache *cache);
  vtkImageCache *GetCache();
  void SetReleaseDataFlag(int flag);
  unsigned long GetPipelineMTime();
  
  // Foward filter messages to fitler1
  void SetInput(vtkImageSource *Input);

  void SetAxes(int axis1, int axis2);

protected:

  vtkImageFilter *Filter0;
  vtkImageFilter *Filter1;
};

#endif



