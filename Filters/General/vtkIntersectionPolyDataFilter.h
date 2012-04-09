/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIntersectionPolyDataFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkIntersectionPolyDataFilter
// .SECTION Description
//
// vtkIntersectionPolyDataFilter computes the intersection between two
// vtkPolyData objects. The first output is a set of lines that marks
// the intersection of the input vtkPolyData objects. The second and
// third outputs are the first and second input vtkPolyData,
// respectively. Optionally, the two output vtkPolyData can be split
// along the intersection lines.
//
// This code was contributed in the Insight Journal paper:
// "Boolean Operations on Surfaces in VTK Without External Libraries"
// by Cory Quammen, Chris Weigle C., Russ Taylor
// http://hdl.handle.net/10380/3262
// http://www.insight-journal.org/browse/publication/797

#ifndef __vtkIntersectionPolyDataFilter_h
#define __vtkIntersectionPolyDataFilter_h

#include "vtkPolyDataAlgorithm.h"

class VTK_GRAPHICS_EXPORT vtkIntersectionPolyDataFilter : public vtkPolyDataAlgorithm
{
public:
  static vtkIntersectionPolyDataFilter *New();
  vtkTypeMacro(vtkIntersectionPolyDataFilter, vtkPolyDataAlgorithm);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // If on, the second output will be the first input mesh split by the
  // intersection with the second input mesh. Defaults to on.
  vtkGetMacro(SplitFirstOutput, int);
  vtkSetMacro(SplitFirstOutput, int);
  vtkBooleanMacro(SplitFirstOutput, int);

  // Description:
  // If on, the third output will be the second input mesh split by the
  // intersection with the first input mesh. Defaults to on.
  vtkGetMacro(SplitSecondOutput, int);
  vtkSetMacro(SplitSecondOutput, int);
  vtkBooleanMacro(SplitSecondOutput, int);

  // Description:
  // Given two triangles defined by points (p1, q1, r1) and (p2, q2,
  // r2), returns whether the two triangles intersect. If they do,
  // the endpoints of the line forming the intersection are returned
  // in pt1 and pt2. The parameter coplanar is set to 1 if the
  // triangles are coplanar and 0 otherwise.
  static int TriangleTriangleIntersection(double p1[3], double q1[3], double r1[3],
                                          double p2[3], double q2[3], double r2[3],
                                          int &coplanar, double pt1[3], double pt2[3]);

protected:
  vtkIntersectionPolyDataFilter();
  ~vtkIntersectionPolyDataFilter();

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*);
  int FillInputPortInformation(int, vtkInformation*);

private:
  vtkIntersectionPolyDataFilter(const vtkIntersectionPolyDataFilter&); // Not implemented
  void operator=(const vtkIntersectionPolyDataFilter&); // Not implemented

  int SplitFirstOutput;
  int SplitSecondOutput;

  class Impl;
};


#endif // __vtkIntersectionPolyDataFilter_h
