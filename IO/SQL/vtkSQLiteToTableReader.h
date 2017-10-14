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
/**
 * @class   vtkSQLiteToTableReader
 * @brief   Read an SQLite table as a vtkTable
 *
 * vtkSQLiteToTableReader reads a table from an SQLite database and
 * outputs it as a vtkTable.
*/

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
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
   vtkSQLiteToTableReader();
  ~vtkSQLiteToTableReader() override;
  int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *) override;
private:
  vtkSQLiteToTableReader(const vtkSQLiteToTableReader&) = delete;
  void operator=(const vtkSQLiteToTableReader&) = delete;
};

#endif
