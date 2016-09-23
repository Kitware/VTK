/* -*- Mode: C++; -*- */
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPostgreSQLDatabase.h

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
 * @class   vtkPostgreSQLDatabase
 * @brief   maintain a connection to a PostgreSQL database
 *
 *
 *
 * PostgreSQL (http://www.postgres.org) is a BSD-licensed SQL database.
 * It's large, fast, and can not be easily embedded
 * inside other applications.  Its databases are stored in files that
 * belong to another process.
 *
 * This class provides a VTK interface to PostgreSQL.  You do need to
 * download external libraries: we need a copy of PostgreSQL 8
 * (currently 8.2 or 8.3) so that we can link against the libpq C
 * interface.
 *
 *
 * @par Thanks:
 * Thanks to David Thompson and Andy Wilson from Sandia National
 * Laboratories for implementing this class.
 *
 * @sa
 * vtkPostgreSQLQuery
*/

#ifndef vtkPostgreSQLDatabase_h
#define vtkPostgreSQLDatabase_h

#include "vtkIOPostgreSQLModule.h" // For export macro
#include "vtkSQLDatabase.h"

class vtkPostgreSQLQuery;
class vtkStringArray;
class vtkPostgreSQLDatabasePrivate;
struct PQconn;

class VTKIOPOSTGRESQL_EXPORT vtkPostgreSQLDatabase : public vtkSQLDatabase
{

  friend class vtkPostgreSQLQuery;
  friend class vtkPostgreSQLQueryPrivate;

public:
  vtkTypeMacro(vtkPostgreSQLDatabase, vtkSQLDatabase);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkPostgreSQLDatabase *New();

  /**
   * Open a new connection to the database.  You need to set the
   * filename before calling this function.  Returns true if the
   * database was opened successfully; false otherwise.
   */
  bool Open( const char* password = 0 );

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
   * Did the last operation generate an error
   */
  virtual bool HasError();

  /**
   * Get the last error text from the database
   */
  const char* GetLastErrorText();

  //@{
  /**
   * String representing database type (e.g. "psql").
   */
  vtkGetStringMacro(DatabaseType);
  //@}

  //@{
  /**
   * The database server host name.
   */
  virtual void SetHostName( const char* );
  vtkGetStringMacro(HostName);
  //@}

  //@{
  /**
   * The user name for connecting to the database server.
   */
  virtual void SetUser( const char* );
  vtkGetStringMacro(User);
  //@}

  /**
   * The user's password for connecting to the database server.
   */
  virtual void SetPassword( const char* );

  //@{
  /**
   * The name of the database to connect to.
   */
  virtual void SetDatabaseName( const char* );
  vtkGetStringMacro(DatabaseName);
  //@}

  //@{
  /**
   * Additional options for the database.
   */
  virtual void SetConnectOptions( const char* );
  vtkGetStringMacro(ConnectOptions);
  //@}

  //@{
  /**
   * The port used for connecting to the database.
   */
  virtual void SetServerPort( int );
  virtual int GetServerPortMinValue()
  {
    return 0;
  }
  virtual int GetServerPortMaxValue()
  {
    return VTK_INT_MAX;
  }
  vtkGetMacro(ServerPort, int);
  //@}

  /**
   * Get a URL referencing the current database connection.
   * This is not well-defined if the HostName and DatabaseName
   * have not been set. The URL will be of the form
   * <code>'psql://'[username[':'password]'@']hostname[':'port]'/'database</code> .
   */
  virtual vtkStdString GetURL();

  /**
   * Get the list of tables from the database
   */
  vtkStringArray* GetTables();

  /**
   * Get the list of fields for a particular table
   */
  vtkStringArray* GetRecord( const char* table );

  /**
   * Return whether a feature is supported by the database.
   */
  bool IsSupported( int feature );

  /**
   * Return a list of databases on the server.
   */
  vtkStringArray* GetDatabases();

  /**
   * Create a new database, optionally dropping any existing database of the same name.
   * Returns true when the database is properly created and false on failure.
   */
  bool CreateDatabase( const char* dbName, bool dropExisting = false );

  /**
   * Drop a database if it exists.
   * Returns true on success and false on failure.
   */
  bool DropDatabase( const char* dbName );

  /**
   * Return the SQL string with the syntax to create a column inside a
   * "CREATE TABLE" SQL statement.
   * NB: this method implements the PostgreSQL-specific syntax:
   * <column name> <column type> <column attributes>
   */
  virtual vtkStdString GetColumnSpecification(
    vtkSQLDatabaseSchema* schema, int tblHandle, int colHandle );

  /**
   * Overridden to determine connection parameters given the URL.
   * This is called by CreateFromURL() to initialize the instance.
   * Look at CreateFromURL() for details about the URL format.
   */
  virtual bool ParseURL(const char* url);

protected:
  vtkPostgreSQLDatabase();
  ~vtkPostgreSQLDatabase();

  /**
   * Create or refresh the map from Postgres column types to VTK array types.

   * Postgres defines a table for types so that users may define types.
   * This adaptor does not support user-defined types or even all of the
   * default types defined by Postgres (some are inherently difficult to
   * translate into VTK since Postgres allows columns to have composite types,
   * vector-valued types, and extended precision types that vtkVariant does
   * not support.

   * This routine examines the pg_types table to get a map from Postgres column
   * type IDs (stored as OIDs) to VTK array types. It is called whenever a new
   * database connection is initiated.
   */
  void UpdateDataTypeMap();

  vtkSetStringMacro(DatabaseType);
  vtkSetStringMacro(LastErrorText);
  void NullTrailingWhitespace( char* msg );
  bool OpenInternal( const char* connectionOptions );

  vtkTimeStamp URLMTime;
  vtkPostgreSQLDatabasePrivate *Connection;
  vtkTimeStamp ConnectionMTime;
  vtkStringArray *Tables;
  char* DatabaseType;
  char* HostName;
  char* User;
  char* Password;
  char* DatabaseName;
  int ServerPort;
  char* ConnectOptions;
  char* LastErrorText;

private:
  vtkPostgreSQLDatabase( const vtkPostgreSQLDatabase& ) VTK_DELETE_FUNCTION;
  void operator = ( const vtkPostgreSQLDatabase& ) VTK_DELETE_FUNCTION;
};

// This is basically the body of the SetStringMacro but with a
// call to update an additional vtkTimeStamp. We inline the implementation
// so that wrapping will work.
#define vtkSetStringPlusMTimeMacro(className,name,timeStamp) \
  inline void className::Set##name (const char* _arg) \
  { \
    vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting " << #name " to " << (_arg?_arg:"(null)") ); \
    if ( this->name == NULL && _arg == NULL) { return;} \
    if ( this->name && _arg && (!strcmp(this->name,_arg))) { return;} \
    delete [] this->name; \
    if (_arg) \
    { \
          size_t n = strlen(_arg) + 1; \
          char *cp1 =  new char[n]; \
          const char *cp2 = (_arg); \
          this->name = cp1; \
          do { *cp1++ = *cp2++; } while ( --n ); \
    } \
     else \
     { \
          this->name = NULL; \
     } \
    this->Modified(); \
    this->timeStamp.Modified(); \
    this->Close(); /* Force a re-open on next query */ \
  }

vtkSetStringPlusMTimeMacro(vtkPostgreSQLDatabase,HostName,URLMTime);
vtkSetStringPlusMTimeMacro(vtkPostgreSQLDatabase,User,URLMTime);
vtkSetStringPlusMTimeMacro(vtkPostgreSQLDatabase,Password,URLMTime);
vtkSetStringPlusMTimeMacro(vtkPostgreSQLDatabase,DatabaseName,URLMTime);
vtkSetStringPlusMTimeMacro(vtkPostgreSQLDatabase,ConnectOptions,URLMTime);

inline void vtkPostgreSQLDatabase::SetServerPort( int _arg )
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting ServerPort to " << _arg );
  if ( this->ServerPort != ( _arg < 0 ? 0 : ( _arg > VTK_INT_MAX ? VTK_INT_MAX : _arg ) ) )
  {
    this->ServerPort = ( _arg < 0 ? 0 : ( _arg > VTK_INT_MAX ? VTK_INT_MAX : _arg ) );
    this->Modified();
    this->URLMTime.Modified();
    this->Close(); // Force a re-open on next query
  }
}

#endif // vtkPostgreSQLDatabase_h
