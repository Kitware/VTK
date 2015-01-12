/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPBGLGraphSQLReader.h

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
// .NAME vtkPBGLGraphSQLReader - read a vtkGraph from a database
//
// .SECTION Description
//
// Creates a vtkGraph using two SQL tables.  The edge table
// must have one row for each edge in the graph.
// The table must have two columns which represent the source and target
// vertex ids.
//
// The vertex table has one row for each vertex in the graph.
// The table must have a field whose values match those in the edge table.
//
// The source, target, and node ID fields must be of the same type.
//
// NOTE: This filter currently only produces the pedigree id field in
// the vertex attributes, and no edge attributes.
//
// @deprecated Not maintained as of VTK 6.2 and will be removed eventually.

#ifndef vtkPBGLGraphSQLReader_h
#define vtkPBGLGraphSQLReader_h

#include "vtkInfovisParallelModule.h" // For export macro
#include "vtkGraphAlgorithm.h"

class vtkSQLDatabase;

#if !defined(VTK_LEGACY_REMOVE)
class VTKINFOVISPARALLEL_EXPORT vtkPBGLGraphSQLReader : public vtkGraphAlgorithm
{
public:
  static vtkPBGLGraphSQLReader* New();
  vtkTypeMacro(vtkPBGLGraphSQLReader,vtkGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // When set, creates a directed graph, as opposed to an undirected graph.
  vtkSetMacro(Directed, bool);
  vtkGetMacro(Directed, bool);
  vtkBooleanMacro(Directed, bool);

  // Description:
  // The database to connect to.
  virtual void SetDatabase(vtkSQLDatabase* db);
  vtkGetObjectMacro(Database, vtkSQLDatabase);

  // Description:
  // The name of the vertex table in the database.
  vtkSetStringMacro(VertexTable);
  vtkGetStringMacro(VertexTable);

  // Description:
  // The name of the edge table in the database.
  vtkSetStringMacro(EdgeTable);
  vtkGetStringMacro(EdgeTable);

  // Description:
  // The name of the field in the edge query for the source node of each edge.
  vtkSetStringMacro(SourceField);
  vtkGetStringMacro(SourceField);

  // Description:
  // The name of the field in the edge query for the target node of each edge.
  vtkSetStringMacro(TargetField);
  vtkGetStringMacro(TargetField);

  // Description:
  // The name of the field in the node query for the node ID.
  vtkSetStringMacro(VertexIdField);
  vtkGetStringMacro(VertexIdField);

  // Description:
  // Get the offset/limit for this process's vertices/edges
  static void GetRange(int rank, int total,
    vtkIdType size, vtkIdType& offset, vtkIdType& limit);

  // Description:
  // Set the distribution user data.
  void SetDistributionUserData(int procs, vtkIdType verts)
    { this->DistributionUserData[0] = procs;
      this->DistributionUserData[1] = verts; }

  // Description:
  // Get the user data (# procs, # vertices) used to determine
  // the distribution.
  vtkIdType* GetDistributionUserData()
    { return this->DistributionUserData; }

protected:
  vtkPBGLGraphSQLReader();
  ~vtkPBGLGraphSQLReader();

  bool Directed;
  vtkSQLDatabase* Database;
  char* VertexTable;
  char* EdgeTable;
  char* SourceField;
  char* TargetField;
  char* VertexIdField;
  vtkIdType DistributionUserData[2];

  virtual int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);

  virtual int RequestDataObject(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);

private:
  vtkPBGLGraphSQLReader(const vtkPBGLGraphSQLReader&); // Not implemented
  void operator=(const vtkPBGLGraphSQLReader&);   // Not implemented
};

#endif //VTK_LEGACY_REMOVE
#endif

