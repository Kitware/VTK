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
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
// .NAME vtkSQLDatabase - maintains a connection to an sql database
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
// .SECTION Thanks
// Thanks to Andrew Wilson from Sandia National Laboratories for his work
// on the database classes and for the SQLite example. Thanks to David Thompson 
// and Philippe Pebay from Sandia National Laboratories for implementing
// this class.
//
// .SECTION See Also
// vtkSQLQuery

#ifndef __vtkSQLDatabase_h
#define __vtkSQLDatabase_h

#include "vtkObject.h"

#include <vtkstd/string> // Because I really enjoy including headers

class vtkSQLQuery;
class vtkStringArray;

// This is a list of features that each database may or may not
// support.  As yet (December 2007) we don't provide access to most of
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
#define VTK_SQL_FEATURE_TRIGGERS                1009

class VTK_IO_EXPORT vtkSQLDatabase : public vtkObject
{
public:
  vtkTypeRevisionMacro(vtkSQLDatabase, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
// Basic data types for database columns
  enum DatabaseColumnType
    {
      SERIAL,
      SMALLINT,
      INTEGER,
      BIGINT,
      VARCHAR,
      TEXT,
      REAL,
      DOUBLE,
      BLOB,
      TIME,
      DATE,
      TIMESTAMP,
    };

// Types of indices that can be generated for database tables
  enum DatabaseIndexType
    {
      INDEX,       // Non-unique index of values in named columns
      UNIQUE,      // Index of values in named columns required to have at most one entry per pair of valid values.
      PRIMARY_KEY, // Like UNIQUE but additionally this serves as the primary key for the table to speed up insertions.
    };

// Events where database triggers can be registered.
  enum DatabaseTriggerType
    {
      BEFORE_INSERT, // Just before a row is inserted
      AFTER_INSERT,  // Just after a row is inserted
      BEFORE_UPDATE, // Just before a row's values are changed
      AFTER_UPDATE,  // Just after a row's values are changed
      BEFORE_DELETE, // Just before a row is deleted
      AFTER_DELETE,  // Just after a row is deleted
    };
//ETX

  // Description:
  // Open a new connection to the database.
  // You need to set up any database parameters before calling this function.
  // Returns true is the database was opened sucessfully, and false otherwise.
  virtual bool Open() = 0;

  // Description:
  // Close the connection to the database.
  virtual void Close() = 0;
  
  // Description:
  // Return whether the database has an open connection
  virtual bool IsOpen() = 0;

  // Description:
  // Return an empty query on this database.
  virtual vtkSQLQuery* GetQueryInstance() = 0;
  
   // Description:
  // Get the last error text from the database
  virtual const char* GetLastErrorText() = 0;
  
  // Description:
  // Get the list of tables from the database
  virtual vtkStringArray* GetTables() = 0;
    
  // Description:
  // Get the list of fields for a particular table
  virtual vtkStringArray* GetRecord(const char *table) = 0;

  // Description:
  // Return whether a feature is supported by the database.
  virtual bool IsSupported(int vtkNotUsed(feature)) { return false; }

  // Description:
  // Create a the proper subclass given a URL
  // The URL format for SQL databases is a true URL of the form:
  //   'protocol://'[[username[':'password]'@']hostname[':'port]]'/'[dbname] .
  static vtkSQLDatabase* CreateFromURL( const char* URL );

  // Description:
  // Get the URL of the database.
  vtkGetStringMacro(URL);

protected:
  vtkSQLDatabase();
  ~vtkSQLDatabase();

  // Description:
  // Set the URL of the database.
  vtkSetStringMacro(URL);

  // Description:
  // Set the last error text of the database.
  vtkSetStringMacro(LastErrorText);

  char* URL;
  char *LastErrorText;

private:
  vtkSQLDatabase(const vtkSQLDatabase &); // Not implemented.
  void operator=(const vtkSQLDatabase &); // Not implemented.
};

#endif // __vtkSQLDatabase_h
