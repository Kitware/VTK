/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMergeGraphs.h

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
// .NAME vtkMergeGraphs - combines two graphs
//
// .SECTION Description
// vtkMergeGraphs combines information from two graphs into one.
// Both graphs must have pedigree ids assigned to the vertices.
// The output will contain the vertices/edges in the first graph, in
// addition to:
//
//  - vertices in the second graph whose pedigree id does not
//    match a vertex in the first input
//
//  - edges in the second graph
//
// The output will contain the same attribute structure as the input;
// fields associated only with the second input graph will not be passed
// to the output. When possible, the vertex/edge data for new vertices and
// edges will be populated with matching attributes on the second graph.
// To be considered a matching attribute, the array must have the same name,
// type, and number of components.
//
// .SECTION Caveats
// This filter is not "domain-aware". Pedigree ids are assumed to be globally
// unique, regardless of their domain.

#ifndef __vtkMergeGraphs_h
#define __vtkMergeGraphs_h

#include "vtkGraphAlgorithm.h"

class vtkBitArray;
class vtkMutableGraphHelper;
class vtkStringArray;
class vtkTable;

class VTK_INFOVIS_EXPORT vtkMergeGraphs : public vtkGraphAlgorithm
{
public:
  static vtkMergeGraphs* New();
  vtkTypeMacro(vtkMergeGraphs,vtkGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This is the core functionality of the algorithm. Adds edges
  // and vertices from g2 into g1.
  int ExtendGraph(vtkMutableGraphHelper* g1, vtkGraph* g2);

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
  vtkMergeGraphs();
  ~vtkMergeGraphs();

  virtual int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);

  virtual int FillInputPortInformation(int port, vtkInformation *info);

  bool UseEdgeWindow;
  char* EdgeWindowArrayName;
  double EdgeWindow;

private:
  vtkMergeGraphs(const vtkMergeGraphs&); // Not implemented
  void operator=(const vtkMergeGraphs&);   // Not implemented
};

#endif

