/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDecomposedFilter.h
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
// .NAME vtkImageDecomposedFilter - Contains multiple 1d filters.
// .SECTION Description
// vtkImageDecomposedFilter is a super class for filters that break
// their Nd processing into 1d steps.  They contain a sub pipeline
// that contains multiple 1d filters in series.  

#ifndef __vtkImageDecomposedFilter_h
#define __vtkImageDecomposedFilter_h


#include "vtkImageFilter.h"

class VTK_EXPORT vtkImageDecomposedFilter : public vtkImageFilter
{
public:
  vtkImageDecomposedFilter();
  ~vtkImageDecomposedFilter();
  const char *GetClassName() {return "vtkImageDecomposedFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void DebugOn();
  void Modified();
  void SetCache(vtkImageCache *cache);
  void SetInput(vtkImageCache *Input);
  //  void SetInput(vtkStructuredPoints *spts)
  //    {this->SetInput(spts->GetStructuredPointsToImage()->GetOutput());}
  // Input memory limit causes streaming
  void SetInputMemoryLimit(long limit);

  void SetFilteredAxes(int num, int *axes);
  vtkImageSetMacro(FilteredAxes, int);

  // Description:
  // Legacy compatability.
  void SetDimensionality(int dim) 
    {this->SetFilteredAxes(dim, this->FilteredAxes);}
  
  void Update();
  void SetStartMethod(void (*f)(void *), void *arg);
  void SetEndMethod(void (*f)(void *), void *arg);
  
protected:
  vtkImageFilter *Filters[4];
  
  void InitializeFilters();
  void SetInternalInput(vtkImageCache *Input);
  void DeleteFilters();
};


#endif



