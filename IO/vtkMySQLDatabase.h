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
/*----------------------------------------------------------------------------
  Copyright (c) Sandia Corporation
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
  ----------------------------------------------------------------------------*/
// .NAME vtkMySQLDatabase - maintain a connection to a MySQL database
//
// .SECTION Description
//
// This class provides a VTK interface to MySQL
// (http://www.mysql.com).  Unlike file-based databases like SQLite, you
// talk to MySQL through a client/server connection.  You must specify
// the hostname, (optional) port to connect to, username, password and
// database name in order to connect.
//
// .SECTION See Also
// vtkMySQLQuery

#ifndef __vtkMySQLDatabase_h
#define __vtkMySQLDatabase_h

#include "vtkSQLDatabase.h"

#ifdef _WIN32
# include <winsock.h> // mysql.h relies on the typedefs from here
#endif

#include <mysql.h> // needed for MYSQL typedefs

class vtkSQLQuery;
class vtkMySQLQuery;
class vtkStringArray;

class VTK_IO_EXPORT vtkMySQLDatabase : public vtkSQLDatabase
{
//BTX
  friend class vtkMySQLQuery;
//ETX

public:
  vtkTypeRevisionMacro(vtkMySQLDatabase, vtkSQLDatabase);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkMySQLDatabase *New();

  // Description:
  // Open a new connection to the database.  You need to set the
  // filename before calling this function.  Returns true if the
  // database was opened successfully; false otherwise.
  bool Open();

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
  // Get the list of tables from the database
  vtkStringArray* GetTables();
    
  // Description:
  // Get the list of fields for a particular table
  vtkStringArray* GetRecord(const char *table);

  // Description:
  // Return whether a feature is supported by the database.
  bool IsSupported(int feature);
  
  // Description:
  // Did the last operation generate an error
  bool HasError();
  
  // Description:
  // Get the last error text from the database
  const char* GetLastErrorText();
  
  // Description:
  // String representing database type (e.g. "mysql").
  vtkGetStringMacro(DatabaseType);

  // Description:
  // The database server host name.
  vtkSetStringMacro(HostName);
  vtkGetStringMacro(HostName);

  // Description:
  // The user name for connecting to the database server.
  vtkSetStringMacro(UserName);
  vtkGetStringMacro(UserName);

  // Description:
  // The user's password for connecting to the database server.
  vtkSetStringMacro(Password);
  vtkGetStringMacro(Password);

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
  vtkSetClampMacro(ServerPort, int, 0, VTK_INT_MAX);
  vtkGetMacro(ServerPort, int);
  
  
  
  // Description:
  // Get the URL of the database.
  virtual vtkStdString GetURL();

private:
  vtkMySQLDatabase();
  ~vtkMySQLDatabase();
  
  // We want this to be private, a user of this class
  // should not be setting this for any reason
  vtkSetStringMacro(DatabaseType);
  
  vtkStringArray *Tables;
  vtkStringArray *Record;

  MYSQL NullConnection;
  MYSQL *Connection;
  
  char* DatabaseType;
  char* HostName;
  char* UserName;
  char* Password;
  char* DatabaseName;
  int ServerPort;
  char* ConnectOptions;

  vtkMySQLDatabase(const vtkMySQLDatabase &); // Not implemented.
  void operator=(const vtkMySQLDatabase &); // Not implemented.
};

#endif // __vtkMySQLDatabase_h

