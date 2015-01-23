/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphHierarchicalBundle.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// .NAME vtkGraphHierarchicalBundle - layout graph arcs in bundles
//
// .SECTION Description
// This algorithm creates a vtkPolyData from a vtkGraph.  As opposed to
// vtkGraphToPolyData, which converts each arc into a straight line, each arc
// is converted to a polyline, following a tree structure.  The filter requires
// both a vtkGraph and vtkTree as input.  The tree vertices must be a
// superset of the graph vertices.  A common example is when the graph vertices
// correspond to the leaves of the tree, but the internal vertices of the tree
// represent groupings of graph vertices.  The algorithm matches the vertices
// using the array "PedigreeId".  The user may alternately set the
// DirectMapping flag to indicate that the two structures must have directly
// corresponding offsets (i.e. node i in the graph must correspond to node i in
// the tree).
//
// The vtkGraph defines the topology of the output vtkPolyData (i.e.
// the connections between nodes) while the vtkTree defines the geometry (i.e.
// the location of nodes and arc routes).  Thus, the tree must have been
// assigned vertex locations, but the graph does not need locations, in fact
// they will be ignored.  The edges approximately follow the path from the
// source to target nodes in the tree.  A bundling parameter controls how
// closely the edges are bundled together along the tree structure.
//
// You may follow this algorithm with vtkSplineFilter in order to make nicely
// curved edges.

// .SECTION Thanks
// This algorithm was developed in the paper
// Danny Holten. Hierarchical Edge Bundles: Visualization of Adjacency Relations
// Relations in Hierarchical Data. IEEE Transactions on Visualization and
// Computer Graphics, Vol. 12, No. 5, 2006. pp. 741-748.

#ifndef vtkGraphHierarchicalBundle_h
#define vtkGraphHierarchicalBundle_h

#include "vtkPolyDataAlgorithm.h"

class VTK_INFOVIS_EXPORT vtkGraphHierarchicalBundle : public vtkPolyDataAlgorithm
{
public:
  static vtkGraphHierarchicalBundle *New();

  vtkTypeMacro(vtkGraphHierarchicalBundle,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The level of arc bundling in the graph.
  // A strength of 0 creates straight lines, while a strength of 1
  // forces arcs to pass directly through hierarchy node points.
  // The default value is 0.8.
  vtkSetClampMacro(BundlingStrength, double, 0.0, 1.0);
  vtkGetMacro(BundlingStrength, double);

  // Description:
  // If on, uses direct mapping from tree to graph vertices.
  // If off, both the graph and tree must contain PedigreeId arrays
  // which are used to match graph and tree vertices.
  // Default is off.
  vtkSetMacro(DirectMapping, bool);
  vtkGetMacro(DirectMapping, bool);
  vtkBooleanMacro(DirectMapping, bool);

  // Description:
  // Set the input type of the algorithm to vtkGraph.
  int FillInputPortInformation(int port, vtkInformation* info);

protected:
  vtkGraphHierarchicalBundle();
  ~vtkGraphHierarchicalBundle() {}

  double BundlingStrength;
  bool DirectMapping;

  // Description:
  // Convert the vtkGraph into vtkPolyData.
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
private:
  vtkGraphHierarchicalBundle(const vtkGraphHierarchicalBundle&);  // Not implemented.
  void operator=(const vtkGraphHierarchicalBundle&);  // Not implemented.
};

#endif
