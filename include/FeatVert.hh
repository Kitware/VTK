/*=========================================================================

  Program:   Visualization Library
  Module:    FeatVert.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlFeatureVertices - extract boundary, non-manifold, and/or sharp vertices from polygonal data (operates on line primitives)
// .SECTION Description
// vlFeatureVertices is a filter to extract special types of vertices from
// input polygonal data. In particular, the filter operates on the line
// primitives in the polygonal data. The vertex types are: 1) boundary 
// (used by one line) or a vertex cell type, 2) non-manifold (used by three 
// or more lines lines), or 3) feature edges (vertices used by two lines 
// and whose orientation angle > FeatureAngle). The orientation angle is 
// computed from the dot product between the two lines. These vertices may 
// be extracted in any combination. Vertices may also be "colored" (i.e., 
// scalar values assigned) based on vertex type.
// .SECTION Caveats
// This filter operates only on line primitives in polygonal data. Some data
// may require pre-processing with vlCleanPolyData to merge coincident points.
// Otherwise points may be flagged as boundary. (This is true when running
// vlFeatureEdges and then vlFeatureVertices).
// .SECTION See Also
// vlFeatureEdges

#ifndef __vlFeatureVertices_h
#define __vlFeatureVertices_h

#include "P2PF.hh"

class vlFeatureVertices : public vlPolyToPolyFilter
{
public:
  vlFeatureVertices();
  ~vlFeatureVertices() {};
  char *GetClassName() {return "vlFeatureVertices";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Turn on/off the extraction of boundary vertices.
  vlSetMacro(BoundaryVertices,int);
  vlGetMacro(BoundaryVertices,int);
  vlBooleanMacro(BoundaryVertices,int);

  // Description:
  // Turn on/off the extraction of feature vertices.
  vlSetMacro(FeatureVertices,int);
  vlGetMacro(FeatureVertices,int);
  vlBooleanMacro(FeatureVertices,int);

  // Description:
  // Specify the feature angle for extracting feature vertices.
  vlSetClampMacro(FeatureAngle,float,0.0,180.0);
  vlGetMacro(FeatureAngle,float);

  // Description:
  // Turn on/off the extraction of non-manifold vertices.
  vlSetMacro(NonManifoldVertices,int);
  vlGetMacro(NonManifoldVertices,int);
  vlBooleanMacro(NonManifoldVertices,int);

  // Description:
  // Turn on/off the coloring of vertices by type.
  vlSetMacro(Coloring,int);
  vlGetMacro(Coloring,int);
  vlBooleanMacro(Coloring,int);

protected:
  // Usual data generation method
  void Execute();

  float FeatureAngle;
  int BoundaryVertices;
  int FeatureVertices;
  int NonManifoldVertices;
  int Coloring;
};

#endif


