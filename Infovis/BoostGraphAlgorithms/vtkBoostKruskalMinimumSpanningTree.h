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
/**
 * @class   vtkBoostKruskalMinimumSpanningTree
 * @brief   Contructs a minimum spanning
 *    tree from a graph and the weighting array
 *
 *
 *
 * This vtk class uses the Boost Kruskal Minimum Spanning Tree
 * generic algorithm to perform a minimum spanning tree creation given
 * a weighting value for each of the edges in the input graph.
 *
 * @sa
 * vtkGraph vtkBoostGraphAdapter
*/

#ifndef vtkBoostKruskalMinimumSpanningTree_h
#define vtkBoostKruskalMinimumSpanningTree_h

#include "vtkInfovisBoostGraphAlgorithmsModule.h" // For export macro
#include "vtkStdString.h" // For string type
#include "vtkVariant.h" // For variant type

#include "vtkSelectionAlgorithm.h"

class VTKINFOVISBOOSTGRAPHALGORITHMS_EXPORT vtkBoostKruskalMinimumSpanningTree : public vtkSelectionAlgorithm
{
public:
  static vtkBoostKruskalMinimumSpanningTree *New();
  vtkTypeMacro(vtkBoostKruskalMinimumSpanningTree, vtkSelectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Set the name of the edge-weight input array, which must name an
   * array that is part of the edge data of the input graph and
   * contains numeric data. If the edge-weight array is not of type
   * vtkDoubleArray, the array will be copied into a temporary
   * vtkDoubleArray.
   */
  vtkSetStringMacro(EdgeWeightArrayName);
  //@}

  //@{
  /**
   * Set the output selection type. The default is to use the
   * the set of minimum spanning tree edges "MINIMUM_SPANNING_TREE_EDGES". No
   * other options are defined.
   */
  vtkSetStringMacro(OutputSelectionType);
  //@}

  //@{
  /**
   * Whether to negate the edge weights. By negating the edge
   * weights this algorithm will give you the 'maximal' spanning
   * tree (i.e. the algorithm will try to create a spanning tree
   * with the highest weighted edges). Defaulted to Off.
   * FIXME: put a real definition in...
   */
  void SetNegateEdgeWeights(bool value);
  vtkGetMacro(NegateEdgeWeights, bool);
  vtkBooleanMacro(NegateEdgeWeights, bool);
  //@}

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
  bool NegateEdgeWeights;
  float EdgeWeightMultiplier;

  vtkBoostKruskalMinimumSpanningTree(const vtkBoostKruskalMinimumSpanningTree&) VTK_DELETE_FUNCTION;
  void operator=(const vtkBoostKruskalMinimumSpanningTree&) VTK_DELETE_FUNCTION;
};

#endif
