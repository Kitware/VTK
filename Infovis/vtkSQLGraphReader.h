/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSQLGraphReader.h

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
// .NAME vtkSQLGraphReader - read a vtkGraph from a database
//
// .SECTION Description
//
// Creates a vtkGraph using one or two vtkSQLQuery's.  The first (required)
// query must have one row for each arc in the graph.
// The query must have two columns which represent the source and target
// node ids.
//
// The second (optional) query has one row for each node in the graph.
// The table must have a field whose values match those in the arc table.
// If the node table is not given, 
// a node will be created for each unique source or target identifier 
// in the arc table.
//
// The source, target, and node ID fields must be of the same type, 
// and must be either vtkStringArray or a subclass of vtkDataArray.
//
// All columns in the queries, including the source, target, and node index
// fields, are copied into the arc data and node data of the resulting
// vtkGraph.  If the node query is not given, the node data will contain
// a single "id" column with the same type as the source/target id arrays.
//
// If parallel arcs are collected, not all the arc data is not copied into 
// the output.  Only the source and target id arrays will be transferred.
// An additional vtkIdTypeArray column called "weight" is created which 
// contains the number of times each arc appeared in the input.
//
// If the node query contains positional data, the user may specify the
// names of these fields.
// These arrays must be data arrays.  The z-coordinate array is optional,
// and if not given the z-coordinates are set to zero.

#ifndef __vtkSQLGraphReader_h
#define __vtkSQLGraphReader_h

#include "vtkGraphAlgorithm.h"

class vtkSQLQuery;

class VTK_INFOVIS_EXPORT vtkSQLGraphReader : public vtkGraphAlgorithm
{
public:
  static vtkSQLGraphReader* New();
  vtkTypeMacro(vtkSQLGraphReader,vtkGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // When set, creates a directed graph, as opposed to an undirected graph.
  vtkSetMacro(Directed, bool);
  vtkGetMacro(Directed, bool);
  vtkBooleanMacro(Directed, bool);

  // Description:
  // The query that retrieves the node information.
  virtual void SetVertexQuery(vtkSQLQuery* q);
  vtkGetObjectMacro(VertexQuery, vtkSQLQuery);

  // Description:
  // The query that retrieves the arc information.
  virtual void SetEdgeQuery(vtkSQLQuery* q);
  vtkGetObjectMacro(EdgeQuery, vtkSQLQuery);

  // Description:
  // The name of the field in the arc query for the source node of each arc.
  vtkSetStringMacro(SourceField);
  vtkGetStringMacro(SourceField);

  // Description:
  // The name of the field in the arc query for the target node of each arc.
  vtkSetStringMacro(TargetField);
  vtkGetStringMacro(TargetField);

  // Description:
  // The name of the field in the node query for the node ID.
  vtkSetStringMacro(VertexIdField);
  vtkGetStringMacro(VertexIdField);

  // Description:
  // The name of the field in the node query for the node's x coordinate.
  vtkSetStringMacro(XField);
  vtkGetStringMacro(XField);

  // Description:
  // The name of the field in the node query for the node's y coordinate.
  vtkSetStringMacro(YField);
  vtkGetStringMacro(YField);

  // Description:
  // The name of the field in the node query for the node's z coordinate.
  vtkSetStringMacro(ZField);
  vtkGetStringMacro(ZField);

  // Description:
  // When set, creates a graph with no parallel arcs.
  // Parallel arcs are combined into one arc.
  // No cell fields are passed to the output, except the vtkGhostLevels array if
  // it exists, but a new field "weight" is created that holds the number of 
  // duplicates of that arc in the input.
  vtkSetMacro(CollapseEdges, bool);
  vtkGetMacro(CollapseEdges, bool);
  vtkBooleanMacro(CollapseEdges, bool);

protected:
  vtkSQLGraphReader();
  ~vtkSQLGraphReader();

  bool Directed;
  bool CollapseEdges;
  vtkSQLQuery* EdgeQuery;
  vtkSQLQuery* VertexQuery;
  char* SourceField;
  char* TargetField;
  char* VertexIdField;
  char* XField;
  char* YField;
  char* ZField;

  virtual int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);

  virtual int RequestDataObject(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);

private:
  vtkSQLGraphReader(const vtkSQLGraphReader&); // Not implemented
  void operator=(const vtkSQLGraphReader&);   // Not implemented
};

#endif

