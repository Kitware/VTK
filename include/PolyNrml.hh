/*=========================================================================

  Program:   Visualization Library
  Module:    PolyNrml.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlPolyNormals - compute normals for polygonal mes
// .SECTION Description
// vlPolyNormals is a filter that computes point normals for a polygonal 
// mesh. The filter can reorder polygons to insure consistent orientation
// across polygon neighbors. Sharp edges can be split and points duplicated
// with separate normals to give crisp (rendered) surface definition. It is
// also possible to globally flip the normal orientation.
//     The algorithm works by determing normals for each polyon and then
// averaging them at shared points. When sharp edges are present, the edges
// are split and new points generated to prevent blurry edges.

#ifndef __vlPolyNormals_h
#define __vlPolyNormals_h

#include "P2PF.hh"

class vlPolyNormals : public vlPolyToPolyFilter
{
public:
  vlPolyNormals();
  ~vlPolyNormals() {};
  char *GetClassName() {return "vlPolyNormals";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Specify the angle that defines a sharp edge. If the difference in
  // angle across neighboring polygons is greater than this value, the
  // shared edge is considered "sharp".
  vlSetClampMacro(FeatureAngle,float,0.0,180.0);
  vlGetMacro(FeatureAngle,float);

  // Description:
  // Turn on/off the splitting of sharp edges.
  vlSetMacro(Splitting,int);
  vlGetMacro(Splitting,int);
  vlBooleanMacro(Splitting,int);

  // Description:
  // Turn on/off the enforcement of consistent polygon ordering.
  vlSetMacro(Consistency,int);
  vlGetMacro(Consistency,int);
  vlBooleanMacro(Consistency,int);

  // Description:
  // Turn on/off the global flipping of normal orientation.
  vlSetMacro(FlipNormals,int);
  vlGetMacro(FlipNormals,int);
  vlBooleanMacro(FlipNormals,int);

  // Description:
  // Control the depth of recursion used in this algorithm. (Some systems
  // have limited stack depth.)
  vlSetClampMacro(RecursionDepth,int,10,LARGE_INTEGER);
  vlGetMacro(RecursionDepth,int);

protected:
  // Usual data generation method
  void Execute();

  float FeatureAngle;
  int Splitting;
  int Consistency;
  int FlipNormals;
  int RecursionDepth;

  void TraverseAndOrder(int cellId);
  void MarkAndReplace(int cellId, int n, int replacement);
};

#endif


