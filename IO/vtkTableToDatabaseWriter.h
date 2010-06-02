/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTableToDatabaseWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTableToDatabaseWriter
// in a SQL database.
// .SECTION Description
// vtkTableToDatabaseWriter abstract parent class that reads a vtkTable and
// inserts it into an SQL database.

#ifndef __vtkTableToDatabaseWriter_h
#define __vtkTableToDatabaseWriter_h

#include <vtkstd/string> // STL Header
#include "vtkTableWriter.h"

class vtkSQLDatabase;
class vtkStringArray;

class VTK_IO_EXPORT vtkTableToDatabaseWriter : public vtkTableWriter
{
public:
  vtkTypeMacro(vtkTableToDatabaseWriter, vtkTableWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the database.  Must already be open.
  bool SetDatabase(vtkSQLDatabase *db);

  // Description:
  // Set the name of the new SQL table that you'd this writer to create.
  // Returns false if the specified table already exists in the database.
  bool SetTableName(const char *name);

  // Description:
  // Check if the currently specified table name exists in the database.
  bool TableNameIsNew();

  vtkSQLDatabase *GetDatabase() { return this->Database; }

  // Description:
  // Get the input to this writer.
  vtkTable* GetInput();
  vtkTable* GetInput(int port);

protected:
   vtkTableToDatabaseWriter();
  ~vtkTableToDatabaseWriter();
  virtual void WriteData() = 0;

  virtual int FillInputPortInformation(int port, vtkInformation *info);


  vtkSQLDatabase *Database;
  vtkTable *Input;
  //BTX
  vtkstd::string TableName;
  //ETX

private:
  vtkTableToDatabaseWriter(const vtkTableToDatabaseWriter&);  // Not implemented.
  void operator=(const vtkTableToDatabaseWriter&);  // Not implemented.
};

#endif
