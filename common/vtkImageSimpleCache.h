/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSimpleCache.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkImageSimpleCache - Caches the last region generated
// .SECTION Description
// vtkImageSimpleCache saves the last generated region.
// If a subsequent region is contained in the cached data, the
// cached data is returned with no call to the filters Update method.
// If the new region is not completely contained in the cached data,
// the cached data is not used.


#ifndef __vtkImageSimpleCache_h
#define __vtkImageSimpleCache_h

#include "vtkImageCache.h"

class VTK_EXPORT vtkImageSimpleCache : public vtkImageCache
{
public:
  static vtkImageSimpleCache *New() {return new vtkImageSimpleCache;};

  vtkTypeMacro(vtkImageSimpleCache,vtkImageCache);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This method updates the region specified by "UpdateExtent".  
  void Update();

  // Description:
  // This is the most common way to obtain data from a cache.
  // After setting the update extent invoke this method and it
  // will return an ImageData instance containing the requested data.
  vtkImageData *UpdateAndReturnData();

  // Description:
  // This method deletes any data in the cache.
  void ReleaseData();

  // Description:
  // Allocates the scalar data required for the current update extent.
  void AllocateData();

  // Description:
  // return the un filled data of the UpdateExtent in this cache.
  vtkImageData *GetData(); 

  // Description:
  // Convenience method to get the range of the scalar data in the
  // current "UpdateExtent". Returns the (min/max) range.  The components
  // are lumped into one range.  If there are no scalars the method will 
  // return (0,1). Note: Update needs to be called first to create the scalars.
  void GetScalarRange(float range[2]);  
  
protected:
  vtkImageSimpleCache();
  ~vtkImageSimpleCache();
  vtkImageSimpleCache(const vtkImageSimpleCache&) {};
  void operator=(const vtkImageSimpleCache&) {};

  vtkImageData *CachedData;
  vtkTimeStamp GenerateTime;
};

#endif


