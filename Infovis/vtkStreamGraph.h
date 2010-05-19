/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStreamGraph.h

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
// .NAME vtkStreamGraph - combines two graphs
//
// .SECTION Description
// vtkStreamGraph iteratively collects information from the input graph
// and combines it in the output graph. It internally maintains a graph
// instance that is incrementally updated every time the filter is called.
//
// Each update, vtkMergeGraphs is used to combine this filter's input with the
// internal graph.

#ifndef __vtkStreamGraph_h
#define __vtkStreamGraph_h

#include "vtkGraphAlgorithm.h"

class vtkBitArray;
class vtkMergeGraphs;
class vtkMutableDirectedGraph;
class vtkMutableGraphHelper;
class vtkStringArray;
class vtkTable;

class VTK_INFOVIS_EXPORT vtkStreamGraph : public vtkGraphAlgorithm
{
public:
  static vtkStreamGraph* New();
  vtkTypeMacro(vtkStreamGraph,vtkGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The maximum number of edges in the combined graph. Default is -1,
  // which specifies that there should be no limit on the number
  // of edges.
  vtkSetMacro(MaxEdges, vtkIdType);
  vtkGetMacro(MaxEdges, vtkIdType);

protected:
  vtkStreamGraph();
  ~vtkStreamGraph();

  virtual int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);

  vtkMutableGraphHelper* CurrentGraph;
  vtkMergeGraphs* MergeGraphs;
  vtkIdType MaxEdges;

private:
  vtkStreamGraph(const vtkStreamGraph&); // Not implemented
  void operator=(const vtkStreamGraph&);   // Not implemented
};

#endif

