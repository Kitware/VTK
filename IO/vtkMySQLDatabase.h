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

#include <mysql.h> // needed for MYSQL typedef

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
  // Set/get the hostname where the database server lives.  No
  // default.
  vtkSetStringMacro(HostName);
  vtkGetStringMacro(HostName);

  // Description:
  // Set/get the username to use while connecting to the database.
  vtkSetStringMacro(UserName);
  vtkGetStringMacro(UserName);
  
  // Description:
  // Set/get the password to use while connecting to the database.
  void SetPassword(const char *passwd);
  vtkGetStringMacro(Password);

  // Description:
  // Set/get the TCP port to use while connecting to the database.
  // Defaults to 3306.
  vtkSetMacro(Port, int);
  vtkGetMacro(Port, int);

  // Description:
  // Set/get the name of the database to open while connecting.
  // Changing this after the connection is already open has no effect.
  vtkSetStringMacro(DatabaseName);
  vtkGetStringMacro(DatabaseName);
  
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

protected:
  vtkMySQLDatabase();
  ~vtkMySQLDatabase();

private:
  char *HostName;
  char *UserName;
  char *Password;
  char *DatabaseName;
  int  Port;

  vtkStringArray *Tables;
  vtkStringArray *Record;

  MYSQL NullConnection;
  MYSQL *Connection;

  vtkMySQLDatabase(const vtkMySQLDatabase &); // Not implemented.
  void operator=(const vtkMySQLDatabase &); // Not implemented.
};

#endif // __vtkMySQLDatabase_h

