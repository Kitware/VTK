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
// .NAME vtkRandomGraphSource - a graph with random arcs
//
// .SECTION Description
// Generates a graph with a specified number of nodes, with the density of
// arcs specified by either an exact number of arcs or the probability of
// an arc.  You may additionally specify whether to begin with a random
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
  // The number of nodes in the graph.
  vtkGetMacro(NumberOfNodes, int);
  vtkSetClampMacro(NumberOfNodes, int, 0, VTK_INT_MAX);

  // Description:
  // If UseArcProbability is off, creates a graph with the specified number
  // of arcs.  Duplicate (parallel) arcs are allowed.
  vtkGetMacro(NumberOfArcs, int);
  vtkSetClampMacro(NumberOfArcs, int, 0, VTK_INT_MAX);

  // Description:
  // If UseArcProbability is on, adds an arc with this probability between 0 and 1
  // for each pair of nodes in the graph.
  vtkGetMacro(ArcProbability, double);
  vtkSetClampMacro(ArcProbability, double, 0.0, 1.0);

  // Description:
  // When set, creates a directed graph, as opposed to an undirected graph.
  vtkSetMacro(Directed, bool);
  vtkGetMacro(Directed, bool);
  vtkBooleanMacro(Directed, bool);

  // Description:
  // When set, uses the ArcProbability parameter to determine the density
  // of arcs.  Otherwise, NumberOfArcs is used.
  vtkSetMacro(UseArcProbability, bool);
  vtkGetMacro(UseArcProbability, bool);
  vtkBooleanMacro(UseArcProbability, bool);

  // Description:
  // When set, builds a random tree structure first, then adds additional
  // random arcs.
  vtkSetMacro(StartWithTree, bool);
  vtkGetMacro(StartWithTree, bool);
  vtkBooleanMacro(StartWithTree, bool);

protected:
  vtkRandomGraphSource();
  ~vtkRandomGraphSource();
  int NumberOfNodes;
  int NumberOfArcs;
  double ArcProbability;
  bool Directed;
  bool UseArcProbability;
  bool StartWithTree;

  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);

private:
  vtkRandomGraphSource(const vtkRandomGraphSource&); // Not implemented
  void operator=(const vtkRandomGraphSource&);   // Not implemented
};

#endif

