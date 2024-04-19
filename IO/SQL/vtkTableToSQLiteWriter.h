// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTableToSQLiteWriter
 * @brief   store a vtkTable in an SQLite database
 *
 * vtkTableToSQLiteWriter reads a vtkTable and inserts it into an SQLite
 * database.
 */

#ifndef vtkTableToSQLiteWriter_h
#define vtkTableToSQLiteWriter_h

#include "vtkIOSQLModule.h" // For export macro
#include "vtkTableToDatabaseWriter.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkSQLiteDatabase;

class VTKIOSQL_EXPORT vtkTableToSQLiteWriter : public vtkTableToDatabaseWriter
{
public:
  static vtkTableToSQLiteWriter* New();
  vtkTypeMacro(vtkTableToSQLiteWriter, vtkTableToDatabaseWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the input to this writer.
   */
  vtkTable* GetInput();
  vtkTable* GetInput(int port);
  ///@}

protected:
  vtkTableToSQLiteWriter();
  ~vtkTableToSQLiteWriter() override;
  void WriteData() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkTableToSQLiteWriter(const vtkTableToSQLiteWriter&) = delete;
  void operator=(const vtkTableToSQLiteWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
