/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBoostKruskalMinimumSpanningTree.h

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
// .NAME vtkBoostKruskalMinimumSpanningTree - Contructs a minimum spanning
//    tree from a graph and the weighting array
//
// .SECTION Description
//
// This vtk class uses the Boost Kruskal Minimum Spanning Tree 
// generic algorithm to perform a minimum spanning tree creation given
// a weighting value for each of the edges in the input graph.
//
// .SECTION See Also
// vtkGraph vtkBoostGraphAdapter

#ifndef __vtkBoostKruskalMinimumSpanningTree_h
#define __vtkBoostKruskalMinimumSpanningTree_h

#include "vtkStdString.h" // For string type
#include "vtkVariant.h" // For variant type

#include "vtkSelectionAlgorithm.h"

class VTK_INFOVIS_EXPORT vtkBoostKruskalMinimumSpanningTree : public vtkSelectionAlgorithm 
{
public:
  static vtkBoostKruskalMinimumSpanningTree *New();
  vtkTypeRevisionMacro(vtkBoostKruskalMinimumSpanningTree, vtkSelectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the name of the edge-weight input array, which must name an
  // array that is part of the edge data of the input graph and
  // contains numeric data. If the edge-weight array is not of type
  // vtkDoubleArray, the array will be copied into a temporary
  // vtkDoubleArray.
  vtkSetStringMacro(EdgeWeightArrayName);

  // Description:
  // Set the output selection type. The default is to use the
  // the set of minimum spanning tree edges "MINIMUM_SPANNING_TREE_EDGES". No
  // other options are defined.
  vtkSetStringMacro(OutputSelectionType);

protected:
  vtkBoostKruskalMinimumSpanningTree();
  ~vtkBoostKruskalMinimumSpanningTree();

  int RequestData(
      vtkInformation *,
      vtkInformationVector **,
      vtkInformationVector *);

  int FillInputPortInformation(
      int port, vtkInformation* info);

  int FillOutputPortInformation(
      int port, vtkInformation* info);
  
private:
  char* EdgeWeightArrayName;
  char* OutputSelectionType;
  
  vtkBoostKruskalMinimumSpanningTree(const vtkBoostKruskalMinimumSpanningTree&);  // Not implemented.
  void operator=(const vtkBoostKruskalMinimumSpanningTree&);  // Not implemented.
};

#endif
