/*=========================================================================

  Program:   Visualization Library
  Module:    FeatEdge.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// Generates edges that are either 1) boundary (used by one polygon)
// 2) non-manifold (used by three or more polygons) 3) feature edges (edges 
// who are used by two triangles and the dihedral angle > FeatureAngle).
//
#ifndef __vlFeatureEdges_h
#define __vlFeatureEdges_h

#include "P2PF.hh"

class vlFeatureEdges : public vlPolyToPolyFilter
{
public:
  vlFeatureEdges();
  ~vlFeatureEdges() {};
  char *GetClassName() {return "vlFeatureEdges";};
  void PrintSelf(ostream& os, vlIndent indent);

  vlSetClampMacro(FeatureAngle,float,0.0,180.0);
  vlGetMacro(FeatureAngle,float);

  vlSetMacro(BoundaryEdges,int);
  vlGetMacro(BoundaryEdges,int);
  vlBooleanMacro(BoundaryEdges,int);

  vlSetMacro(FeatureEdges,int);
  vlGetMacro(FeatureEdges,int);
  vlBooleanMacro(FeatureEdges,int);

  vlSetMacro(NonManifoldEdges,int);
  vlGetMacro(NonManifoldEdges,int);
  vlBooleanMacro(NonManifoldEdges,int);

  vlSetMacro(Coloring,int);
  vlGetMacro(Coloring,int);
  vlBooleanMacro(Coloring,int);

protected:
  // Usual data generation method
  void Execute();

  float FeatureAngle;
  int BoundaryEdges;
  int FeatureEdges;
  int NonManifoldEdges;
  int Coloring;
};

#endif


