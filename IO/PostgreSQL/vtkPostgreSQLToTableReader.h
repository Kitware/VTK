// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPostgreSQLToTableReader
 * @brief   Read a PostgreSQL table as a vtkTable
 *
 * vtkPostgreSQLToTableReader reads a table from a PostgreSQL database and
 * outputs it as a vtkTable.
 */

#ifndef vtkPostgreSQLToTableReader_h
#define vtkPostgreSQLToTableReader_h

#include "vtkDatabaseToTableReader.h"
#include "vtkIOPostgreSQLModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkPostgreSQLDatabase;

class VTKIOPOSTGRESQL_EXPORT vtkPostgreSQLToTableReader : public vtkDatabaseToTableReader
{
public:
  static vtkPostgreSQLToTableReader* New();
  vtkTypeMacro(vtkPostgreSQLToTableReader, vtkDatabaseToTableReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkPostgreSQLToTableReader();
  ~vtkPostgreSQLToTableReader() override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkPostgreSQLToTableReader(const vtkPostgreSQLToTableReader&) = delete;
  void operator=(const vtkPostgreSQLToTableReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
