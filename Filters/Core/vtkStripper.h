/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStripper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkStripper - create triangle strips and/or poly-lines
// .SECTION Description

// vtkStripper is a filter that generates triangle strips and/or poly-lines
// from input polygons, triangle strips, and lines. Input polygons are
// assembled into triangle strips only if they are triangles; other types of
// polygons are passed through to the output and not stripped. (Use
// vtkTriangleFilter to triangulate non-triangular polygons prior to running
// this filter if you need to strip all the data.) The filter will pass
// through (to the output) vertices if they are present in the input
// polydata. Also note that if triangle strips or polylines are defined in
// the input they are passed through and not joined nor extended. (If you wish
// to strip these use vtkTriangleFilter to fragment the input into triangles
// and lines prior to running vtkStripper.)
//
// The ivar MaximumLength can be used to control the maximum
// allowable triangle strip and poly-line length.
//
// By default, this filter discards any cell data associated with the input.
// Thus is because the cell structure changes and and the old cell data
// is no longer valid. When PassCellDataAsFieldData flag is set,
// the cell data is passed as FieldData to the output using the following rule:
// 1) for every cell in the output that is not a triangle strip,
//    the cell data is inserted once per cell in the output field data.
// 2) for every triangle strip cell in the output:
//    ii) 1 tuple is inserted for every point(j|j>=2) in the strip.
//    This is the cell data for the cell formed by (j-2, j-1, j) in
//    the input.
// The field data order is same as cell data i.e. (verts,line,polys,tsrips).

// .SECTION Caveats
// If triangle strips or poly-lines exist in the input data they will
// be passed through to the output data. This filter will only construct
// triangle strips if triangle polygons are available; and will only
// construct poly-lines if lines are available.

// .SECTION See Also
// vtkTriangleFilter

#ifndef vtkStripper_h
#define vtkStripper_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class VTKFILTERSCORE_EXPORT vtkStripper : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkStripper,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with MaximumLength set to 1000.
  static vtkStripper *New();

  // Description:
  // Specify the maximum number of triangles in a triangle strip,
  // and/or the maximum number of lines in a poly-line.
  vtkSetClampMacro(MaximumLength,int,4,100000);
  vtkGetMacro(MaximumLength,int);

  // Description:
  // Enable/Disable passing of the CellData in the input to
  // the output as FieldData. Note the field data is transformed.
  vtkBooleanMacro(PassCellDataAsFieldData, int);
  vtkSetMacro(PassCellDataAsFieldData, int);
  vtkGetMacro(PassCellDataAsFieldData, int);

  // Description:
  // If on, the output polygonal dataset will have a celldata array that
  // holds the cell index of the original 3D cell that produced each output
  // cell. This is useful for picking. The default is off to conserve
  // memory.
  vtkSetMacro(PassThroughCellIds,int);
  vtkGetMacro(PassThroughCellIds,int);
  vtkBooleanMacro(PassThroughCellIds,int);

  // Description:
  // If on, the output polygonal dataset will have a pointdata array that
  // holds the point index of the original vertex that produced each output
  // vertex. This is useful for picking. The default is off to conserve
  // memory.
  vtkSetMacro(PassThroughPointIds,int);
  vtkGetMacro(PassThroughPointIds,int);
  vtkBooleanMacro(PassThroughPointIds,int);

protected:
  vtkStripper();
  ~vtkStripper() {}

  // Usual data generation method
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  int MaximumLength;
  int PassCellDataAsFieldData;
  int PassThroughCellIds;
  int PassThroughPointIds;

private:
  vtkStripper(const vtkStripper&);  // Not implemented.
  void operator=(const vtkStripper&);  // Not implemented.
};

#endif
