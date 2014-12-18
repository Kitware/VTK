/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSQLDatabaseTableSource.h

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
// .NAME vtkSQLDatabaseTableSource - Generates a vtkTable based on an SQL query.
//
// .SECTION Description
// This class combines vtkSQLDatabase, vtkSQLQuery, and vtkQueryToTable to
// provide a convenience class for generating tables from databases.
// Also this class can be easily wrapped and used within ParaView / OverView.

#ifndef vtkSQLDatabaseTableSource_h
#define vtkSQLDatabaseTableSource_h

#include "vtkIOSQLModule.h" // For export macro
#include "vtkStdString.h"
#include "vtkTableAlgorithm.h"

class vtkEventForwarderCommand;

class VTKIOSQL_EXPORT vtkSQLDatabaseTableSource : public vtkTableAlgorithm
{
public:
  static vtkSQLDatabaseTableSource* New();
  vtkTypeMacro(vtkSQLDatabaseTableSource, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkStdString GetURL();
  void SetURL(const vtkStdString& url);

  void SetPassword(const vtkStdString& password);

  vtkStdString GetQuery();
  void SetQuery(const vtkStdString& query);

  // Description:
  // The name of the array for generating or assigning pedigree ids
  // (default "id").
  vtkSetStringMacro(PedigreeIdArrayName);
  vtkGetStringMacro(PedigreeIdArrayName);

  // Description:
  // If on (default), generates pedigree ids automatically.
  // If off, assign one of the arrays to be the pedigree id.
  vtkSetMacro(GeneratePedigreeIds, bool);
  vtkGetMacro(GeneratePedigreeIds, bool);
  vtkBooleanMacro(GeneratePedigreeIds, bool);

protected:
  vtkSQLDatabaseTableSource();
  ~vtkSQLDatabaseTableSource();

  int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);

private:
  vtkSQLDatabaseTableSource(const vtkSQLDatabaseTableSource&); // Not implemented
  void operator=(const vtkSQLDatabaseTableSource&);   // Not implemented

  char* PedigreeIdArrayName;
  bool GeneratePedigreeIds;

  // Description:
  // This intercepts events from the graph layout class
  // and re-emits them as if they came from this class.
  vtkEventForwarderCommand *EventForwarder;

//BTX
  class implementation;
  implementation* const Implementation;
//ETX
};

#endif

// VTK-HeaderTest-Exclude: vtkSQLDatabaseTableSource.h
