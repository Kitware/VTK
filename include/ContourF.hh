/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ContourF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkContourFilter - generate iso-surfaces/iso-lines from scalar values
// .SECTION Description
// vtkContourFilter is a filter that takes as input any dataset and 
// generates on output iso-surfaces and/or iso-lines. The exact form 
// of the output depends upon the dimensionality of the input data. 
// Data consisting of 3D cells will generate iso-surfaces, data 
// consisting of 2D cells will generate iso-lines, and data with 1D 
// or 0D cells will generate iso-points. Combinations of output type 
// is possible if the input dimension is mixed.
//    If the input type is volume (e.g., 3D structured point dataset), 
// you may wish to use vtkMarchingCubes. This class is specifically tailored
// for volumes and is therefore much faster.
// .SECTION Caveats
// vtkContourFilter uses variations of marching cubes to generate output
// primitives. The output primitives are disjoint - that is, points may
// be generated that are coincident but distinct. You may want to use
// vtkCleanPolyData to remove the coincident points. Also, the iso-surface
// is not generated with surface normals. Use vtkPolyNormals to create them,
// if desired.

#ifndef __vtkContourFilter_h
#define __vtkContourFilter_h

#include "DS2PolyF.hh"

#define MAX_CONTOURS 256

class vtkContourFilter : public vtkDataSetToPolyFilter
{
public:
  vtkContourFilter();
  ~vtkContourFilter();
  char *GetClassName() {return "vtkContourFilter";};
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


