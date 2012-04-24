/*=========================================================================

  Program:   Visualization Toolkit
  Module:    DatabaseSchemaWith2Tables.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTestingIOSQLModule.h"

class vtkSQLDatabaseSchema;

class VTKTESTINGIOSQL_EXPORT DatabaseSchemaWith2Tables
{
  public:
    DatabaseSchemaWith2Tables();
    ~DatabaseSchemaWith2Tables();
    vtkSQLDatabaseSchema* GetSchema() { return Schema; };
    int GetTableAHandle() { return TableAHandle; };
    int GetTableBHandle() { return TableBHandle; };
    vtkSQLDatabaseSchema* operator->() const { return this->Schema; };

  private:
    void Create();
    vtkSQLDatabaseSchema* Schema;
    int TableAHandle;
    int TableBHandle;
};

// VTK-HeaderTest-Exclude: DatabaseSchemaWith2Tables.h
