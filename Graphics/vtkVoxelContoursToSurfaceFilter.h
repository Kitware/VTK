/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVoxelContoursToSurfaceFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

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

class VTK_GRAPHICS_EXPORT vtkVoxelContoursToSurfaceFilter : public vtkPolyDataToPolyDataFilter
{
public:
  static vtkVoxelContoursToSurfaceFilter *New();
  vtkTypeRevisionMacro(vtkVoxelContoursToSurfaceFilter,vtkPolyDataToPolyDataFilter);
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
private:
  vtkVoxelContoursToSurfaceFilter(const vtkVoxelContoursToSurfaceFilter&);  // Not implemented.
  void operator=(const vtkVoxelContoursToSurfaceFilter&);  // Not implemented.
};

#endif


