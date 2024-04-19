// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMySQLToTableReader
 * @brief   Read a MySQL table as a vtkTable
 *
 * vtkMySQLToTableReader reads a table from a MySQL database and
 * outputs it as a vtkTable.
 */

#ifndef vtkMySQLToTableReader_h
#define vtkMySQLToTableReader_h

#include "vtkDatabaseToTableReader.h"
#include "vtkIOMySQLModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkMySQLDatabase;

class VTKIOMYSQL_EXPORT vtkMySQLToTableReader : public vtkDatabaseToTableReader
{
public:
  static vtkMySQLToTableReader* New();
  vtkTypeMacro(vtkMySQLToTableReader, vtkDatabaseToTableReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkMySQLToTableReader();
  ~vtkMySQLToTableReader() override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkMySQLToTableReader(const vtkMySQLToTableReader&) = delete;
  void operator=(const vtkMySQLToTableReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
