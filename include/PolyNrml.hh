/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PolyNrml.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkPolyNormals - compute normals for polygonal mesh
// .SECTION Description
// vtkPolyNormals is a filter that computes point normals for a polygonal 
// mesh. The filter can reorder polygons to insure consistent orientation
// across polygon neighbors. Sharp edges can be split and points duplicated
// with separate normals to give crisp (rendered) surface definition. It is
// also possible to globally flip the normal orientation.
//     The algorithm works by determing normals for each polyon and then
// averaging them at shared points. When sharp edges are present, the edges
// are split and new points generated to prevent blurry edges (due to 
// Gouraud shading).

#ifndef __vtkPolyNormals_h
#define __vtkPolyNormals_h

#include "P2PF.hh"

class vtkPolyNormals : public vtkPolyToPolyFilter
{
public:
  vtkPolyNormals();
  ~vtkPolyNormals() {};
  char *GetClassName() {return "vtkPolyNormals";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the angle that defines a sharp edge. If the difference in
  // angle across neighboring polygons is greater than this value, the
  // shared edge is considered "sharp".
  vtkSetClampMacro(FeatureAngle,float,0.0,180.0);
  vtkGetMacro(FeatureAngle,float);

  // Description:
  // Turn on/off the splitting of sharp edges.
  vtkSetMacro(Splitting,int);
  vtkGetMacro(Splitting,int);
  vtkBooleanMacro(Splitting,int);

  // Description:
  // Turn on/off the enforcement of consistent polygon ordering.
  vtkSetMacro(Consistency,int);
  vtkGetMacro(Consistency,int);
  vtkBooleanMacro(Consistency,int);

  // Description:
  // Turn on/off the global flipping of normal orientation.
  vtkSetMacro(FlipNormals,int);
  vtkGetMacro(FlipNormals,int);
  vtkBooleanMacro(FlipNormals,int);

  // Description:
  // Control the depth of recursion used in this algorithm. (Some systems
  // have limited stack depth.)
  vtkSetClampMacro(MaxRecursionDepth,int,10,LARGE_INTEGER);
  vtkGetMacro(MaxRecursionDepth,int);

protected:
  // Usual data generation method
  void Execute();

  float FeatureAngle;
  int Splitting;
  int Consistency;
  int FlipNormals;
  int MaxRecursionDepth;

  void TraverseAndOrder(int cellId);
  void MarkAndReplace(int cellId, int n, int replacement);
};

#endif


