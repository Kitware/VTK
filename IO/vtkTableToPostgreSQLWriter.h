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

#ifndef __vtkTableToPostgreSQLWriter_h
#define __vtkTableToPostgreSQLWriter_h

#include <vtkstd/string>
#include <vtkstd/vector>
#include "vtkTableToDatabaseWriter.h"

class vtkPostgreSQLDatabase;

class VTK_IO_EXPORT vtkTableToPostgreSQLWriter : public vtkTableToDatabaseWriter
{
public:
  static vtkTableToPostgreSQLWriter *New();
  vtkTypeRevisionMacro(vtkTableToPostgreSQLWriter,vtkTableToDatabaseWriter);
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
  // Not implemented.
  vtkTableToPostgreSQLWriter(const vtkTableToPostgreSQLWriter&);
  // Not implemented.
  void operator=(const vtkTableToPostgreSQLWriter&);
};

#endif
