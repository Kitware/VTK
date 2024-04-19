// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkTestingIOSQLModule.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkSQLDatabaseSchema;

class VTKTESTINGIOSQL_EXPORT DatabaseSchemaWith2Tables
{
public:
  DatabaseSchemaWith2Tables();
  ~DatabaseSchemaWith2Tables();
  vtkSQLDatabaseSchema* GetSchema() { return Schema; }
  int GetTableAHandle() { return TableAHandle; }
  int GetTableBHandle() { return TableBHandle; }
  vtkSQLDatabaseSchema* operator->() const { return this->Schema; }

private:
  void Create();
  vtkSQLDatabaseSchema* Schema;
  int TableAHandle;
  int TableBHandle;
};

// VTK-HeaderTest-Exclude: DatabaseSchemaWith2Tables.h
VTK_ABI_NAMESPACE_END
