/*=========================================================================

  Program:   Visualization Toolkit
  Module:    FeatEdge.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkFeatureEdges - extract boundary, non-manifold, and/or sharp edges from polygonal data
// .SECTION Description
// vtkFeatureEdges is a filter to extract special types edges of edges from
// input polygonal data. These edges that are either 1) boundary (used by 
// one polygon) or a line cell, 2) non-manifold (used by three or more 
// polygons) 3) feature edges (edges used by two triangles and whose
// dihedral angle > FeatureAngle). These edges may be extracted in any
// combination. Edges may also be "colored" (i.e., scalar values assigned)
// based on edge type.

#ifndef __vtkFeatureEdges_h
#define __vtkFeatureEdges_h

#include "P2PF.hh"

class vtkFeatureEdges : public vtkPolyToPolyFilter
{
public:
  vtkFeatureEdges();
  ~vtkFeatureEdges() {};
  char *GetClassName() {return "vtkFeatureEdges";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Turn on/off the extraction of boundary edges.
  vtkSetMacro(BoundaryEdges,int);
  vtkGetMacro(BoundaryEdges,int);
  vtkBooleanMacro(BoundaryEdges,int);

  // Description:
  // Turn on/off the extraction of feature edges.
  vtkSetMacro(FeatureEdges,int);
  vtkGetMacro(FeatureEdges,int);
  vtkBooleanMacro(FeatureEdges,int);

  // Description:
  // Specify the feature angle for extracting feature edges.
  vtkSetClampMacro(FeatureAngle,float,0.0,180.0);
  vtkGetMacro(FeatureAngle,float);

  // Description:
  // Turn on/off the extraction of non-manifold edges.
  vtkSetMacro(NonManifoldEdges,int);
  vtkGetMacro(NonManifoldEdges,int);
  vtkBooleanMacro(NonManifoldEdges,int);

  // Description:
  // Turn on/off the coloring of edges by type.
  vtkSetMacro(Coloring,int);
  vtkGetMacro(Coloring,int);
  vtkBooleanMacro(Coloring,int);

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


