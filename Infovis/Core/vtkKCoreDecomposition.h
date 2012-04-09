/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKCoreDecomposition.h

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
// .NAME vtkKCoreDecomposition - Compute the k-core decomposition of the input graph.
//
// .SECTION Description
// The k-core decomposition is a graph partitioning strategy that is useful for
// analyzing the structure of large networks. A k-core of a graph G is a maximal
// connected subgraph of G in which all vertices have degree at least k.  The k-core
// membership for each vertex of the input graph is found on the vertex data of the
// output graph as an array named 'KCoreDecompositionNumbers' by default.  The algorithm
// used to find the k-cores has O(number of graph edges) running time, and is described
// in the following reference paper.
//
// An O(m) Algorithm for Cores Decomposition of Networks
//   V. Batagelj, M. Zaversnik, 2001
//
// .SECTION Thanks
// Thanks to Thomas Otahal from Sandia National Laboratories for providing this
// implementation.

#ifndef __vtkKCoreDecomposition_h
#define __vtkKCoreDecomposition_h

#include "vtkGraphAlgorithm.h"

class vtkIntArray;

class VTK_INFOVIS_EXPORT vtkKCoreDecomposition : public vtkGraphAlgorithm
{
public:
  static vtkKCoreDecomposition *New();

  vtkTypeMacro(vtkKCoreDecomposition, vtkGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the output array name. If no output array name is
  // set then the name 'KCoreDecompositionNumbers' is used.
  vtkSetStringMacro(OutputArrayName);

  // Description:
  // Directed graphs only.  Use only the in edges to
  // compute the vertex degree of a vertex.  The default
  // is to use both in and out edges to compute vertex
  // degree.
  vtkSetMacro(UseInDegreeNeighbors, bool);
  vtkGetMacro(UseInDegreeNeighbors, bool);
  vtkBooleanMacro(UseInDegreeNeighbors, bool);

  // Description:
  // Directed graphs only.  Use only the out edges to
  // compute the vertex degree of a vertex.  The default
  // is to use both in and out edges to compute vertex
  // degree.
  vtkSetMacro(UseOutDegreeNeighbors, bool);
  vtkGetMacro(UseOutDegreeNeighbors, bool);
  vtkBooleanMacro(UseOutDegreeNeighbors, bool);

  // Description:
  // Check the input graph for self loops and parallel
  // edges.  The k-core is not defined for graphs that
  // contain either of these.  Default is on.
  vtkSetMacro(CheckInputGraph, bool);
  vtkGetMacro(CheckInputGraph, bool);
  vtkBooleanMacro(CheckInputGraph, bool);

protected:
  vtkKCoreDecomposition();
  ~vtkKCoreDecomposition();

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:

  char* OutputArrayName;

  bool UseInDegreeNeighbors;
  bool UseOutDegreeNeighbors;
  bool CheckInputGraph;

  // K-core partitioning implementation
  void Cores(vtkGraph* g,
             vtkIntArray* KCoreNumbers);

  vtkKCoreDecomposition(const vtkKCoreDecomposition&);  // Not implemented.
  void operator=(const vtkKCoreDecomposition&);  // Not implemented.
};

#endif
