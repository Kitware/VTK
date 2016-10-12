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
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/**
 * @class   vtkSQLiteDatabase
 * @brief   maintain a connection to an SQLite database
 *
 *
 *
 * SQLite (http://www.sqlite.org) is a public-domain SQL database
 * written in C++.  It's small, fast, and can be easily embedded
 * inside other applications.  Its databases are stored in files.
 *
 * This class provides a VTK interface to SQLite.  You do not need to
 * download any external libraries: we include a copy of SQLite 3.3.16
 * in VTK/Utilities/vtksqlite.
 *
 * If you want to open a database that stays in memory and never gets
 * written to disk, pass in the URL 'sqlite://:memory:'; otherwise,
 * specifiy the file path by passing the URL 'sqlite://<file_path>'.
 *
 * @par Thanks:
 * Thanks to Andrew Wilson and Philippe Pebay from Sandia National
 * Laboratories for implementing this class.
 *
 * @sa
 * vtkSQLiteQuery
*/

#ifndef vtkSQLiteDatabase_h
#define vtkSQLiteDatabase_h

#include "vtkIOSQLModule.h" // For export macro
#include "vtkSQLDatabase.h"

class vtkSQLQuery;
class vtkSQLiteQuery;
class vtkStringArray;
struct vtk_sqlite3;

class VTKIOSQL_EXPORT vtkSQLiteDatabase : public vtkSQLDatabase
{

  friend class vtkSQLiteQuery;

public:
  vtkTypeMacro(vtkSQLiteDatabase, vtkSQLDatabase);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSQLiteDatabase *New();

  enum {
    USE_EXISTING,
    USE_EXISTING_OR_CREATE,
    CREATE_OR_CLEAR,
    CREATE
  };

  //@{
  /**
   * Open a new connection to the database.  You need to set the
   * filename before calling this function.  Returns true if the
   * database was opened successfully; false otherwise.
   * - USE_EXISTING (default) - Fail if the file does not exist.
   * - USE_EXISTING_OR_CREATE - Create a new file if necessary.
   * - CREATE_OR_CLEAR - Create new or clear existing file.
   * - CREATE - Create new, fail if file exists.
   */
  bool Open(const char* password);
  bool Open(const char* password, int mode);
  //@}

  /**
   * Close the connection to the database.
   */
  void Close();

  /**
   * Return whether the database has an open connection
   */
  bool IsOpen();

  /**
   * Return an empty query on this database.
   */
  vtkSQLQuery* GetQueryInstance();

  /**
   * Get the list of tables from the database
   */
  vtkStringArray* GetTables();

  /**
   * Get the list of fields for a particular table
   */
  vtkStringArray* GetRecord(const char *table);

  /**
   * Return whether a feature is supported by the database.
   */
  bool IsSupported(int feature);

  /**
   * Did the last operation generate an error
   */
  bool HasError();

  /**
   * Get the last error text from the database
   */
  const char* GetLastErrorText();

  //@{
  /**
   * String representing database type (e.g. "sqlite").
   */
  vtkGetStringMacro(DatabaseType);
  //@}

  //@{
  /**
   * String representing the database filename.
   */
  vtkGetStringMacro(DatabaseFileName);
  vtkSetStringMacro(DatabaseFileName);
  //@}

  /**
   * Get the URL of the database.
   */
  virtual vtkStdString GetURL();

  /**
   * Return the SQL string with the syntax to create a column inside a
   * "CREATE TABLE" SQL statement.
   * NB: this method implements the SQLite-specific syntax:
   * <column name> <column type> <column attributes>
   */
  virtual vtkStdString GetColumnSpecification( vtkSQLDatabaseSchema* schema,
                                               int tblHandle,
                                               int colHandle );

protected:
  vtkSQLiteDatabase();
  ~vtkSQLiteDatabase();

  /**
   * Overridden to determine connection parameters given the URL.
   * This is called by CreateFromURL() to initialize the instance.
   * Look at CreateFromURL() for details about the URL format.
   */
  virtual bool ParseURL(const char* url);

private:
  vtk_sqlite3 *SQLiteInstance;

  // We want this to be private, a user of this class
  // should not be setting this for any reason
  vtkSetStringMacro(DatabaseType);

  vtkStringArray *Tables;

  char* DatabaseType;
  char* DatabaseFileName;

  vtkStdString TempURL;

  vtkSQLiteDatabase(const vtkSQLiteDatabase &) VTK_DELETE_FUNCTION;
  void operator=(const vtkSQLiteDatabase &) VTK_DELETE_FUNCTION;
};

#endif // vtkSQLiteDatabase_h

