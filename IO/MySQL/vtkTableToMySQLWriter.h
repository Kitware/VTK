/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTableToMySQLWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTableToMySQLWriter - store a vtkTable in a MySQL database
// .SECTION Description
// vtkTableToMySQLWriter reads a vtkTable and inserts it into a MySQL
// database.

#ifndef __vtkTableToMySQLWriter_h
#define __vtkTableToMySQLWriter_h

#include "vtkIOMySQLModule.h" // For export macro
#include "vtkTableToDatabaseWriter.h"

class vtkMySQLDatabase;

class VTKIOMYSQL_EXPORT vtkTableToMySQLWriter : public vtkTableToDatabaseWriter
{
public:
  static vtkTableToMySQLWriter *New();
  vtkTypeMacro(vtkTableToMySQLWriter,vtkTableToDatabaseWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the input to this writer.
  vtkTable* GetInput();
  vtkTable* GetInput(int port);

protected:
   vtkTableToMySQLWriter();
  ~vtkTableToMySQLWriter();
  void WriteData();

  virtual int FillInputPortInformation(int port, vtkInformation *info);

  vtkTable *Input;

private:
  vtkTableToMySQLWriter(const vtkTableToMySQLWriter&);  // Not implemented.
  void operator=(const vtkTableToMySQLWriter&);  // Not implemented.
};

#endif
