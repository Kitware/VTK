/*=========================================================================

  Program:   Visualization Library
  Module:    FeatEdge.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlFeatureEdges - extract boundary, non-manifold, and/or sharp edges from polygonal data
// .SECTION Description
// vlFeatureEdges is a filter to extract special types edges of edges from
// input polygonal data. These edges that are either 1) boundary (used by 
// one polygon) or a line cell, 2) non-manifold (used by three or more 
// polygons) 3) feature edges (edges used by two triangles and whose
// dihedral angle > FeatureAngle). These edges may be extracted in any
// combination. Edges may also be "colored" (i.e., scalar values assigned)
// based on edge type.

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

  // Description:
  // Turn on/off the extraction of boundary edges.
  vlSetMacro(BoundaryEdges,int);
  vlGetMacro(BoundaryEdges,int);
  vlBooleanMacro(BoundaryEdges,int);

  // Description:
  // Turn on/off the extraction of feature edges.
  vlSetMacro(FeatureEdges,int);
  vlGetMacro(FeatureEdges,int);
  vlBooleanMacro(FeatureEdges,int);

  // Description:
  // Specify the feature angle for extracting feature edges.
  vlSetClampMacro(FeatureAngle,float,0.0,180.0);
  vlGetMacro(FeatureAngle,float);

  // Description:
  // Turn on/off the extraction of non-manifold edges.
  vlSetMacro(NonManifoldEdges,int);
  vlGetMacro(NonManifoldEdges,int);
  vlBooleanMacro(NonManifoldEdges,int);

  // Description:
  // Turn on/off the coloring of edges by type.
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


