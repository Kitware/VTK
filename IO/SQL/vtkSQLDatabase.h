/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkSQLDatabase.h

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
// .NAME vtkSQLDatabase - maintain a connection to an sql database
//
// .SECTION Description
// Abstract base class for all SQL database connection classes.
// Manages a connection to the database, and is responsible for creating
// instances of the associated vtkSQLQuery objects associated with this
// class in order to perform execute queries on the database.
// To allow connections to a new type of database, create both a subclass
// of this class and vtkSQLQuery, and implement the required functions:
//
// Open() - open the database connection, if possible.
// Close() - close the connection.
// GetQueryInstance() - create and return an instance of the vtkSQLQuery
//                      subclass associated with the database type.
//
// The subclass should also provide API to set connection parameters.
//
// This class also provides the function EffectSchema to transform a
// database schema into a SQL database.
//
// .SECTION Thanks
// Thanks to Andrew Wilson from Sandia National Laboratories for his work
// on the database classes and for the SQLite example. Thanks to David Thompson
// and Philippe Pebay from Sandia National Laboratories for implementing
// this class.
//
// .SECTION See Also
// vtkSQLQuery
// vtkSQLDatabaseSchema

#ifndef __vtkSQLDatabase_h
#define __vtkSQLDatabase_h

#include "vtkIOSQLModule.h" // For export macro
#include "vtkObject.h"

#include "vtkStdString.h" // Because at least one method returns a vtkStdString

class vtkInformationObjectBaseKey;
class vtkSQLDatabaseSchema;
class vtkSQLQuery;
class vtkStringArray;

// This is a list of features that each database may or may not
// support.  As yet (April 2008) we don't provide access to most of
// them.
#define VTK_SQL_FEATURE_TRANSACTIONS            1000
#define VTK_SQL_FEATURE_QUERY_SIZE              1001
#define VTK_SQL_FEATURE_BLOB                    1002
#define VTK_SQL_FEATURE_UNICODE                 1003
#define VTK_SQL_FEATURE_PREPARED_QUERIES        1004
#define VTK_SQL_FEATURE_NAMED_PLACEHOLDERS      1005
#define VTK_SQL_FEATURE_POSITIONAL_PLACEHOLDERS 1006
#define VTK_SQL_FEATURE_LAST_INSERT_ID          1007
#define VTK_SQL_FEATURE_BATCH_OPERATIONS        1008
#define VTK_SQL_FEATURE_TRIGGERS                1009 // supported

// Default size for columns types which require a size to be specified
// (i.e., VARCHAR), when no size has been specified
#define VTK_SQL_DEFAULT_COLUMN_SIZE 32

