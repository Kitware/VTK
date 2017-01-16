/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkSQLDatabaseSchema.h

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
 * @class   vtkSQLDatabaseSchema
 * @brief   represent an SQL database schema
 *
 *
 * This class stores the information required to create
 * an SQL database from scratch.
 * Information on each table's columns, indices, and triggers is stored.
 * You may also store an arbitrary number of preamble statements, intended
 * to be executed before any tables are created;
 * this provides a way to create procedures or functions that may be
 * invoked as part of a trigger action.
 * Triggers and table options may be specified differently for each backend
 * database type you wish to support.
 *
 * @par Thanks:
 * Thanks to Philippe Pebay and David Thompson from Sandia National
 * Laboratories for implementing this class.
 *
 * @sa
 * vtkSQLDatabase
*/

#ifndef vtkSQLDatabaseSchema_h
#define vtkSQLDatabaseSchema_h

#include "vtkIOSQLModule.h" // For export macro
#include "vtkObject.h"

#include <cstdarg> // Because one method has a variable list of arguments

// This is a list of known supported VTK SQL backend classes.
// A particular SQL backend does not have to be listed here to be supported, but
// these macros allow for the specification of SQL backend-specific database schema items.
#define VTK_SQL_ALLBACKENDS      "*" // works for all backends
#define VTK_SQL_MYSQL            "vtkMySQLDatabase"
#define VTK_SQL_POSTGRESQL       "vtkPostgreSQLDatabase"
#define VTK_SQL_SQLITE           "vtkSQLiteDatabase"

class vtkSQLDatabaseSchemaInternals;

