/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVoxelContoursToSurfaceFilter.h
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
// .NAME vtkVoxelContoursToSurfaceFilter - create surface from contours
// .SECTION Description
// vtkVoxelContoursToSurfaceFilter is a filter that takes contours and
// produces surfaces. There are some restrictions for the contours:
//
//   - The contours are input as vtkPolyData, with the contours being
//     polys in the vtkPolyData.
//   - The contours lie on XY planes - each contour has a constant Z
//   - The contours are ordered in the polys of the vtkPolyData such 
//     that all contours on the first (lowest) XY plane are first, then
//     continuing in order of increasing Z value. 
//   - The X, Y and Z coordinates are all integer values.
//   - The desired sampling of the contour data is 1x1x1 - Aspect can
//     be used to control the aspect ratio in the output polygonal
//     dataset.
//
// This filter takes the contours and produces a structured points
// dataset of signed floating point number indicating distance from
// a contour. A contouring filter is then applied to generate 3D
// surfaces from a stack of 2D contour distance slices. This is 
// done in a streaming fashion so as not to use to much memory.

// .SECTION See Also
// vtkPolyDataToPolyDataFilter

#ifndef __vtkVoxelContoursToSurfaceFilter_h
#define __vtkVoxelContoursToSurfaceFilter_h

#include "vtkPolyDataToPolyDataFilter.h"

class VTK_EXPORT vtkVoxelContoursToSurfaceFilter : public vtkPolyDataToPolyDataFilter
{
public:
  static vtkVoxelContoursToSurfaceFilter *New();
  vtkTypeMacro(vtkVoxelContoursToSurfaceFilter,vtkPolyDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set / Get the memory limit in bytes for this filter. This is the limit
  // of the size of the structured points data set that is created for
  // intermediate processing. The data will be streamed through this volume
  // in as many pieces as necessary.
  vtkSetMacro( MemoryLimitInBytes, int );
  vtkGetMacro( MemoryLimitInBytes, int );

  vtkSetVector3Macro( Spacing, float );
  vtkGetVectorMacro( Spacing, float, 3 );

protected:
  vtkVoxelContoursToSurfaceFilter();
  ~vtkVoxelContoursToSurfaceFilter();
  vtkVoxelContoursToSurfaceFilter(const vtkVoxelContoursToSurfaceFilter&);
  void operator=(const vtkVoxelContoursToSurfaceFilter&);

  void    Execute();

  int     MemoryLimitInBytes;

  float   Spacing[3];

  float   *LineList;
  int     LineListLength;
  int     LineListSize;

  float   *SortedXList;
  float   *SortedYList;
  int     SortedListSize;

  int     *WorkingList;
  int     WorkingListLength;

  float   *IntersectionList;
  int     IntersectionListLength;

  void    AddLineToLineList( float x1, float y1, float x2, float y2 );
  void    SortLineList();
  
  void    CastLines( float *slice, float gridOrigin[3], 
		     int gridSize[3], int type );

  void    PushDistances( float *ptr, int gridSize[3], int chunkSize );
};

#endif


