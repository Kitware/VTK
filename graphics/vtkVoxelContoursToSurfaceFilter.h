/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVoxelContoursToSurfaceFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
  const char *GetClassName() {return "vtkVoxelContoursToSurfaceFilter";};
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
  vtkVoxelContoursToSurfaceFilter(const vtkVoxelContoursToSurfaceFilter&) {};
  void operator=(const vtkVoxelContoursToSurfaceFilter&) {};

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


