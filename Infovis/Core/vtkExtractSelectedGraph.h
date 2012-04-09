/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractSelectedGraph.h

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
// .NAME vtkExtractSelectedGraph - return a subgraph of a vtkGraph
//
// .SECTION Description
// The first input is a vtkGraph to take a subgraph from.
// The second input (optional) is a vtkSelection containing selected 
// indices. The third input (optional) is a vtkAnnotationsLayers whose 
// annotations contain selected specifying selected indices.
// The vtkSelection may have FIELD_TYPE set to POINTS (a vertex selection)
// or CELLS (an edge selection).  A vertex selection preserves all edges
// that connect selected vertices.  An edge selection preserves all vertices
// that are adjacent to at least one selected edge.  Alternately, you may
// indicate that an edge selection should maintain the full set of vertices,
// by turning RemoveIsolatedVertices off.

#ifndef __vtkExtractSelectedGraph_h
#define __vtkExtractSelectedGraph_h

#include "vtkGraphAlgorithm.h"

class vtkSelection;
class vtkDataSet;

class VTK_INFOVIS_EXPORT vtkExtractSelectedGraph : public vtkGraphAlgorithm
{
public:
  static vtkExtractSelectedGraph* New();
  vtkTypeMacro(vtkExtractSelectedGraph,vtkGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // A convenience method for setting the second input (i.e. the selection).
  void SetSelectionConnection(vtkAlgorithmOutput* in);

  // Description:
  // A convenience method for setting the third input (i.e. the annotation layers).
  void SetAnnotationLayersConnection(vtkAlgorithmOutput* in);
  
  // Description:
  // If set, removes vertices with no adjacent edges in an edge selection.
  // A vertex selection ignores this flag and always returns the full set 
  // of selected vertices.  Default is on.
  vtkSetMacro(RemoveIsolatedVertices, bool);
  vtkGetMacro(RemoveIsolatedVertices, bool);
  vtkBooleanMacro(RemoveIsolatedVertices, bool);

  // Description:
  // Specify the first vtkGraph input and the second vtkSelection input.
  int FillInputPortInformation(int port, vtkInformation* info);

protected:
  vtkExtractSelectedGraph();
  ~vtkExtractSelectedGraph();

  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);
  
  int RequestDataObject(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);
  
  bool RemoveIsolatedVertices;

private:
  vtkExtractSelectedGraph(const vtkExtractSelectedGraph&); // Not implemented
  void operator=(const vtkExtractSelectedGraph&);   // Not implemented
};

#endif

