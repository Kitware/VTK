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
//
// If you can use an edge window array to filter out old edges based on a
// moving threshold.

#ifndef vtkStreamGraph_h
#define vtkStreamGraph_h

#include "vtkInfovisCoreModule.h" // For export macro
#include "vtkGraphAlgorithm.h"

class vtkBitArray;
class vtkMergeGraphs;
class vtkMutableDirectedGraph;
class vtkMutableGraphHelper;
class vtkStringArray;
class vtkTable;

class VTKINFOVISCORE_EXPORT vtkStreamGraph : public vtkGraphAlgorithm
{
public:
  static vtkStreamGraph* New();
  vtkTypeMacro(vtkStreamGraph,vtkGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Whether to use an edge window array. The default is to
  // not use a window array.
  vtkSetMacro(UseEdgeWindow, bool);
  vtkGetMacro(UseEdgeWindow, bool);
  vtkBooleanMacro(UseEdgeWindow, bool);

  // Description:
  // The edge window array. The default array name is "time".
  vtkSetStringMacro(EdgeWindowArrayName);
  vtkGetStringMacro(EdgeWindowArrayName);

  // Description:
  // The time window amount. Edges with values lower
  // than the maximum value minus this window will be
  // removed from the graph. The default edge window is
  // 10000.
  vtkSetMacro(EdgeWindow, double);
  vtkGetMacro(EdgeWindow, double);

protected:
  vtkStreamGraph();
  ~vtkStreamGraph();

  virtual int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);

  vtkMutableGraphHelper* CurrentGraph;
  vtkMergeGraphs* MergeGraphs;
  bool UseEdgeWindow;
  double EdgeWindow;
  char* EdgeWindowArrayName;

private:
  vtkStreamGraph(const vtkStreamGraph&); // Not implemented
  void operator=(const vtkStreamGraph&);   // Not implemented
};

#endif

