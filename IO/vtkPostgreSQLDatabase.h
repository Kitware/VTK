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
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
// .NAME vtkPostgreSQLDatabase - maintain a connection to a PostgreSQL database
//
// .SECTION Description
//
// PostgreSQL (http://www.postgres.org) is a BSD-licensed SQL database.
// It's large, fast, and can not be easily embedded
// inside other applications.  Its databases are stored in files that
// belong to another process.
//
// This class provides a VTK interface to PostgreSQL.  You do need to
// download external libraries: we need a copy of PostgreSQL 8 and libpqxx.
//
// .SECTION Thanks
// Thanks to David Thompson from Sandia National Laboratories for implementing
// this class based on Andy Wilson's vtkSQLiteDatabase class.
//
// .SECTION See Also
// vtkPostgreSQLQuery

#ifndef __vtkPostgreSQLDatabase_h
#define __vtkPostgreSQLDatabase_h

#include "vtkSQLDatabase.h"

class vtkSQLQuery;
class vtkPostgreSQLQuery;
class vtkStringArray;
class vtkPostgreSQLDatabasePrivate;

class VTK_IO_EXPORT vtkPostgreSQLDatabase : public vtkSQLDatabase
{
  //BTX
  friend class vtkPostgreSQLQuery;
  friend class vtkPostgreSQLQueryPrivate;
  //ETX

public:
  vtkTypeRevisionMacro(vtkPostgreSQLDatabase, vtkSQLDatabase);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkPostgreSQLDatabase *New();

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
  // Did the last operation generate an error
  virtual bool HasError();
  
  // Description:
  // Get the last error text from the database
  const char* GetLastErrorText();
  
  // Description:
  // String representing database type (e.g. "psql").
  vtkGetStringMacro(DatabaseType);

  // Description:
  // The database server host name.
  virtual void SetHostName( const char* );
  vtkGetStringMacro(HostName);

  // Description:
  // The user name for connecting to the database server.
  virtual void SetUserName( const char* );
  vtkGetStringMacro(UserName);

  // Description:
  // The user's password for connecting to the database server.
  virtual void SetPassword( const char* );
  vtkGetStringMacro(Password);

  // Description:
  // The name of the database to connect to.
  virtual void SetDatabaseName( const char* );
  vtkGetStringMacro(DatabaseName);

  // Description:
  // Additional options for the database.
  virtual void SetConnectOptions( const char* );
  vtkGetStringMacro(ConnectOptions);

  // Description:
  // The port used for connecting to the database.
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
  
  // Description:
  // Get a URL referencing the current database connection.
  // This is not well-defined if the HostName and DatabaseName
  // have not been set. The URL will be of the form
  // <code>'psql://'[username[':'password]'@']hostname[':'port]'/'database</code> .
  virtual vtkStdString GetURL();

  // Description:
  // Get the list of tables from the database
  vtkStringArray* GetTables();
    
  // Description:
  // Get the list of fields for a particular table
  vtkStringArray* GetRecord( const char* table );

  // Description:
  // Return whether a feature is supported by the database.
  bool IsSupported( int feature );

  // Description:
  // Return a list of databases on the server.
  vtkStringArray* GetDatabases();

  // Description:
  // Create a new database, optionally dropping any existing database of the same name.
  // Returns true when the database is properly created and false on failure.
  bool CreateDatabase( const char* dbName, bool dropExisting = false );

  // Description:
  // Drop a database if it exists.
  // Returns true on success and false on failure.
  bool DropDatabase( const char* dbName );

protected:
  vtkPostgreSQLDatabase();
  ~vtkPostgreSQLDatabase();

  vtkTimeStamp URLMTime;
  vtkPostgreSQLDatabasePrivate* Connection;
  vtkTimeStamp ConnectionMTime;
  char* DatabaseType;
  char* HostName;
  char* UserName;
  char* Password;
  char* DatabaseName;
  int ServerPort;
  char* ConnectOptions;
  
  vtkSetStringMacro(DatabaseType);

private:
  vtkPostgreSQLDatabase(const vtkPostgreSQLDatabase &); // Not implemented.
  void operator=(const vtkPostgreSQLDatabase &); // Not implemented.
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
    if (this->name) { delete [] this->name; } \
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
vtkSetStringPlusMTimeMacro(vtkPostgreSQLDatabase,UserName,URLMTime);
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

#endif // __vtkPostgreSQLDatabase_h
