/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageIslandRemoval2D.h
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
// .NAME vtkImageIslandRemoval2D - Removes small clusters in masks.
// .SECTION Description
// vtkImageIslandRemoval2D computes the area of separate islands in 
// a mask image.  It removes any island that has less than AreaThreshold
// pixels.  Output has the same ScalarType as input.  It generates
// the whole 2D output image for any output request.


#ifndef __vtkImageIslandRemoval2D_h
#define __vtkImageIslandRemoval2D_h


#include "vtkImageToImageFilter.h"

//BTX
typedef struct{
  void *inPtr;
  void *outPtr;
  int idx0;
  int idx1;
  } vtkImage2DIslandPixel;
//ETX

class VTK_EXPORT vtkImageIslandRemoval2D : public vtkImageToImageFilter
{
public:
  // Description:
  // Constructor: Sets default filter to be identity.
  static vtkImageIslandRemoval2D *New();
  vtkTypeMacro(vtkImageIslandRemoval2D,vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set/Get the cutoff area for removal
  vtkSetMacro(AreaThreshold, int);
  vtkGetMacro(AreaThreshold, int);

  // Description:
  // Set/Get whether to use 4 or 8 neighbors
  vtkSetMacro(SquareNeighborhood, int);
  vtkGetMacro(SquareNeighborhood, int);
  vtkBooleanMacro(SquareNeighborhood, int);

  // Description:
  // Set/Get the value to remove.
  vtkSetMacro(IslandValue, float);
  vtkGetMacro(IslandValue, float);

  // Description:
  // Set/Get the value to put in the place of removed pixels.
  vtkSetMacro(ReplaceValue, float);
  vtkGetMacro(ReplaceValue, float);
  
protected:
  vtkImageIslandRemoval2D();
  ~vtkImageIslandRemoval2D() {};
  vtkImageIslandRemoval2D(const vtkImageIslandRemoval2D&);
  void operator=(const vtkImageIslandRemoval2D&);

  int AreaThreshold;
  int SquareNeighborhood;
  float IslandValue;
  float ReplaceValue;

  void ExecuteData(vtkDataObject *output);
};

#endif



