/*=========================================================================

  Program:   Visualization Toolkit
  Module:    MCubes.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkMarchingCubes - generate iso-surface(s) from volume
// .SECTION Description
// vtkMarchingCubes is a filter that takes as input a volume (e.g., 3D
// structured point set) and generates on output one or more iso-surfaces.
// One or more contour values must be specified to generate the iso-surfaces.
// Alternatively, you can specify a min/max scalar range and the number of
// contours to generate a series of evenly spaced contour values. The current
// implementation requires that the scalar data is defined with "short int"
// data values.
// .SECTION Caveats
// The output primitives are disjoint - that is, points may
// be generated that are coincident but distinct. You may want to use
// vtkCleanPolyData to remove the coincident points. 
// .SECTION See Also
// This filter is specialized to volumes. If you are interested in 
// contouring other types of data, use the general vtkContourFilter.

#ifndef __vtkMarchingCubes_h
#define __vtkMarchingCubes_h

#include "SPt2Poly.hh"

#define MAX_CONTOURS 256

class vtkMarchingCubes : public vtkStructuredPointsToPolyDataFilter
{
public:
  vtkMarchingCubes();
  ~vtkMarchingCubes();
  char *GetClassName() {return "vtkMarchingCubes";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void SetValue(int i, float value);

  // Description:
  // Return array of contour values (size of numContours).
  vtkGetVectorMacro(Values,float,MAX_CONTOURS);

  void GenerateValues(int numContours, float range[2]);
  void GenerateValues(int numContours, float range1, float range2);

protected:
  void Execute();

  float Values[MAX_CONTOURS];
  int NumberOfContours;
  float Range[2];
};

#endif