class VTKIOSQL_EXPORT vtkSQLDatabase : public vtkObject
{
public:
  vtkTypeMacro(vtkSQLDatabase, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Open a new connection to the database.
  // You need to set up any database parameters before calling this function.
  // For database connections that do not require a password, pass an empty string.
  // Returns true is the database was opened successfully, and false otherwise.
  virtual bool Open(const char* password) = 0;

  // Description:
  // Close the connection to the database.
  virtual void Close() = 0;

  // Description:
  // Return whether the database has an open connection.
  virtual bool IsOpen() = 0;

  // Description:
  // Return an empty query on this database.
  virtual vtkSQLQuery* GetQueryInstance() = 0;

  // Description:
  // Did the last operation generate an error
  virtual bool HasError() = 0;

  // Description:
  // Get the last error text from the database
  // I'm using const so that people do NOT
  // use the standard vtkGetStringMacro in their
  // implementation, because 99% of the time that
  // will not be the correct thing to do...
  virtual const char* GetLastErrorText() = 0;

  // Description:
  // Get the type of the database (e.g. mysql, psql,..).
  virtual char* GetDatabaseType() = 0;

  // Description:
  // Get the list of tables from the database.
  virtual vtkStringArray* GetTables() = 0;

  // Description:
  // Get the list of fields for a particular table.
  virtual vtkStringArray* GetRecord(const char *table) = 0;

  // Description:
  // Return whether a feature is supported by the database.
  virtual bool IsSupported(int vtkNotUsed(feature)) { return false; }

  // Description:
  // Get the URL of the database.
  virtual vtkStdString GetURL() = 0;

  // Description:
  // Return the SQL string with the syntax of the preamble following a
  // "CREATE TABLE" SQL statement.
  // NB: by default, this method returns an empty string.
  // It must be overwritten for those SQL backends which allow such
  // preambles such as, e.g., MySQL.
  virtual vtkStdString GetTablePreamble( bool ) { return vtkStdString(); }

  // Description:
  // Return the SQL string with the syntax to create a column inside a
  // "CREATE TABLE" SQL statement.
  // NB: this method implements the following minimally-portable syntax:
  // <column name> <column type> <column attributes>
  // It must be overwritten for those SQL backends which have a different
  // syntax such as, e.g., MySQL.
  virtual vtkStdString GetColumnSpecification( vtkSQLDatabaseSchema* schema,
                                               int tblHandle,
                                               int colHandle );

  // Description:
  // Return the SQL string with the syntax to create an index inside a
  // "CREATE TABLE" SQL statement.
  // NB1: this method implements the following minimally-portable syntax:
  // <index type> [<index name>] (<column name 1>,... )
  // It must be overwritten for those SQL backends which have a different
  // syntax such as, e.g., MySQL.
  // NB2: this method does not assume that INDEX creation is supported
  // within a CREATE TABLE statement. Therefore, should such an INDEX arise
  // in the schema, a CREATE INDEX statement is returned and skipped is
  // set to true. Otherwise, skipped will always be returned false.
  virtual vtkStdString GetIndexSpecification( vtkSQLDatabaseSchema* schema,
                                              int tblHandle,
                                              int idxHandle,
                                              bool& skipped );

  // Description:
  // Return the SQL string with the syntax to create a trigger using a
  // "CREATE TRIGGER" SQL statement.
  // NB1: support is contingent on VTK_FEATURE_TRIGGERS being recognized as
  // a supported feature. Not all backends (e.g., SQLite) support it.
  // NB2: this method implements the following minimally-portable syntax:
  // <trigger name> {BEFORE | AFTER} <event> ON <table name> FOR EACH ROW <trigger action>
  // It must be overwritten for those SQL backends which have a different
  // syntax such as, e.g., PostgreSQL.
  virtual vtkStdString GetTriggerSpecification( vtkSQLDatabaseSchema* schema,
                                                int tblHandle,
                                                int trgHandle );

  // Description:
  // Create a the proper subclass given a URL.
  // The URL format for SQL databases is a true URL of the form:
  //   'protocol://'[[username[':'password]'@']hostname[':'port]]'/'[dbname] .
  static vtkSQLDatabase* CreateFromURL( const char* URL );

  // Description:
  // Effect a database schema.
  virtual bool EffectSchema( vtkSQLDatabaseSchema*, bool dropIfExists = false );

//BTX
  // Description:
  // Type for CreateFromURL callback.
  typedef vtkSQLDatabase* (*CreateFunction)(const char* URL);
//ETX

  // Description:
  // Provides mechanism to register/unregister additional callbacks to create
  // concrete subclasses of vtkSQLDatabase to handle different protocols.
  // The registered callbacks are tried in the order they are registered.
  static void RegisterCreateFromURLCallback(CreateFunction callback);
  static void UnRegisterCreateFromURLCallback(CreateFunction callback);
  static void UnRegisterAllCreateFromURLCallbacks();

  // Description:
  // Stores the database class pointer as an information key. This is currently
  // used to store database pointers as part of 'data on demand' data objects.
  // For example: The application may have a table/tree/whatever of documents,
  // the data structure is storing the meta-data but not the full text. Further
  // down the pipeline algorithms or views may want to retrieve additional
  // information (full text)for specific documents.
  static vtkInformationObjectBaseKey* DATABASE();

//BTX
protected:
  vtkSQLDatabase();
  ~vtkSQLDatabase();

  // Description:
  // Subclasses should override this method to determine connection parameters
  // given the URL. This is called by CreateFromURL() to initialize the instance.
  // Look at CreateFromURL() for details about the URL format.
  virtual bool ParseURL( const char* url ) = 0;

private:
  vtkSQLDatabase(const vtkSQLDatabase &); // Not implemented.
  void operator=(const vtkSQLDatabase &); // Not implemented.

  // Description;
  // Datastructure used to store registered callbacks.
  class vtkCallbackVector;
  static vtkCallbackVector* Callbacks;
//ETX
};

#endif // __vtkSQLDatabase_h
