/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredPointsFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkStructuredPointsFilter - filter that takes vtkStructuredPoints as input
// .SECTION Description
// vtkStructuredPointsFilter is a filter that takes a single vtkStructuredPoints 
// data object as input.

// .SECTION See Also
// vtkStructuredPointsToPolyDataFilter vtkStructuredPointsToStructuredPointsFilter

#ifndef __vtkStructuredPointsFilter_h
#define __vtkStructuredPointsFilter_h

#include "vtkFilter.h"
#include "vtkStructuredPoints.h"
#include "vtkImageToStructuredPoints.h"
#include "vtkImageCache.h"

class VTK_EXPORT vtkStructuredPointsFilter : public vtkFilter 
{
public:
  static vtkStructuredPointsFilter *New() {return new vtkStructuredPointsFilter;};
  char *GetClassName() {return "vtkStructuredPointsFilter";};

  void SetInput(vtkStructuredPoints *input);
  void SetInput(vtkStructuredPoints &input) {this->SetInput(&input);};
  void SetInput(vtkImageCache *cache)
    {this->SetInput(cache->GetImageToStructuredPoints()->GetOutput());}
  vtkStructuredPoints *GetInput() {return (vtkStructuredPoints *)this->Input;};
};

#endif


