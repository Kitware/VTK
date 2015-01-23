/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSQLiteToTableReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSQLiteToTableReader - Read an SQLite table as a vtkTable
// .SECTION Description
// vtkSQLiteToTableReader reads a table from an SQLite database and
// outputs it as a vtkTable.

#ifndef vtkSQLiteToTableReader_h
#define vtkSQLiteToTableReader_h

#include "vtkIOSQLModule.h" // For export macro
#include "vtkDatabaseToTableReader.h"

class vtkSQLiteDatabase;

class VTKIOSQL_EXPORT vtkSQLiteToTableReader :
  public vtkDatabaseToTableReader
{
public:
  static vtkSQLiteToTableReader *New();
  vtkTypeMacro(vtkSQLiteToTableReader,vtkDatabaseToTableReader);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
   vtkSQLiteToTableReader();
  ~vtkSQLiteToTableReader();
  int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);
private:
  vtkSQLiteToTableReader(const vtkSQLiteToTableReader&); // Not implemented.
  void operator=(const vtkSQLiteToTableReader&); // Not implemented.
};

#endif
