/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCollapseGraph.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

// .NAME vtkCollapseGraph - "Collapses" vertices onto their neighbors.
// .SECTION Description
//
// vtkCollapseGraph "collapses" vertices onto their neighbors, while maintaining
// connectivity.  Two inputs are required - a graph (directed or undirected),
// and a vertex selection that can be converted to indices.
//
// Conceptually, each of the vertices specified in the input selection
// expands, "swallowing" adacent vertices.  Edges to-or-from the "swallowed"
// vertices become edges to-or-from the expanding vertices, maintaining the
// overall graph connectivity.
//
// In the case of directed graphs, expanding vertices only swallow vertices that
// are connected via out edges.  This rule provides intuitive behavior when
// working with trees, so that "child" vertices collapse into their parents
// when the parents are part of the input selection.
//
// Input port 0: graph
// Input port 1: selection

#ifndef __vtkCollapseGraph_h
#define __vtkCollapseGraph_h

#include "vtkInfovisCoreModule.h" // For export macro
#include "vtkGraphAlgorithm.h"

class VTKINFOVISCORE_EXPORT vtkCollapseGraph : public vtkGraphAlgorithm
{
public:
  static vtkCollapseGraph* New();
  vtkTypeMacro(vtkCollapseGraph,vtkGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Convenience function provided for setting the graph input.
  void SetGraphConnection(vtkAlgorithmOutput*);
  /// Convenience function provided for setting the selection input.
  void SetSelectionConnection(vtkAlgorithmOutput*);

protected:
  vtkCollapseGraph();
  ~vtkCollapseGraph();

  int FillInputPortInformation(int port, vtkInformation* info);

  int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);

private:
  vtkCollapseGraph(const vtkCollapseGraph&); // Not implemented
  void operator=(const vtkCollapseGraph&);   // Not implemented
};

#endif

