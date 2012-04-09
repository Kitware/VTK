/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSQLDatabaseGraphSource.h

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
// .NAME vtkSQLDatabaseGraphSource - Generates a vtkGraph based on an SQL query.
//
// .SECTION Description
// This class combines vtkSQLDatabase, vtkSQLQuery, and vtkQueryToGraph to
// provide a convenience class for generating graphs from databases. 
// Also this class can be easily wrapped and used within ParaView / OverView.

#ifndef __vtkSQLDatabaseGraphSource_h
#define __vtkSQLDatabaseGraphSource_h

#include "vtkStdString.h"
#include "vtkGraphAlgorithm.h"

class vtkEventForwarderCommand;

class VTK_INFOVIS_EXPORT vtkSQLDatabaseGraphSource : public vtkGraphAlgorithm
{
public:
  static vtkSQLDatabaseGraphSource* New();
  vtkTypeMacro(vtkSQLDatabaseGraphSource, vtkGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkStdString GetURL();
  void SetURL(const vtkStdString& url);

  void SetPassword(const vtkStdString& password);

  vtkStdString GetEdgeQuery();
  void SetEdgeQuery(const vtkStdString& query);

  vtkStdString GetVertexQuery();
  void SetVertexQuery(const vtkStdString& query);

  void AddLinkVertex(const char* column, const char* domain = 0, int hidden = 0);
  void ClearLinkVertices();
  void AddLinkEdge(const char* column1, const char* column2);
  void ClearLinkEdges();

  // Description:
  // If on (default), generate edge pedigree ids.
  // If off, assign an array to be edge pedigree ids.
  vtkGetMacro(GenerateEdgePedigreeIds, bool);
  vtkSetMacro(GenerateEdgePedigreeIds, bool);
  vtkBooleanMacro(GenerateEdgePedigreeIds, bool);

  // Description:
  // Use this array name for setting or generating edge pedigree ids.
  vtkSetStringMacro(EdgePedigreeIdArrayName);
  vtkGetStringMacro(EdgePedigreeIdArrayName);

  // Description:
  // If on (default), generate a directed output graph.
  // If off, generate an undirected output graph.
  vtkSetMacro(Directed, bool);
  vtkGetMacro(Directed, bool);
  vtkBooleanMacro(Directed, bool);
 
protected:
  vtkSQLDatabaseGraphSource();
  ~vtkSQLDatabaseGraphSource();
  
  int RequestDataObject(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);

  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);

  bool GenerateEdgePedigreeIds;
  char* EdgePedigreeIdArrayName;
  
private:
  vtkSQLDatabaseGraphSource(const vtkSQLDatabaseGraphSource&); // Not implemented
  void operator=(const vtkSQLDatabaseGraphSource&);   // Not implemented
 
  // Description:
  // This intercepts events from the graph layout class 
  // and re-emits them as if they came from this class.
  vtkEventForwarderCommand *EventForwarder;

//BTX
  class implementation;
  implementation* const Implementation;
//ETX

  bool Directed;
  

};

#endif

