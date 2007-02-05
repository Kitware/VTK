/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRandomGraphSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkRandomGraphSource - a graph with random edges
//
// .SECTION Description
// Generates a graph with a specified number of vertices, with the density of
// edges specified by either an exact number of edges or the probability of
// an edge.  You may additionally specify whether to begin with a random
// tree (which enforces graph connectivity).
//

#ifndef __vtkRandomGraphSource_h
#define __vtkRandomGraphSource_h

#include "vtkGraphAlgorithm.h"

class vtkGraph;
class vtkPVXMLElement;

class VTK_INFOVIS_EXPORT vtkRandomGraphSource : public vtkGraphAlgorithm
{
public:
  static vtkRandomGraphSource* New();
  vtkTypeRevisionMacro(vtkRandomGraphSource,vtkGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The number of vertices in the graph.
  vtkGetMacro(NumberOfVertices, int);
  vtkSetClampMacro(NumberOfVertices, int, 0, VTK_INT_MAX);

  // Description:
  // If UseEdgeProbability is off, creates a graph with the specified number
  // of edges.  Duplicate (parallel) edges are allowed.
  vtkGetMacro(NumberOfEdges, int);
  vtkSetClampMacro(NumberOfEdges, int, 0, VTK_INT_MAX);

  // Description:
  // If UseEdgeProbability is on, adds an edge with this probability between 0 and 1
  // for each pair of vertices in the graph.
  vtkGetMacro(EdgeProbability, double);
  vtkSetClampMacro(EdgeProbability, double, 0.0, 1.0);

  // Description:
  // When set, includes edge weights in an array named "edge_weights".
  // Defaults to off.  Weights are random between 0 and 1.
  vtkSetMacro(IncludeEdgeWeights, bool);
  vtkGetMacro(IncludeEdgeWeights, bool);
  vtkBooleanMacro(IncludeEdgeWeights, bool);

  // Description:
  // When set, creates a directed graph, as opposed to an undirected graph.
  vtkSetMacro(Directed, bool);
  vtkGetMacro(Directed, bool);
  vtkBooleanMacro(Directed, bool);

  // Description:
  // When set, uses the EdgeProbability parameter to determine the density
  // of edges.  Otherwise, NumberOfEdges is used.
  vtkSetMacro(UseEdgeProbability, bool);
  vtkGetMacro(UseEdgeProbability, bool);
  vtkBooleanMacro(UseEdgeProbability, bool);

  // Description:
  // When set, builds a random tree structure first, then adds additional
  // random edges.
  vtkSetMacro(StartWithTree, bool);
  vtkGetMacro(StartWithTree, bool);
  vtkBooleanMacro(StartWithTree, bool);

  // Description:
  // If this flag is set to true, edges where the source and target
  // vertex are the same can be generated.  The default is to forbid
  // such loops.
  vtkSetMacro(AllowSelfLoops, bool);
  vtkGetMacro(AllowSelfLoops, bool);
  vtkBooleanMacro(AllowSelfLoops, bool);

protected:
  vtkRandomGraphSource();
  ~vtkRandomGraphSource();
  int NumberOfVertices;
  int NumberOfEdges;
  double EdgeProbability;
  bool Directed;
  bool UseEdgeProbability;
  bool StartWithTree;
  bool IncludeEdgeWeights;
  bool AllowSelfLoops;

  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);

private:
  vtkRandomGraphSource(const vtkRandomGraphSource&); // Not implemented
  void operator=(const vtkRandomGraphSource&);   // Not implemented
};

#endif

