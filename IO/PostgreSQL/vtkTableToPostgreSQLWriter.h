// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTableToPostgreSQLWriter
 * @brief   store a vtkTable in a PostgreSQL database
 *
 * vtkTableToPostgreSQLWriter reads a vtkTable and inserts it into a PostgreSQL
 * database.
 */

#ifndef vtkTableToPostgreSQLWriter_h
#define vtkTableToPostgreSQLWriter_h

#include "vtkIOPostgreSQLModule.h" // For export macro
#include "vtkTableToDatabaseWriter.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkPostgreSQLDatabase;

class VTKIOPOSTGRESQL_EXPORT vtkTableToPostgreSQLWriter : public vtkTableToDatabaseWriter
{
public:
  static vtkTableToPostgreSQLWriter* New();
  vtkTypeMacro(vtkTableToPostgreSQLWriter, vtkTableToDatabaseWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the input to this writer.
   */
  vtkTable* GetInput();
  vtkTable* GetInput(int port);
  ///@}

protected:
  vtkTableToPostgreSQLWriter();
  ~vtkTableToPostgreSQLWriter() override;
  void WriteData() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkTableToPostgreSQLWriter(const vtkTableToPostgreSQLWriter&) = delete;
  void operator=(const vtkTableToPostgreSQLWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
