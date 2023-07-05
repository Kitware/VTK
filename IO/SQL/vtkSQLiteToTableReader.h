// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSQLiteToTableReader
 * @brief   Read an SQLite table as a vtkTable
 *
 * vtkSQLiteToTableReader reads a table from an SQLite database and
 * outputs it as a vtkTable.
 */

#ifndef vtkSQLiteToTableReader_h
#define vtkSQLiteToTableReader_h

#include "vtkDatabaseToTableReader.h"
#include "vtkIOSQLModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkSQLiteDatabase;

class VTKIOSQL_EXPORT vtkSQLiteToTableReader : public vtkDatabaseToTableReader
{
public:
  static vtkSQLiteToTableReader* New();
  vtkTypeMacro(vtkSQLiteToTableReader, vtkDatabaseToTableReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSQLiteToTableReader();
  ~vtkSQLiteToTableReader() override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkSQLiteToTableReader(const vtkSQLiteToTableReader&) = delete;
  void operator=(const vtkSQLiteToTableReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
