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
/**
 * @class   vtkPostgreSQLToTableReader
 * @brief   Read a PostgreSQL table as a vtkTable
 *
 * vtkPostgreSQLToTableReader reads a table from a PostgreSQL database and
 * outputs it as a vtkTable.
*/

#ifndef vtkPostgreSQLToTableReader_h
#define vtkPostgreSQLToTableReader_h

#include "vtkIOPostgreSQLModule.h" // For export macro
#include "vtkDatabaseToTableReader.h"

class vtkPostgreSQLDatabase;

class VTKIOPOSTGRESQL_EXPORT vtkPostgreSQLToTableReader :
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
  vtkPostgreSQLToTableReader(const vtkPostgreSQLToTableReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPostgreSQLToTableReader&) VTK_DELETE_FUNCTION;
};

#endif
