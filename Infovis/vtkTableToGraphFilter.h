/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTableToGraphFilter.h

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
// .NAME vtkTableToGraphFilter - convert a vtkTable into a vtkGraph
//
// .SECTION Description
//
// Creates a vtkGraph using one or two vtkTables.  The first (required)
// input table must have one row for each arc in the graph.
// The table must have two columns which represent the source and target
// node ids.  Use 
//
// SetInputArrayToProcess(i,0,0,vtkDataObject::FIELD_ASSOCIATION_NONE,"name")
//
// to specify these fields, where i=0 is the source field, and i=1 is the
// target field.
//
// The second (optional) vtkTable has one row for each node in the graph.
// The table must have a field whose values match those in the arc table.
// Use 
//
// SetInputArrayToProcess(2,1,0,vtkDataObject::FIELD_ASSOCIATION_NONE,"name")
//
// to specify the node index field. If the node table is not given, 
// a node will be created for each unique source or target identifier 
// in the arc table.
//
// Input arrays 0, 1 and 2 must be of the same type, and must be either
// vtkStringArray or a subclass of vtkDataArray.
//
// All columns in the tables, including the source, target, and node index
// fields, are copied into the arc data and node data of the resulting
// vtkGraph.  If the node table is not given, the node data will contain
// a single "id" column with the same type as the source/target id arrays.
//
// If parallel arcs are collected, not all the arc data is not copied into 
// the output.  Only the source and target id arrays will be transferred.
// An additional vtkIdTypeArray column called "weight" is created which 
// contains the number of times each arc appeared in the input.
//
// If the node table contains positional data, the user may specify these
// with input arrays 3, 4 and 5 for x, y, and z-coordinates, respectively.
// These arrays must be data arrays.  The z-coordinate array is optional,
// and if not given the z-coordinates are set to zero.

#ifndef __vtkTableToGraphFilter_h
#define __vtkTableToGraphFilter_h

#include "vtkGraphAlgorithm.h"

class vtkGraph;
class vtkGraphIdList;
class vtkIdTypeArray;

class VTK_INFOVIS_EXPORT vtkTableToGraphFilter : public vtkGraphAlgorithm
{
public:
  static vtkTableToGraphFilter* New();
  vtkTypeRevisionMacro(vtkTableToGraphFilter,vtkGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // When set, creates a directed graph, as opposed to an undirected graph.
  vtkSetMacro(Directed, bool);
  vtkGetMacro(Directed, bool);
  vtkBooleanMacro(Directed, bool);

  // Description:
  // When set, creates a graph with no parallel arcs.
  // Parallel arcs are combined into one arc.
  // No cell fields are passed to the output, but a new field "weight"
  // is created that holds the number of duplicates of that arc in the input.
  vtkSetMacro(CollapseEdges, bool);
  vtkGetMacro(CollapseEdges, bool);
  vtkBooleanMacro(CollapseEdges, bool);

  // Description:
  // Specify the first required vtkTable input and the second
  // optional vtkTable input.
  int FillInputPortInformation(int port, vtkInformation* info);

protected:
  vtkTableToGraphFilter();
  ~vtkTableToGraphFilter();

  bool Directed;
  bool CollapseEdges;
  vtkGraphIdList* Adj;
  vtkGraphIdList* Incident;

  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);

  vtkIdType AppendEdge(
    vtkGraph* output, 
    vtkIdType sourceNode, 
    vtkIdType targetNode, 
    vtkIdTypeArray* weightArr);

private:
  vtkTableToGraphFilter(const vtkTableToGraphFilter&); // Not implemented
  void operator=(const vtkTableToGraphFilter&);   // Not implemented
};

#endif

