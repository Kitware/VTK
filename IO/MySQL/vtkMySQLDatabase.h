/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMySQLDatabase.h

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
 * @class   vtkMySQLDatabase
 * @brief   maintain a connection to a MySQL database
 *
 *
 *
 * This class provides a VTK interface to MySQL
 * (http://www.mysql.com).  Unlike file-based databases like SQLite, you
 * talk to MySQL through a client/server connection.  You must specify
 * the hostname, (optional) port to connect to, username, password and
 * database name in order to connect.
 *
 * @sa
 * vtkMySQLQuery
*/

#ifndef vtkMySQLDatabase_h
#define vtkMySQLDatabase_h

#include "vtkIOMySQLModule.h" // For export macro
#include "vtkSQLDatabase.h"

class vtkSQLQuery;
class vtkMySQLQuery;
class vtkStringArray;
class vtkMySQLDatabasePrivate;

class VTKIOMYSQL_EXPORT vtkMySQLDatabase : public vtkSQLDatabase
{

  friend class vtkMySQLQuery;

public:
  vtkTypeMacro(vtkMySQLDatabase, vtkSQLDatabase);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkMySQLDatabase *New();

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
   * String representing database type (e.g. "mysql").
   */
  const char* GetDatabaseType() VTK_OVERRIDE
  {
    return this->DatabaseType;
  }
  //@}

  //@{
  /**
   * The database server host name.
   */
  vtkSetStringMacro(HostName);
  vtkGetStringMacro(HostName);
  //@}

  //@{
  /**
   * The user name for connecting to the database server.
   */
  vtkSetStringMacro(User);
  vtkGetStringMacro(User);
  //@}

  //@{
  /**
   * The user's password for connecting to the database server.
   */
  vtkSetStringMacro(Password);
  //@}

  //@{
  /**
   * The name of the database to connect to.
   */
  vtkSetStringMacro(DatabaseName);
  vtkGetStringMacro(DatabaseName);
  //@}

  //@{
  /**
   * Should automatic reconnection be enabled?
   * This defaults to true.
   * If you change its value, you must do so before any call to Open().
   */
  vtkSetMacro(Reconnect,int);
  vtkGetMacro(Reconnect,int);
  vtkBooleanMacro(Reconnect,int);
  //@}

  //@{
  /**
   * The port used for connecting to the database.
   */
  vtkSetClampMacro(ServerPort, int, 0, VTK_INT_MAX);
  vtkGetMacro(ServerPort, int);
  //@}

  /**
   * Get the URL of the database.
   */
  virtual vtkStdString GetURL();

  /**
   * Return the SQL string with the syntax of the preamble following a
   * "CREATE TABLE" SQL statement.
   * NB: this method implements the MySQL-specific IF NOT EXISTS syntax,
   * used when b = false.
   */
  virtual vtkStdString GetTablePreamble( bool b ) { return b ? vtkStdString() :"IF NOT EXISTS "; }

  /**
   * Return the SQL string with the syntax to create a column inside a
   * "CREATE TABLE" SQL statement.
   * NB1: this method implements the MySQL-specific syntax:
   * \verbatim
   * `<column name>` <column type> <column attributes>
   * \endverbatim
   * NB2: if a column has type SERIAL in the schema, this will be turned
   * into INT NOT NULL AUTO_INCREMENT. Therefore, one should not pass
   * NOT NULL as an attribute of a column whose type is SERIAL.
   */
  virtual vtkStdString GetColumnSpecification( vtkSQLDatabaseSchema* schema,
                                               int tblHandle,
                                               int colHandle );

  /**
   * Return the SQL string with the syntax to create an index inside a
   * "CREATE TABLE" SQL statement.
   * NB1: this method implements the MySQL-specific syntax:
   * \verbatim
   * <index type> [<index name>]  (`<column name 1>`,... )
   * \endverbatim
   * NB2: since MySQL supports INDEX creation within a CREATE TABLE statement,
   * skipped is always returned false.
   */
  virtual vtkStdString GetIndexSpecification( vtkSQLDatabaseSchema* schema,
                                              int tblHandle,
                                              int idxHandle,
                                              bool& skipped );

  /**
   * Create a new database, optionally dropping any existing database of the same name.
   * Returns true when the database is properly created and false on failure.
   */
  bool CreateDatabase( const char* dbName, bool dropExisting );

  /**
   * Drop a database if it exists.
   * Returns true on success and false on failure.
   */
  bool DropDatabase( const char* dbName );

  /**
   * Overridden to determine connection parameters given the URL.
   * This is called by CreateFromURL() to initialize the instance.
   * Look at CreateFromURL() for details about the URL format.
   */
  virtual bool ParseURL(const char* url);

protected:
  vtkMySQLDatabase();
  ~vtkMySQLDatabase();

private:
  // We want this to be private, a user of this class
  // should not be setting this for any reason
  vtkSetStringMacro(DatabaseType);

  vtkStringArray *Tables;
  vtkStringArray *Record;

  char* DatabaseType;
  char* HostName;
  char* User;
  char* Password;
  char* DatabaseName;
  int ServerPort;
  int Reconnect;

  vtkMySQLDatabasePrivate* const Private;

  vtkMySQLDatabase(const vtkMySQLDatabase &) VTK_DELETE_FUNCTION;
  void operator=(const vtkMySQLDatabase &) VTK_DELETE_FUNCTION;
};

#endif // vtkMySQLDatabase_h

