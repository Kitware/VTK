/* -*- Mode: C++; -*- */

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkODBCDatabase.h

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

// .NAME vtkODBCDatabase - maintain an ODBC connection to a SQL database
//
// .SECTION Description
//
// ODBC is a standard for connecting to an SQL database regardless of
// vendor or implementation.  In order to make it work you need two
// separate pices of software: a driver manager and then a
// database-specific driver.  On Windows, the driver manager is built
// into the OS.  On Unix platforms, the open-source packages iODBC and
// unixODBC will do the job.  Mac OS X has its own driver manager and
// can also use the open-source packages.  Database-specific drivers
// must be obtained from the entity that makes the database(s) you
// use.
//
// Unlike the other VTK database drivers, ODBC can read its parameters
// from a configuration file (odbc.ini).  That file can define an
// entire set of connection parameters and give it a single name
// called a data source name (DSN).  Writing and maintaining odbc.ini
// files is beyond the scope of this header file.
//
// .SECTION Caveats
//
// The password supplied as an argument to the Open call will override
// whatever password is set (if any) in the DSN definition.  To use
// the password from the DSN definition, pass in NULL for
// the password argument.
//
// Also, vtkSQLDatabase::CreateFromURL() will only handle URLs of the
// following form for ODBC:
//
// odbc://[user@]datsourcename[:port]/[dbname]
//
// Anything more complicated than that needs to be set up manually.
//
// Finally, this class does not yet support the schema API present in
// the SQLite, MySQL and PostgreSQL drivers.  Those functions will be
// added once the bare-bones driver has been successfully integrated
// into VTK.
//
// .SECTION See Also
// vtkODBCQuery


#ifndef vtkODBCDatabase_h
#define vtkODBCDatabase_h

#include "vtkIOODBCModule.h" // For export macro
#include "vtkSQLDatabase.h"

class vtkSQLQuery;
class vtkODBCQuery;
class vtkStringArray;
class vtkODBCInternals;

class VTKIOODBC_EXPORT vtkODBCDatabase : public vtkSQLDatabase
{
//BTX
  friend class vtkODBCQuery;
//ETX

public:
  vtkTypeMacro(vtkODBCDatabase, vtkSQLDatabase);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkODBCDatabase *New();

  // Description:
  // Open a new connection to the database.  You need to set the
  // filename before calling this function.  Returns true if the
  // database was opened successfully; false otherwise.
  bool Open(const char* password);

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

  // Description:
  // Set the data source name.  For ODBC connections this will be
  // something listed in odbc.ini.  The location of that file varies wildly
  // based on system, ODBC library, and installation.  Good luck.
  vtkSetStringMacro(DataSourceName);
  vtkGetStringMacro(DataSourceName);

  vtkSetMacro(ServerPort, int);
  vtkSetStringMacro(HostName);
  vtkSetStringMacro(UserName);
  vtkSetStringMacro(DatabaseName);
  vtkGetStringMacro(DatabaseName);
  vtkSetStringMacro(Password);

  bool HasError();

  // Description:
  // String representing database type (e.g. "ODBC").
  vtkGetStringMacro(DatabaseType);

  vtkStdString GetURL();

  // Description:
  // Return the SQL string with the syntax to create a column inside a
  // "CREATE TABLE" SQL statement.
  // NB2: if a column has type SERIAL in the schema, this will be turned
  // into INT NOT NULL. Therefore, one should not pass
  // NOT NULL as an attribute of a column whose type is SERIAL.
  virtual vtkStdString GetColumnSpecification( vtkSQLDatabaseSchema* schema,
                                               int tblHandle,
                                               int colHandle );

  // Description:
  // Return the SQL string with the syntax to create an index inside a
  // "CREATE TABLE" SQL statement.
  virtual vtkStdString GetIndexSpecification( vtkSQLDatabaseSchema* schema,
                                              int tblHandle,
                                              int idxHandle,
                                              bool& skipped );

  // Description:
  // Create a new database, optionally dropping any existing database of the same name.
  // Returns true when the database is properly created and false on failure.
  bool CreateDatabase( const char* dbName, bool dropExisting );

  // Description:
  // Drop a database if it exists.
  // Returns true on success and false on failure.
  bool DropDatabase( const char* dbName );

  // Description:
  // This will only handle URLs of the form
  // odbc://[user@]datsourcename[:port]/[dbname].  Anything
  // more complicated than that needs to be set up manually.
  bool ParseURL(const char *url);

protected:
  vtkODBCDatabase();
  ~vtkODBCDatabase();

  vtkSetStringMacro(LastErrorText);

private:
  vtkStringArray *Tables;
  vtkStringArray *Record;

  char *LastErrorText;

  char *HostName;
  char *UserName;
  char *Password;
  char *DataSourceName;
  char *DatabaseName;
  int ServerPort;

  vtkODBCInternals *Internals;

  // We want this to be private, a user of this class
  // should not be setting this for any reason
  vtkSetStringMacro(DatabaseType);

  char *DatabaseType;

  vtkODBCDatabase(const vtkODBCDatabase &); // Not implemented.
  void operator=(const vtkODBCDatabase &); // Not implemented.
};

#endif // vtkODBCDatabase_h

