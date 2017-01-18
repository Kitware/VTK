/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDatabaseToTableReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkDatabaseToTableReader
 * @brief   Read an SQL table as a vtkTable
 *
 * vtkDatabaseToTableReader reads a table from an SQL database, outputting
 * it as a vtkTable.
*/

#ifndef vtkDatabaseToTableReader_h
#define vtkDatabaseToTableReader_h

#include "vtkIOSQLModule.h" // For export macro
#include <string> // STL Header
#include "vtkTableAlgorithm.h"

class vtkSQLDatabase;
class vtkStringArray;

class VTKIOSQL_EXPORT vtkDatabaseToTableReader : public vtkTableAlgorithm
{
public:
  vtkTypeMacro(vtkDatabaseToTableReader,vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Set the database associated with this reader
   */
  bool SetDatabase(vtkSQLDatabase *db);

  /**
   * Set the name of the table that you'd like to convert to a vtkTable
   * Returns false if the specified table does not exist in the database.
   */
  bool SetTableName(const char *name);

  /**
   * Check if the currently specified table name exists in the database.
   */
  bool CheckIfTableExists();

  vtkSQLDatabase *GetDatabase() { return this->Database; }

protected:
   vtkDatabaseToTableReader();
  ~vtkDatabaseToTableReader() VTK_OVERRIDE;
  int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *) VTK_OVERRIDE = 0;
  vtkSQLDatabase *Database;

  std::string TableName;

private:
  vtkDatabaseToTableReader(const vtkDatabaseToTableReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkDatabaseToTableReader&) VTK_DELETE_FUNCTION;
};

#endif
