// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDatabaseToTableReader
 * @brief   Read an SQL table as a vtkTable
 *
 * vtkDatabaseToTableReader reads a table from an SQL database, outputting
 * it as a vtkTable.
 */

#ifndef vtkDatabaseToTableReader_h
#define vtkDatabaseToTableReader_h

#include "vtkIOSQLModule.h" // For export macro
#include "vtkTableAlgorithm.h"
#include <string> // STL Header

VTK_ABI_NAMESPACE_BEGIN
class vtkSQLDatabase;
class vtkStringArray;

class VTKIOSQL_EXPORT vtkDatabaseToTableReader : public vtkTableAlgorithm
{
public:
  vtkTypeMacro(vtkDatabaseToTableReader, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set the database associated with this reader
   */
  bool SetDatabase(vtkSQLDatabase* db);

  /**
   * Set the name of the table that you'd like to convert to a vtkTable
   * Returns false if the specified table does not exist in the database.
   */
  bool SetTableName(const char* name);

  /**
   * Check if the currently specified table name exists in the database.
   */
  bool CheckIfTableExists();

  vtkSQLDatabase* GetDatabase() { return this->Database; }

protected:
  vtkDatabaseToTableReader();
  ~vtkDatabaseToTableReader() override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override = 0;
  vtkSQLDatabase* Database;

  std::string TableName;

private:
  vtkDatabaseToTableReader(const vtkDatabaseToTableReader&) = delete;
  void operator=(const vtkDatabaseToTableReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
