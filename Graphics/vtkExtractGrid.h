/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractGrid.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


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
// .NAME vtkExtractGrid - select piece (e.g., volume of interest) and/or subsample structured grid dataset

// .SECTION Description
// vtkExtractGrid is a filter that selects a portion of an input structured
// grid dataset, or subsamples an input dataset. (The selected portion of
// interested is referred to as the Volume Of Interest, or VOI.) The output of
// this filter is a structured grid dataset. The filter treats input data of
// any topological dimension (i.e., point, line, image, or volume) and can
// generate output data of any topological dimension.
//
// To use this filter set the VOI ivar which are i-j-k min/max indices that
// specify a rectangular region in the data. (Note that these are 0-offset.)
// You can also specify a sampling rate to subsample the data.
//
// Typical applications of this filter are to extract a plane from a grid for 
// contouring, subsampling large grids to reduce data size, or extracting
// regions of a grid with interesting data.

// .SECTION See Also
// vtkGeometryFilter vtkExtractGeometry vtkExtractVOI 
// vtkStructuredGridGeometryFilter

#ifndef __vtkExtractGrid_h
#define __vtkExtractGrid_h

#include "vtkStructuredGridToStructuredGridFilter.h"

class VTK_GRAPHICS_EXPORT vtkExtractGrid : public vtkStructuredGridToStructuredGridFilter
{
public:
  static vtkExtractGrid *New();
  vtkTypeMacro(vtkExtractGrid,vtkStructuredGridToStructuredGridFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify i-j-k (min,max) pairs to extract. The resulting structured grid
  // dataset can be of any topological dimension (i.e., point, line, plane,
  // or 3D grid). 
  vtkSetVector6Macro(VOI,int);
  vtkGetVectorMacro(VOI,int,6);

  // Description:
  // Set the sampling rate in the i, j, and k directions. If the rate is > 1,
  // then the resulting VOI will be subsampled representation of the input.
  // For example, if the SampleRate=(2,2,2), every other point will be
  // selected, resulting in a volume 1/8th the original size.
  vtkSetVector3Macro(SampleRate, int);
  vtkGetVectorMacro(SampleRate, int, 3);

  // Description:
  // Control whether to enforce that the "boundary" of the grid is output in
  // the subsampling process. (This ivar only has effect when the SampleRate
  // in any direction is not equal to 1.) When this ivar IncludeBoundary is
  // on, the subsampling will always include the boundary of the grid even
  // though the sample rate is not an even multiple of the grid
  // dimensions. (By default IncludeBoundary is off.)
  vtkSetMacro(IncludeBoundary,int);
  vtkGetMacro(IncludeBoundary,int);
  vtkBooleanMacro(IncludeBoundary,int);

protected:
  vtkExtractGrid();
  ~vtkExtractGrid() {};
  vtkExtractGrid(const vtkExtractGrid&);
  void operator=(const vtkExtractGrid&);

  void Execute();
  void ExecuteInformation();
  void ComputeInputUpdateExtents(vtkDataObject *out);
  
  int VOI[6];
  int SampleRate[3];
  int IncludeBoundary;
  
};

#endif


