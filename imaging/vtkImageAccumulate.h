/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageAccumulate.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class

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
// .NAME vtkImageAccumulate - Generalized histograms up to 4 dimensions.
// .SECTION Description
// vtkImageAccumulate - This filter divides component space into
// discrete bins.  It then counts the number of pixels associated
// with each bin.  The output is this "scatter plot".
// The input can be any type, but the output is always int.
// The SetStencilFunction, SetClippingExtents and ReverseStencil
// functions allow the statistics to be computed on an arbitrary
// portion of the input data.
// See the documentation for vtkImageStencil for more information.


#ifndef __vtkImageAccumulate_h
#define __vtkImageAccumulate_h


#include "vtkImageToImageFilter.h"
#include "vtkImageStencilData.h"

class VTK_EXPORT vtkImageAccumulate : public vtkImageToImageFilter
{
public:
  static vtkImageAccumulate *New();
  vtkTypeMacro(vtkImageAccumulate,vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get - The component spacing is the dimension of each cell.
  vtkSetVector3Macro(ComponentSpacing, float);
  vtkGetVector3Macro(ComponentSpacing, float);

  // Description:
  // Set/Get - The component origin is the location of bin (0, 0, 0).
  vtkSetVector3Macro(ComponentOrigin, float);
  vtkGetVector3Macro(ComponentOrigin, float);

  // Description:
  // Set/Get - The component extent is the number/extent of the bins.  
  void SetComponentExtent(int extent[6]);
  void SetComponentExtent(int minX, int maxX, int minY, int maxY, 
			  int minZ, int maxZ);
  void GetComponentExtent(int extent[6]);
  int *GetComponentExtent() {return this->ComponentExtent;}


  // Description:
  // Use a stencil to specify which voxels to accumulate.
  void SetStencil(vtkImageStencilData *stencil);
  vtkImageStencilData *GetStencil();

  // Description:
  // Reverse the stencil.
  vtkSetMacro(ReverseStencil, int);
  vtkBooleanMacro(ReverseStencil, int);
  vtkGetMacro(ReverseStencil, int);

  // Description:
  // Get the statistics information for the data.
  vtkGetVector3Macro(Min, double);
  vtkGetVector3Macro(Max, double);
  vtkGetVector3Macro(Mean, double);
  vtkGetMacro(VoxelCount, long int);
 
  
protected:
  vtkImageAccumulate();
  ~vtkImageAccumulate();
  vtkImageAccumulate(const vtkImageAccumulate&) {};
  void operator=(const vtkImageAccumulate&) {};

  float ComponentSpacing[3];
  float ComponentOrigin[3];
  int ComponentExtent[6];

  void ExecuteInformation(vtkImageData *input, vtkImageData *output);
  void ComputeInputUpdateExtent(int inExt[6], int outExt[6]);
  void ExecuteInformation(){this->vtkImageToImageFilter::ExecuteInformation();};
  void ExecuteData(vtkDataObject *out);

  double Min[3];
  double Max[3];
  double Mean[3];
  long int VoxelCount;

  int ReverseStencil;

};

#endif



