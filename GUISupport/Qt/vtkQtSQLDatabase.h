/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtSQLDatabase.h

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
// .NAME vtkQtSQLDatabase - maintains a connection to an sql database
//
// .SECTION Description
// Implements a vtkSQLDatabase using an underlying Qt QSQLDatabase.

#ifndef __vtkQtSQLDatabase_h
#define __vtkQtSQLDatabase_h

// Check for Qt SQL module before defining this class.
#include <qglobal.h>
#if (QT_EDITION & QT_MODULE_SQL)

#include "QVTKWin32Header.h"
#include "vtkSQLDatabase.h"

#include <QtSql/QSqlDatabase>

class vtkSQLQuery;
class vtkStringArray;

class QVTK_EXPORT vtkQtSQLDatabase : public vtkSQLDatabase
{
public:
  static vtkQtSQLDatabase* New();
  vtkTypeMacro(vtkQtSQLDatabase, vtkSQLDatabase);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Open a new connection to the database.
  // You need to set up any database parameters before calling this function.
  // Returns true is the database was opened sucessfully, and false otherwise.
  virtual bool Open(const char* password);

  // Description:
  // Close the connection to the database.
  virtual void Close();
  
  // Description:
  // Return whether the database has an open connection
  virtual bool IsOpen();

  // Description:
  // Return an empty query on this database.
  virtual vtkSQLQuery* GetQueryInstance();
  
  // Description:
  // Get the list of tables from the database
  vtkStringArray* GetTables();
  
  // Description:
  // Get the list of fields for a particular table
  vtkStringArray* GetRecord(const char *table);

  // Description:
  // Returns a list of columns for a particular table.
  // Note that this is mainly for use with the VTK parallel server.
  // Serial VTK developers should prefer to use GetRecord() instead.
  vtkStringArray* GetColumns();
  
  // Description:
  // Set the table used by GetColumns()
  // Note that this is mainly for use with the VTK parallel server.
  // Serial VTK developers should prefer to use GetRecord() instead.
  void SetColumnsTable(const char* table);

  // Description:
  // Return whether a feature is supported by the database.
  virtual bool IsSupported(int feature);

  // Description:
  // Did the last operation generate an error
  bool HasError();
  
  // Description:
  // Get the last error text from the database
  const char* GetLastErrorText();
  
  // Description:
  // String representing Qt database type (e.g. "mysql").
  vtkGetStringMacro(DatabaseType);
  vtkSetStringMacro(DatabaseType);

  // Description:
  // The database server host name.
  vtkSetStringMacro(HostName);
  vtkGetStringMacro(HostName);

  // Description:
  // The user name for connecting to the database server.
  vtkSetStringMacro(UserName);
  vtkGetStringMacro(UserName);

  // Description:
  // The name of the database to connect to.
  vtkSetStringMacro(DatabaseName);
  vtkGetStringMacro(DatabaseName);

  // Description:
  // Additional options for the database.
  vtkSetStringMacro(ConnectOptions);
  vtkGetStringMacro(ConnectOptions);

  // Description:
  // The port used for connecting to the database.
  vtkSetClampMacro(Port, int, 0, VTK_INT_MAX);
  vtkGetMacro(Port, int);
  
  // Description:
  // Create a the proper subclass given a URL.
  // The URL format for SQL databases is a true URL of the form:
  //   'protocol://'[[username[':'password]'@']hostname[':'port]]'/'[dbname] .
  static vtkSQLDatabase* CreateFromURL( const char* URL );
  
  // Description:
  // Get the URL of the database.
  virtual vtkStdString GetURL();

//BTX
protected:
  vtkQtSQLDatabase();
  ~vtkQtSQLDatabase();
  
  char* DatabaseType;
  char* HostName;
  char* UserName;
  char* DatabaseName;
  int Port;
  char* ConnectOptions;

  QSqlDatabase QtDatabase;

  friend class vtkQtSQLQuery;

  // Description:
  // Overridden to determine connection paramters given the URL. 
  // This is called by CreateFromURL() to initialize the instance.
  // Look at CreateFromURL() for details about the URL format.
  virtual bool ParseURL(const char* url);
private:
  
  // Storing the tables in the database, this array
  // is accessable through GetTables() method
  vtkStringArray *myTables;
  
  // Storing the currect record list from any one
  // of the tables in the database, this array is
  // accessable through GetRecord(const char *table)
  vtkStringArray *currentRecord;
  
  // Used to assign unique identifiers for database instances
  static int id;
  
  vtkQtSQLDatabase(const vtkQtSQLDatabase &); // Not implemented.
  void operator=(const vtkQtSQLDatabase &); // Not implemented.
//ETX
};

#endif // (QT_EDITION & QT_MODULE_SQL)
#endif // __vtkQtSQLDatabase_h

