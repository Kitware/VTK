/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTableToPostgreSQLWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTableToPostgreSQLWriter - store a vtkTable in a PostgreSQL database
// .SECTION Description
// vtkTableToPostgreSQLWriter reads a vtkTable and inserts it into a PostgreSQL
// database.

#ifndef vtkTableToPostgreSQLWriter_h
#define vtkTableToPostgreSQLWriter_h

#include "vtkIOPostgreSQLModule.h" // For export macro
#include "vtkTableToDatabaseWriter.h"

class vtkPostgreSQLDatabase;

class VTKIOPOSTGRESQL_EXPORT vtkTableToPostgreSQLWriter : public vtkTableToDatabaseWriter
{
public:
  static vtkTableToPostgreSQLWriter *New();
  vtkTypeMacro(vtkTableToPostgreSQLWriter,vtkTableToDatabaseWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the input to this writer.
  vtkTable* GetInput();
  vtkTable* GetInput(int port);

protected:
   vtkTableToPostgreSQLWriter();
  ~vtkTableToPostgreSQLWriter();
  void WriteData();

  virtual int FillInputPortInformation(int port, vtkInformation *info);

  vtkTable *Input;

private:
  vtkTableToPostgreSQLWriter(const vtkTableToPostgreSQLWriter&); // Not implemented.
  void operator=(const vtkTableToPostgreSQLWriter&); // Not implemented.
};

#endif