class VTKIOSQL_EXPORT vtkSQLDatabaseSchema : public vtkObject
{
 public:
  vtkTypeMacro(vtkSQLDatabaseSchema, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  static vtkSQLDatabaseSchema* New();

  /**
   * Basic data types for database columns
   */
  enum DatabaseColumnType
  {
      SERIAL    = 0, // specifying the indices explicitly to prevent bad compiler mishaps
      SMALLINT  = 1,
      INTEGER   = 2,
      BIGINT    = 3,
      VARCHAR   = 4,
      TEXT      = 5,
      REAL      = 6,
      DOUBLE    = 7,
      BLOB      = 8,
      TIME      = 9,
      DATE      = 10,
      TIMESTAMP = 11
  };

  /**
   * Types of indices that can be generated for database tables
   */
  enum DatabaseIndexType
  {
      INDEX       = 0, // Non-unique index of values in named columns
      UNIQUE      = 1, // Index of values in named columns required to have at most one entry per pair of valid values.
      PRIMARY_KEY = 2 // Like UNIQUE but additionally this serves as the primary key for the table to speed up insertions.
  };

  /**
   * Events where database triggers can be registered.
   */
  enum DatabaseTriggerType
  {
      BEFORE_INSERT = 0, // Just before a row is inserted
      AFTER_INSERT  = 1,  // Just after a row is inserted
      BEFORE_UPDATE = 2, // Just before a row's values are changed
      AFTER_UPDATE  = 3,  // Just after a row's values are changed
      BEFORE_DELETE = 4, // Just before a row is deleted
      AFTER_DELETE  = 5  // Just after a row is deleted
  };

  /**
   * Add a preamble to the schema
   * This can be used, in particular, to create functions and/or
   * load languages in a backend-specific manner.
   * Example usage:
   * vtkSQLDatabaseSchema* schema = vtkSQLDatabaseSchema::New();
   * schema->SetName( "Example" );
   * schema->AddPreamble( "dropPLPGSQL", "DROP LANGUAGE IF EXISTS PLPGSQL CASCADE", VTK_SQL_POSTGRESQL );
   * schema->AddPreamble( "loadPLPGSQL", "CREATE LANGUAGE PLPGSQL", VTK_SQL_POSTGRESQL );
   * schema->AddPreamble( "createsomefunction",
   * "CREATE OR REPLACE FUNCTION somefunction() RETURNS TRIGGER AS $btable$ "
   * "BEGIN "
   * "INSERT INTO btable (somevalue) VALUES (NEW.somenmbr); "
   * "RETURN NEW; "
   * "END; $btable$ LANGUAGE PLPGSQL",
   * VTK_SQL_POSTGRESQL );
   */
  virtual int AddPreamble(
    const char* preName, const char* preAction,
    const char* preBackend = VTK_SQL_ALLBACKENDS );

  /**
   * Add a table to the schema
   */
  virtual int AddTable( const char* tblName );

  //@{
  /**
   * Add a column to table.

   * The returned value is a column handle or -1 if an error occurred.
   */
  virtual int AddColumnToTable(
    int tblHandle, int colType, const char* colName,
    int colSize, const char* colAttribs );
  virtual int AddColumnToTable(
    const char* tblName, int colType, const char* colName,
    int colSize, const char* colAttribs )
  {
    return this->AddColumnToTable( this->GetTableHandleFromName( tblName ),
      colType, colName, colSize, colAttribs );
  }
  //@}

  //@{
  /**
   * Add an index to table.

   * The returned value is an index handle or -1 if an error occurred.
   */
  virtual int AddIndexToTable(
    int tblHandle, int idxType, const char* idxName );
  virtual int AddIndexToTable(
    const char* tblName, int idxType, const char* idxName )
  {
    return this->AddIndexToTable( this->GetTableHandleFromName( tblName ),
      idxType, idxName );
  }
  //@}

  //@{
  /**
   * Add a column to a table index.

   * The returned value is an index-column handle or -1 if an error occurred.
   */
  virtual int AddColumnToIndex( int tblHandle, int idxHandle, int colHandle );
  virtual int AddColumnToIndex(
    const char* tblName, const char* idxName, const char* colName )
  {
    int tblHandle = this->GetTableHandleFromName( tblName );
    return this->AddColumnToIndex( tblHandle,
      this->GetIndexHandleFromName( tblName, idxName ),
      this->GetColumnHandleFromName( tblName, colName ) );
  }
  //@}

  //@{
  /**
   * Add a (possibly backend-specific) trigger action to a table.

   * Triggers must be given unique, non-NULL names as some database backends require them.
   * The returned value is a trigger handle or -1 if an error occurred.
   */
  virtual int AddTriggerToTable(
    int tblHandle, int trgType, const char* trgName,
    const char* trgAction, const char* trgBackend = VTK_SQL_ALLBACKENDS );
  virtual int AddTriggerToTable(
    const char* tblName, int trgType, const char* trgName,
    const char* trgAction, const char* trgBackend = VTK_SQL_ALLBACKENDS )
  {
    return this->AddTriggerToTable( this->GetTableHandleFromName( tblName ),
      trgType, trgName, trgAction, trgBackend );
  }
  //@}

  //@{
  /**
   * Add (possibly backend-specific) text to the end of a
   * CREATE TABLE (...) statement.

   * This is most useful for specifying storage semantics of tables
   * that are specific to the backend. For example, table options
   * can be used to specify the TABLESPACE of a PostgreSQL table or
   * the ENGINE of a MySQL table.

   * The returned value is an option handle or -1 if an error occurred.
   */
  virtual int AddOptionToTable(
    int tblHandle, const char* optStr,
    const char* optBackend = VTK_SQL_ALLBACKENDS );
  virtual int AddOptionToTable(
    const char* tblName, const char* optStr,
    const char* optBackend = VTK_SQL_ALLBACKENDS )
  {
    return this->AddOptionToTable( this->GetTableHandleFromName( tblName ),
      optStr, optBackend );
  }
  //@}

  /**
   * Given a preamble name, get its handle.
   */
  int GetPreambleHandleFromName( const char* preName );

  /**
   * Given a preamble handle, get its name.
   */
  const char* GetPreambleNameFromHandle( int preHandle );

  /**
   * Given a preamble handle, get its action.
   */
  const char* GetPreambleActionFromHandle( int preHandle );

  /**
   * Given a preamble handle, get its backend.
   */
  const char* GetPreambleBackendFromHandle( int preHandle );

  /**
   * Given a table name, get its handle.
   */
  int GetTableHandleFromName( const char* tblName );

  /**
   * Given a table hanlde, get its name.
   */
  const char* GetTableNameFromHandle( int tblHandle );

  /**
   * Given the names of a table and an index, get the handle of the index in this table.
   */
  int GetIndexHandleFromName( const char* tblName, const char* idxName );

  /**
   * Given the handles of a table and an index, get the name of the index.
   */
  const char* GetIndexNameFromHandle( int tblHandle, int idxHandle );

  /**
   * Given the handles of a table and an index, get the type of the index.
   */
  int GetIndexTypeFromHandle( int tblHandle, int idxHandle );

  /**
   * Given the handles of a table, an index, and a column name, get the column name.
   */
  const char* GetIndexColumnNameFromHandle(
    int tblHandle, int idxHandle, int cnmHandle );

  /**
   * Given the names of a table and a column, get the handle of the column in this table.
   */
  int GetColumnHandleFromName( const char* tblName, const char* colName );

  /**
   * Given the handles of a table and a column, get the name of the column.
   */
  const char* GetColumnNameFromHandle( int tblHandle, int colHandle );

  /**
   * Given the handles of a table and a column, get the type of the column.
   */
  int GetColumnTypeFromHandle( int tblHandle, int colHandle );

  /**
   * Given the handles of a table and a column, get the size of the column.
   */
  int GetColumnSizeFromHandle( int tblHandle, int colHandle );

  /**
   * Given the handles of a table and a column, get the attributes of the column.
   */
  const char* GetColumnAttributesFromHandle( int tblHandle, int colHandle );

  /**
   * Given the names of a trigger and a table, get the handle of the trigger in this table.
   */
  int GetTriggerHandleFromName( const char* tblName, const char* trgName );

  /**
   * Given the handles of a table and a trigger, get the name of the trigger.
   */
  const char* GetTriggerNameFromHandle( int tblHandle, int trgHandle );

  /**
   * Given the handles of a table and a trigger, get the type of the trigger.
   */
  int GetTriggerTypeFromHandle( int tblHandle, int trgHandle );

  /**
   * Given the handles of a table and a trigger, get the action of the trigger.
   */
  const char* GetTriggerActionFromHandle( int tblHandle, int trgHandle );

  /**
   * Given the handles of a table and a trigger, get the backend of the trigger.
   */
  const char* GetTriggerBackendFromHandle( int tblHandle, int trgHandle );

  /**
   * Given the handles of a table and one of its options, return the text of the option.
   */
  const char* GetOptionTextFromHandle( int tblHandle, int optHandle );

  /**
   * Given the handles of a table and one of its options, get the backend of the option.
   */
  const char* GetOptionBackendFromHandle( int tblHandle, int trgHandle );

  /**
   * Reset the schema to its initial, empty state.
   */
  void Reset();

  /**
   * Get the number of preambles.
   */
  int GetNumberOfPreambles();

  /**
   * Get the number of tables.
   */
  int GetNumberOfTables();

  /**
   * Get the number of columns in a particular table .
   */
  int GetNumberOfColumnsInTable( int tblHandle );

  /**
   * Get the number of indices in a particular table .
   */
  int GetNumberOfIndicesInTable( int tblHandle );

  /**
   * Get the number of column names associated to a particular index in a particular table .
   */
  int GetNumberOfColumnNamesInIndex( int tblHandle, int idxHandle );

  /**
   * Get the number of triggers defined for a particular table.
   */
  int GetNumberOfTriggersInTable( int tblHandle );

  /**
   * Get the number of options associated with a particular table.
   */
  int GetNumberOfOptionsInTable( int tblHandle );

  //@{
  /**
   * Set/Get the name of the schema.
   */
  vtkSetStringMacro(Name);
  vtkGetStringMacro(Name);
  //@}

  // Tokens passed to AddTable to indicate the type of data that follows. Random integers chosen to prevent mishaps.
  enum VarargTokens
  {
      COLUMN_TOKEN       = 58,
      INDEX_TOKEN        = 63,
      INDEX_COLUMN_TOKEN = 65,
      END_INDEX_TOKEN    = 75,
      TRIGGER_TOKEN      = 81,
      OPTION_TOKEN       = 86,
      END_TABLE_TOKEN    = 99
  };

  /**
   * An unwrappable but useful routine to construct built-in schema.
   * Example usage:
   * int main()
   * {
   * vtkSQLDatabaseSchema* schema = vtkSQLDatabaseSchema::New();
   * schema->SetName( "Example" );
   * schema->AddTableMultipleArguments( "atable",
   * vtkSQLDatabaseSchema::COLUMN_TOKEN, vtkSQLDatabaseSchema::INTEGER, "tablekey",  0, "",
   * vtkSQLDatabaseSchema::COLUMN_TOKEN, vtkSQLDatabaseSchema::VARCHAR, "somename", 11, "NOT NULL",
   * vtkSQLDatabaseSchema::COLUMN_TOKEN, vtkSQLDatabaseSchema::BIGINT,  "somenmbr", 17, "DEFAULT 0",
   * vtkSQLDatabaseSchema::INDEX_TOKEN, vtkSQLDatabaseSchema::PRIMARY_KEY, "bigkey",
   * vtkSQLDatabaseSchema::INDEX_COLUMN_TOKEN, "tablekey",
   * vtkSQLDatabaseSchema::END_INDEX_TOKEN,
   * vtkSQLDatabaseSchema::INDEX_TOKEN,  vtkSQLDatabaseSchema::UNIQUE, "reverselookup",
   * vtkSQLDatabaseSchema::INDEX_COLUMN_TOKEN, "somename",
   * vtkSQLDatabaseSchema::INDEX_COLUMN_TOKEN, "somenmbr",
   * vtkSQLDatabaseSchema::END_INDEX_TOKEN,
   * vtkSQLDatabaseSchema::TRIGGER_TOKEN,  vtkSQLDatabaseSchema::AFTER_INSERT,
   * "InsertTrigger", "DO NOTHING", VTK_SQL_SQLITE,
   * vtkSQLDatabaseSchema::TRIGGER_TOKEN,  vtkSQLDatabaseSchema::AFTER_INSERT,
   * "InsertTrigger", "FOR EACH ROW EXECUTE PROCEDURE somefunction ()", VTK_SQL_POSTGRESQL,
   * vtkSQLDatabaseSchema::TRIGGER_TOKEN,  vtkSQLDatabaseSchema::AFTER_INSERT,
   * "InsertTrigger", "FOR EACH ROW INSERT INTO btable SET SomeValue = NEW.SomeNmbr", VTK_SQL_MYSQL,
   * vtkSQLDatabaseSchema::END_TABLE_TOKEN
   * );
   * return 0;
   * }
   */
  int AddTableMultipleArguments( const char* tblName, ... );

 protected:
  vtkSQLDatabaseSchema();
  ~vtkSQLDatabaseSchema() VTK_OVERRIDE;

  char* Name;

  class vtkSQLDatabaseSchemaInternals* Internals;

 private:
  vtkSQLDatabaseSchema(const vtkSQLDatabaseSchema &) VTK_DELETE_FUNCTION;
  void operator=(const vtkSQLDatabaseSchema &) VTK_DELETE_FUNCTION;
};

#endif // vtkSQLDatabaseSchema_h
