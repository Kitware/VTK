/*=========================================================================

  Program:   Visualization Toolkit
  Module:    FeatVert.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkFeatureVertices - extract boundary, non-manifold, and/or sharp vertices from polygonal data (operates on line primitives)
// .SECTION Description
// vtkFeatureVertices is a filter to extract special types of vertices from
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
// may require pre-processing with vtkCleanPolyData to merge coincident points.
// Otherwise points may be flagged as boundary. (This is true when running
// vtkFeatureEdges and then vtkFeatureVertices).
// .SECTION See Also
// vtkFeatureEdges

#ifndef __vtkFeatureVertices_h
#define __vtkFeatureVertices_h

#include "P2PF.hh"

class vtkFeatureVertices : public vtkPolyToPolyFilter
{
public:
  vtkFeatureVertices();
  ~vtkFeatureVertices() {};
  char *GetClassName() {return "vtkFeatureVertices";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Turn on/off the extraction of boundary vertices.
  vtkSetMacro(BoundaryVertices,int);
  vtkGetMacro(BoundaryVertices,int);
  vtkBooleanMacro(BoundaryVertices,int);

  // Description:
  // Turn on/off the extraction of feature vertices.
  vtkSetMacro(FeatureVertices,int);
  vtkGetMacro(FeatureVertices,int);
  vtkBooleanMacro(FeatureVertices,int);

  // Description:
  // Specify the feature angle for extracting feature vertices.
  vtkSetClampMacro(FeatureAngle,float,0.0,180.0);
  vtkGetMacro(FeatureAngle,float);

  // Description:
  // Turn on/off the extraction of non-manifold vertices.
  vtkSetMacro(NonManifoldVertices,int);
  vtkGetMacro(NonManifoldVertices,int);
  vtkBooleanMacro(NonManifoldVertices,int);

  // Description:
  // Turn on/off the coloring of vertices by type.
  vtkSetMacro(Coloring,int);
  vtkGetMacro(Coloring,int);
  vtkBooleanMacro(Coloring,int);

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


