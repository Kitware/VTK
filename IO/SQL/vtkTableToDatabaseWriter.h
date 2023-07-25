// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTableToDatabaseWriter
 * in a SQL database.
 *
 * vtkTableToDatabaseWriter abstract parent class that reads a vtkTable and
 * inserts it into an SQL database.
 */

#ifndef vtkTableToDatabaseWriter_h
#define vtkTableToDatabaseWriter_h

#include "vtkIOSQLModule.h" // For export macro
#include "vtkWriter.h"
#include <string> // STL Header

VTK_ABI_NAMESPACE_BEGIN
class vtkSQLDatabase;
class vtkStringArray;
class vtkTable;

class VTKIOSQL_EXPORT vtkTableToDatabaseWriter : public vtkWriter
{
public:
  vtkTypeMacro(vtkTableToDatabaseWriter, vtkWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set the database.  Must already be open.
   */
  bool SetDatabase(vtkSQLDatabase* db);

  /**
   * Set the name of the new SQL table that you'd this writer to create.
   * Returns false if the specified table already exists in the database.
   */
  bool SetTableName(const char* name);

  /**
   * Check if the currently specified table name exists in the database.
   */
  bool TableNameIsNew();

  vtkSQLDatabase* GetDatabase() { return this->Database; }

  ///@{
  /**
   * Get the input to this writer.
   */
  vtkTable* GetInput();
  vtkTable* GetInput(int port);
  ///@}

protected:
  vtkTableToDatabaseWriter();
  ~vtkTableToDatabaseWriter() override;
  void WriteData() override = 0;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  vtkSQLDatabase* Database;
  vtkTable* Input;

  std::string TableName;

private:
  vtkTableToDatabaseWriter(const vtkTableToDatabaseWriter&) = delete;
  void operator=(const vtkTableToDatabaseWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
