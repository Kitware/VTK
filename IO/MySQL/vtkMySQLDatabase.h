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
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkMySQLDatabase* New();

  /**
   * Open a new connection to the database.  You need to set the
   * filename before calling this function.  Returns true if the
   * database was opened successfully; false otherwise.
   */
  bool Open(const char* password = 0) override;

  /**
   * Close the connection to the database.
   */
  void Close() override;

  /**
   * Return whether the database has an open connection
   */
  bool IsOpen() override;

  /**
   * Return an empty query on this database.
   */
  vtkSQLQuery* GetQueryInstance() override;

  /**
   * Get the list of tables from the database
   */
  vtkStringArray* GetTables() override;

  /**
   * Get the list of fields for a particular table
   */
  vtkStringArray* GetRecord(const char* table) override;

  /**
   * Return whether a feature is supported by the database.
   */
  bool IsSupported(int feature) override;

  /**
   * Did the last operation generate an error
   */
  bool HasError() override;

  /**
   * Get the last error text from the database
   */
  const char* GetLastErrorText() override;

  //@{
  /**
   * String representing database type (e.g. "mysql").
   */
  const char* GetDatabaseType() override { return this->DatabaseType; }
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
  vtkSetMacro(Reconnect, int);
  vtkGetMacro(Reconnect, int);
  vtkBooleanMacro(Reconnect, int);
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
  vtkStdString GetURL() override;

  /**
   * Return the SQL string with the syntax of the preamble following a
   * "CREATE TABLE" SQL statement.
   * NB: this method implements the MySQL-specific IF NOT EXISTS syntax,
   * used when b = false.
   */
  vtkStdString GetTablePreamble(bool b) override { return b ? vtkStdString() : "IF NOT EXISTS "; }

  /**
   * Return the SQL string with the syntax to create a column inside a
   * "CREATE TABLE" SQL statement.
   * NB1: this method implements the MySQL-specific syntax:
   * \verbatim
   * `<column name>` <column type> <column attributes>
   * \endverbatim
   * NB2: if a column has type SERIAL in the schema, this will be turned
   * into INT NOT nullptr AUTO_INCREMENT. Therefore, one should not pass
   * NOT nullptr as an attribute of a column whose type is SERIAL.
   */
  vtkStdString GetColumnSpecification(
    vtkSQLDatabaseSchema* schema, int tblHandle, int colHandle) override;

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
  vtkStdString GetIndexSpecification(
    vtkSQLDatabaseSchema* schema, int tblHandle, int idxHandle, bool& skipped) override;

  /**
   * Create a new database, optionally dropping any existing database of the same name.
   * Returns true when the database is properly created and false on failure.
   */
  bool CreateDatabase(const char* dbName, bool dropExisting);

  /**
   * Drop a database if it exists.
   * Returns true on success and false on failure.
   */
  bool DropDatabase(const char* dbName);

  /**
   * Overridden to determine connection parameters given the URL.
   * This is called by CreateFromURL() to initialize the instance.
   * Look at CreateFromURL() for details about the URL format.
   */
  bool ParseURL(const char* url) override;

protected:
  vtkMySQLDatabase();
  ~vtkMySQLDatabase() override;

private:
  // We want this to be private, a user of this class
  // should not be setting this for any reason
  vtkSetStringMacro(DatabaseType);

  vtkStringArray* Tables;
  vtkStringArray* Record;

  char* DatabaseType;
  char* HostName;
  char* User;
  char* Password;
  char* DatabaseName;
  int ServerPort;
  int Reconnect;

  vtkMySQLDatabasePrivate* const Private;

  vtkMySQLDatabase(const vtkMySQLDatabase&) = delete;
  void operator=(const vtkMySQLDatabase&) = delete;
};

#endif // vtkMySQLDatabase_h
