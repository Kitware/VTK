/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTableToGraph.h

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
// .NAME vtkTableToGraph - convert a vtkTable into a vtkGraph
//
// .SECTION Description
// vtkTableToGraph converts a table to a graph using an auxiliary 
// link graph.  The link graph specifies how each row in the table
// should be converted to an edge, or a collection of edges.  It also
// specifies which columns of the table should be considered part of
// the same domain, and which columns should be hidden.
//
// A second, optional, table may be provided as the vertex table.
// This vertex table must have one or more domain columns whose values
// match values in the edge table.  The linked column name is specified in
// the domain array in the link graph.  The output graph will only contain
// vertices corresponding to a row in the vertex table.  For heterogenous
// graphs, you may want to use vtkMergeTables to create a single vertex table.
//
// The link graph contains the following arrays:
//
// (1) The "column" array has the names of the columns to connect in each table row.
// This array is required.
//
// (2) The optional "domain" array provides user-defined domain names for each column.
// Matching domains in multiple columns will merge vertices with the same
// value from those columns.  By default, all columns are in the same domain.
// If a vertex table is supplied, the domain indicates the column in the vertex
// table that the edge table column associates with.  If the user provides a
// vertex table but no domain names, the output will be an empty graph.
// Hidden columns do not need valid domain names.
//
// (3) The optional "hidden" array is a bit array specifying whether the column should be
// hidden.  The resulting graph will contain edges representing connections
// "through" the hidden column, but the vertices for that column will not 
// be present.  By default, no columns are hidden.  Hiding a column
// in a particular domain hides all columns in that domain.
//
// The output graph will contain three additional arrays in the vertex data.
// The "domain" column is a string array containing the domain of each vertex.
// The "label" column is a string version of the distinct value that, along
// with the domain, defines that vertex. The "ids" column also contains
// the distinguishing value, but as a vtkVariant holding the raw value instead
// of being converted to a string. The "ids" column is set as the vertex pedigree
// ID attribute.

#ifndef __vtkTableToGraph_h
#define __vtkTableToGraph_h

#include "vtkGraphAlgorithm.h"

class vtkBitArray;
class vtkMutableDirectedGraph;
class vtkStringArray;
class vtkTable;

class VTK_INFOVIS_EXPORT vtkTableToGraph : public vtkGraphAlgorithm
{
public:
  static vtkTableToGraph* New();
  vtkTypeMacro(vtkTableToGraph,vtkGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Add a vertex to the link graph.  Specify the column name, the domain name
  // for the column, and whether the column is hidden.
  void AddLinkVertex(const char* column, const char* domain = 0, int hidden = 0);

  // Description:
  // Clear the link graph vertices.  This also clears all edges.
  void ClearLinkVertices();

  // Description:
  // Add an edge to the link graph.  Specify the names of the columns to link.
  void AddLinkEdge(const char* column1, const char* column2);

  // Description:
  // Clear the link graph edges.  The graph vertices will remain.
  void ClearLinkEdges();

  // Description:
  // The graph describing how to link the columns in the table.
  vtkGetObjectMacro(LinkGraph, vtkMutableDirectedGraph);
  void SetLinkGraph(vtkMutableDirectedGraph* g);
  
  // Description:
  // Links the columns in a specific order.
  // This creates a simple path as the link graph.
  void LinkColumnPath(vtkStringArray* column, vtkStringArray* domain = 0, vtkBitArray* hidden = 0);

  // Description:
  // Specify the directedness of the output graph.
  vtkSetMacro(Directed, bool);
  vtkGetMacro(Directed, bool);
  vtkBooleanMacro(Directed, bool);
  
  // Description:
  // Get the current modified time.
  virtual unsigned long GetMTime();

  // Description:
  // A convenience method for setting the vertex table input.  This
  // is mainly for the benefit of the VTK client/server layer,
  // vanilla VTK code should use e.g:
  //
  // table_to_graph->SetInputConnection(1, vertex_table->output());
  //
  void SetVertexTableConnection(vtkAlgorithmOutput* in);
  
protected:
  vtkTableToGraph();
  ~vtkTableToGraph();

  // Description:
  // Validate that the link graph is in the appropriate format.
  int ValidateLinkGraph();
  
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  virtual int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);

  virtual int RequestDataObject(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);

  bool Directed;
  vtkMutableDirectedGraph* LinkGraph;
  vtkStringArray* VertexTableDomains;

private:
  vtkTableToGraph(const vtkTableToGraph&); // Not implemented
  void operator=(const vtkTableToGraph&);   // Not implemented
};

#endif

