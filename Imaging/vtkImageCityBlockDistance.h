/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCityBlockDistance.h
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
// .NAME vtkImageCityBlockDistance - 1,2 or 3D distance map.
// .SECTION Description
// vtkImageCityBlockDistance creates a distance map using the city block
// (Manhatten) distance measure.  The input is a mask.  Zero values are
// considered boundaries.  The output pixel is the minimum of the input pixel
// and the distance to a boundary (or neighbor value + 1 unit).
// distance values are calculated in pixels.
// The filter works by taking 6 passes (for 3d distance map): 2 along each 
// axis (forward and backward). Each pass keeps a running minimum distance.
// For some reason, I preserve the sign if the distance.  If the input 
// mask is initially negative, the output distances will be negative.
// Distances maps can have inside (negative regions) 
// and outsides (positive regions).

#ifndef __vtkImageCityBlockDistance_h
#define __vtkImageCityBlockDistance_h


#include "vtkImageDecomposeFilter.h"

class VTK_IMAGING_EXPORT vtkImageCityBlockDistance : public vtkImageDecomposeFilter
{
public:
  static vtkImageCityBlockDistance *New();
  vtkTypeMacro(vtkImageCityBlockDistance,vtkImageDecomposeFilter);
  
protected:
  vtkImageCityBlockDistance();
  ~vtkImageCityBlockDistance() {};

  void ComputeInputUpdateExtent(int inExt[6], int outExt[6]);
  void IterativeExecuteData(vtkImageData *inData, vtkImageData *outData);

  void AllocateOutputScalars(vtkImageData *outData);

private:
  vtkImageCityBlockDistance(const vtkImageCityBlockDistance&);  // Not implemented.
  void operator=(const vtkImageCityBlockDistance&);  // Not implemented.
};

#endif



