/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSQLiteDatabase.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
// .NAME vtkSQLiteDatabase - maintain a connection to an SQLite database
//
// .SECTION Description
//
// SQLite (http://www.sqlite.org) is a public-domain SQL database
// written in C++.  It's small, fast, and can be easily embedded
// inside other applications.  Its databases are stored in files.
//
// This class provides a VTK interface to SQLite.  You do not need to
// download any external libraries: we include a copy of SQLite 3.3.16
// in VTK/Utilities/vtksqlite.  
//
// If you want to open a database that stays in memory and never gets
// written to disk, pass in the filename ':memory:'.
//
// .SECTION Thanks
// Thanks to Andrew Wilson from Sandia National Laboratories for implementing
// this class.
//
// .SECTION See Also
// vtkSQLiteQuery

#ifndef __vtkSQLiteDatabase_h
#define __vtkSQLiteDatabase_h

#include "vtkSQLDatabase.h"

class vtkSQLQuery;
class vtkSQLiteQuery;
class vtkStringArray;
struct vtk_sqlite3;

class VTK_IO_EXPORT vtkSQLiteDatabase : public vtkSQLDatabase
{
  //BTX
  friend class vtkSQLiteQuery;
  //ETX

public:
  vtkTypeRevisionMacro(vtkSQLiteDatabase, vtkSQLDatabase);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSQLiteDatabase *New();

  // Description:
  // Set/get the name of the file containing the database.  SQLite works
  // entirely on files and does not have a notion of network
  // connections.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  
  // Description:
  // Open a new connection to the database.  You need to set the
  // filename before calling this function.  Returns true if the
  // database was opened successfully; false otherwise.
  bool Open();

  // Description:
  // Close the connection to the database.
  void Close();
  
  // Description:
  // Return whether the database has an open connection
  bool IsOpen();

  // Description:
  // Return an empty query on this database.
  vtkSQLQuery* GetQueryInstance();
  
  // Description:
  // Get the last error text from the database
  const char* GetLastErrorText();
  
  // Description:
  // Get the list of tables from the database
  vtkStringArray* GetTables();
    
  // Description:
  // Get the list of fields for a particular table
  vtkStringArray* GetRecord(const char *table);

  // Description:
  // Return whether a feature is supported by the database.
  bool IsSupported(int feature);

protected:
  vtkSQLiteDatabase();
  ~vtkSQLiteDatabase();

private:
  vtk_sqlite3 *SQLiteInstance;
  char *FileName;
  char *LastErrorText;
  
  vtkSetStringMacro(LastErrorText);

  vtkSQLiteDatabase(const vtkSQLiteDatabase &); // Not implemented.
  void operator=(const vtkSQLiteDatabase &); // Not implemented.
};

#endif // __vtkSQLiteDatabase_h

