/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCacheFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkImageCacheFilter - Caches multiple vtkImageData objects.
// .SECTION Description
// vtkImageCacheFilter keep a number of vtkImageDataObjects from previous updates
// to satisfy future updates without needing to update the input.



#ifndef __vtkImageCacheFilter_h
#define __vtkImageCacheFilter_h

#include "vtkImageToImageFilter.h"

class VTK_EXPORT vtkImageCacheFilter : public vtkImageToImageFilter
{
public:
  static vtkImageCacheFilter *New();
  vtkTypeMacro(vtkImageCacheFilter,vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  void UpdateData(vtkDataObject *outData);

  void SetCacheSize(int size);
  vtkGetMacro(CacheSize, int);
  
protected:
  vtkImageCacheFilter();
  ~vtkImageCacheFilter();
  vtkImageCacheFilter(const vtkImageCacheFilter&);
  void operator=(const vtkImageCacheFilter&);

  int CacheSize;
  
  vtkImageData **Data;
  // I do not have write access to UpdateTime.
  unsigned long *Times;
};



#endif



