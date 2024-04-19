// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
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
 * written to disk, pass in the URL <tt>sqlite://:memory:</tt>; otherwise,
 * specify the file path by passing the URL <tt>sqlite://\<file_path\></tt>.
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

VTK_ABI_NAMESPACE_BEGIN
class vtkSQLQuery;
class vtkSQLiteQuery;
class vtkStringArray;
class vtkSQLiteDatabaseInternals;

class VTKIOSQL_EXPORT vtkSQLiteDatabase : public vtkSQLDatabase
{

  friend class vtkSQLiteQuery;

public:
  vtkTypeMacro(vtkSQLiteDatabase, vtkSQLDatabase);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkSQLiteDatabase* New();

  enum
  {
    USE_EXISTING,
    USE_EXISTING_OR_CREATE,
    CREATE_OR_CLEAR,
    CREATE
  };

  ///@{
  /**
   * Open a new connection to the database.  You need to set the
   * filename before calling this function.  Returns true if the
   * database was opened successfully; false otherwise.
   * - USE_EXISTING (default) - Fail if the file does not exist.
   * - USE_EXISTING_OR_CREATE - Create a new file if necessary.
   * - CREATE_OR_CLEAR - Create new or clear existing file.
   * - CREATE - Create new, fail if file exists.
   */
  bool Open(const char* password) override;
  bool Open(const char* password, int mode);
  ///@}

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

  ///@{
  /**
   * String representing database type (e.g. "sqlite").
   */
  const char* GetDatabaseType() override { return this->DatabaseType; }
  ///@}

  ///@{
  /**
   * String representing the database filename.
   */
  vtkGetFilePathMacro(DatabaseFileName);
  vtkSetFilePathMacro(DatabaseFileName);
  ///@}

  /**
   * Get the URL of the database.
   */
  vtkStdString GetURL() override;

  /**
   * Return the SQL string with the syntax to create a column inside a
   * "CREATE TABLE" SQL statement.
   * NB: this method implements the SQLite-specific syntax:
   * \code
   * <column name> <column type> <column attributes>
   * \endcode
   */
  vtkStdString GetColumnSpecification(
    vtkSQLDatabaseSchema* schema, int tblHandle, int colHandle) override;

protected:
  vtkSQLiteDatabase();
  ~vtkSQLiteDatabase() override;

  /**
   * Overridden to determine connection parameters given the URL.
   * This is called by CreateFromURL() to initialize the instance.
   * Look at CreateFromURL() for details about the URL format.
   */
  bool ParseURL(const char* url) override;

private:
  vtkSQLiteDatabaseInternals* Internal;

  // We want this to be private, a user of this class
  // should not be setting this for any reason
  vtkSetStringMacro(DatabaseType);

  vtkStringArray* Tables;

  char* DatabaseType;
  char* DatabaseFileName;

  vtkStdString TempURL;

  vtkSQLiteDatabase(const vtkSQLiteDatabase&) = delete;
  void operator=(const vtkSQLiteDatabase&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkSQLiteDatabase_h
