/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTableToSQLiteWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

class vtkSQLiteDatabase;

class VTKIOSQL_EXPORT vtkTableToSQLiteWriter : public vtkTableToDatabaseWriter
{
public:
  static vtkTableToSQLiteWriter *New();
  vtkTypeMacro(vtkTableToSQLiteWriter,vtkTableToDatabaseWriter);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Get the input to this writer.
   */
  vtkTable* GetInput();
  vtkTable* GetInput(int port);
  //@}

protected:
   vtkTableToSQLiteWriter();
  ~vtkTableToSQLiteWriter() VTK_OVERRIDE;
  void WriteData() VTK_OVERRIDE;

  int FillInputPortInformation(int port, vtkInformation *info) VTK_OVERRIDE;

  vtkTable *Input;

private:
  vtkTableToSQLiteWriter(const vtkTableToSQLiteWriter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkTableToSQLiteWriter&) VTK_DELETE_FUNCTION;
};

#endif
