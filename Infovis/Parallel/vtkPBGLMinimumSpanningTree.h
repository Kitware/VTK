/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPBGLMinimumSpanningTree.h

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
 * @class   vtkPBGLMinimumSpanningTree
 * @brief   Minimum spanning tree of a
 * distributed vtkGraph.
 *
 *
 * This VTK class uses the Parallel BGL minimum spanning tree
 * generic algorithm to compute the minimum spanning tree of a weighted,
 * undirected graph (a distributed vtkGraph).
 *
 * @sa
 * vtkGraph vtkPBGLGraphAdaptor vtkBoostGraphAdapter
 *
 * @sa
 * @deprecated Not maintained as of VTK 6.2 and will be removed eventually.
*/

#ifndef vtkPBGLMinimumSpanningTree_h
#define vtkPBGLMinimumSpanningTree_h

#include "vtkInfovisParallelModule.h" // For export macro
#include "vtkStdString.h" // For string type
#include "vtkVariant.h" // For variant type

#include "vtkGraphAlgorithm.h"

class vtkSelection;

#if !defined(VTK_LEGACY_REMOVE)
class VTKINFOVISPARALLEL_EXPORT vtkPBGLMinimumSpanningTree : public vtkGraphAlgorithm
{
public:
  static vtkPBGLMinimumSpanningTree *New();
  vtkTypeMacro(vtkPBGLMinimumSpanningTree, vtkGraphAlgorithm);
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

protected:
  vtkPBGLMinimumSpanningTree();
  ~vtkPBGLMinimumSpanningTree();

  virtual int RequestData(
    vtkInformation *,
    vtkInformationVector **,
    vtkInformationVector *);

  virtual int FillInputPortInformation(
    int port, vtkInformation* info);

  virtual int FillOutputPortInformation(
    int port, vtkInformation* info);

private:
  char* EdgeWeightArrayName;
  char* OutputSelectionType;

  vtkPBGLMinimumSpanningTree(const vtkPBGLMinimumSpanningTree&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPBGLMinimumSpanningTree&) VTK_DELETE_FUNCTION;
};

#endif //VTK_LEGACY_REMOVE
#endif
