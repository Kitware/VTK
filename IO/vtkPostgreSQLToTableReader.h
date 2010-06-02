/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPostgreSQLToTableReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPostgreSQLToTableReader - Read a PostgreSQL table as a vtkTable
// .SECTION Description
// vtkPostgreSQLToTableReader reads a table from a PostgreSQL database and
// outputs it as a vtkTable.

#ifndef __vtkPostgreSQLToTableReader_h
#define __vtkPostgreSQLToTableReader_h

#include "vtkDatabaseToTableReader.h"

class vtkPostgreSQLDatabase;

class VTK_IO_EXPORT vtkPostgreSQLToTableReader :
  public vtkDatabaseToTableReader
{
public:
  static vtkPostgreSQLToTableReader *New();
  vtkTypeMacro(vtkPostgreSQLToTableReader,vtkDatabaseToTableReader);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
   vtkPostgreSQLToTableReader();
  ~vtkPostgreSQLToTableReader();
  int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);
private:
  vtkPostgreSQLToTableReader(const vtkPostgreSQLToTableReader&); // Not implemented.
  void operator=(const vtkPostgreSQLToTableReader&); // Not implemented.
};

#endif
