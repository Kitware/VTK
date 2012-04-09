/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtSQLDatabase.cxx

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

// Check for Qt SQL module before defining this class.
#include <qglobal.h>
#if (QT_EDITION & QT_MODULE_SQL)

#include "vtkQtSQLDatabase.h"

#include "vtkObjectFactory.h"
#include "vtkQtSQLQuery.h"

#include <vtkStringArray.h>
#include <vtkVariant.h>

#include <QtSql/QtSql>
#include <QtSql/QSqlError>

#include <vtksys/SystemTools.hxx>
#include <vtksys/ios/sstream>

vtkStandardNewMacro(vtkQtSQLDatabase);

int vtkQtSQLDatabase::id = 0;

vtkSQLDatabase* vtkQtSQLDatabaseCreateFromURLCallback(const char* URL)
{
  return vtkQtSQLDatabase::CreateFromURL(URL);
}

class vtkQtSQLDatabaseInitializer
{
public:
  inline void Use()
    {
    }

  vtkQtSQLDatabaseInitializer()
    {
    vtkSQLDatabase::RegisterCreateFromURLCallback(
      vtkQtSQLDatabaseCreateFromURLCallback);
    }
};

static vtkQtSQLDatabaseInitializer vtkQtSQLDatabaseInitializerGlobal;

vtkQtSQLDatabase::vtkQtSQLDatabase()
{
  vtkQtSQLDatabaseInitializerGlobal.Use();
  this->DatabaseType = NULL;
  this->HostName = NULL;
  this->UserName = NULL;
  this->DatabaseName = NULL;
  this->Port = -1;
  this->ConnectOptions = NULL;
  this->myTables = vtkStringArray::New();
  this->currentRecord = vtkStringArray::New();
}

vtkQtSQLDatabase::~vtkQtSQLDatabase()
{
  this->SetDatabaseType(NULL);
  this->SetHostName(NULL);
  this->SetUserName(NULL);
  this->SetDatabaseName(NULL);
  this->SetConnectOptions(NULL);
  this->myTables->Delete();
  this->currentRecord->Delete();
}

bool vtkQtSQLDatabase::Open(const char* password)
{
  if (!QCoreApplication::instance())
    {
    vtkErrorMacro("Qt isn't initialized, you must create an instance of QCoreApplication before using this class.");
    return false;
    }

  if (this->DatabaseType == NULL)
    {
    vtkErrorMacro("Qt database type must be non-null.");
    return false;
    }
  
  // We have to assign a unique ID to each database connection, so
  // Qt doesn't blow-away existing connections
  const QString connection_name = QString::number(this->id++);
  this->QtDatabase = QSqlDatabase::addDatabase(this->DatabaseType, connection_name);
  
  if (this->HostName != NULL)
    {
    this->QtDatabase.setHostName(this->HostName);
    }
  if (this->DatabaseName != NULL)
    {
    this->QtDatabase.setDatabaseName(this->DatabaseName);
    }
  if (this->ConnectOptions != NULL)
    {
    this->QtDatabase.setConnectOptions(this->ConnectOptions);
    }
  if (this->Port >= 0)
    {
    this->QtDatabase.setPort(this->Port);
    }
  if (this->QtDatabase.open(this->UserName, password))
    {
    return true;
    }

  return false;
}

void vtkQtSQLDatabase::Close()
{
  this->QtDatabase.close();
}

bool vtkQtSQLDatabase::IsOpen()
{
  return this->QtDatabase.isOpen();
}

vtkSQLQuery* vtkQtSQLDatabase::GetQueryInstance()
{
  vtkQtSQLQuery* query = vtkQtSQLQuery::New();
  query->SetDatabase(this);
  return query;
}

bool vtkQtSQLDatabase::HasError()
{
  return (this->QtDatabase.lastError().number() != QSqlError::NoError);
}

const char* vtkQtSQLDatabase::GetLastErrorText()
{
  return this->QtDatabase.lastError().text().toAscii();
}

vtkStringArray* vtkQtSQLDatabase::GetTables()
{
  // Clear out any exiting stuff
  this->myTables->Initialize();
  
  // Yea... do different things depending on database type
  // Get tables on oracle is different
  if (this->QtDatabase.driverName() == "QOCI")
    {
    vtkSQLQuery *query = this->GetQueryInstance();
    query->SetQuery("select table_name from user_tables");
    query->Execute();
    while(query->NextRow())
      this->myTables->InsertNextValue(query->DataValue(0).ToString());
      
    // Okay done with query so delete
    query->Delete();
    }
  else
    {
    // Copy the table list from Qt database  
    QStringList tables = this->QtDatabase.tables(QSql::Tables);
    for (int i = 0; i < tables.size(); ++i)
      {
      this->myTables->InsertNextValue(tables.at(i).toAscii());
      }
    
    }
    
  return this->myTables;
}

vtkStringArray* vtkQtSQLDatabase::GetRecord(const char *table)
{
  // Clear any existing records
  currentRecord->Resize(0);
  
  QSqlRecord columns = this->QtDatabase.record(table);
  for (int i = 0; i < columns.count(); i++)
    {
    this->currentRecord->InsertNextValue(columns.fieldName(i).toAscii());
    }
    
  return currentRecord;
} 

vtkStringArray* vtkQtSQLDatabase::GetColumns()
{
  return this->currentRecord;
}

void vtkQtSQLDatabase::SetColumnsTable(const char* table)
{
  this->GetRecord(table);
}

bool vtkQtSQLDatabase::IsSupported(int feature)
{
  switch (feature)
    {
    case VTK_SQL_FEATURE_TRANSACTIONS:
      return this->QtDatabase.driver()->hasFeature(QSqlDriver::Transactions);

    case VTK_SQL_FEATURE_QUERY_SIZE:
      return this->QtDatabase.driver()->hasFeature(QSqlDriver::QuerySize);

    case VTK_SQL_FEATURE_BLOB:
      return this->QtDatabase.driver()->hasFeature(QSqlDriver::BLOB);

    case VTK_SQL_FEATURE_UNICODE:
      return this->QtDatabase.driver()->hasFeature(QSqlDriver::Unicode);

    case VTK_SQL_FEATURE_PREPARED_QUERIES:
      return this->QtDatabase.driver()->hasFeature(QSqlDriver::PreparedQueries);

    case VTK_SQL_FEATURE_NAMED_PLACEHOLDERS:
      return this->QtDatabase.driver()->hasFeature(QSqlDriver::NamedPlaceholders);

    case VTK_SQL_FEATURE_POSITIONAL_PLACEHOLDERS:
      return this->QtDatabase.driver()->hasFeature(QSqlDriver::PositionalPlaceholders);

    case VTK_SQL_FEATURE_LAST_INSERT_ID:
      return this->QtDatabase.driver()->hasFeature(QSqlDriver::LastInsertId);
      
    case VTK_SQL_FEATURE_BATCH_OPERATIONS:
      return this->QtDatabase.driver()->hasFeature(QSqlDriver::BatchOperations);

    default:
    {
    vtkErrorMacro(<< "Unknown SQL feature code " << feature << "!  See "
                  << "vtkSQLDatabase.h for a list of possible features.");
    return false;
    }
    }
}

void vtkQtSQLDatabase::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "DatabaseType: " << (this->DatabaseType ? this->DatabaseType : "NULL") << endl;
  os << indent << "HostName: " << (this->HostName ? this->HostName : "NULL") << endl;
  os << indent << "UserName: " << (this->UserName ? this->UserName : "NULL") << endl;
  os << indent << "DatabaseName: " << (this->DatabaseName ? this->DatabaseName : "NULL") << endl;
  os << indent << "Port: " << this->Port << endl;
  os << indent << "ConnectOptions: " << (this->ConnectOptions ? this->ConnectOptions : "NULL") << endl;
}

// ----------------------------------------------------------------------
bool vtkQtSQLDatabase::ParseURL(const char* URL)
{
  std::string protocol;
  std::string username; 
  std::string unused;
  std::string hostname; 
  std::string dataport; 
  std::string database;
  std::string dataglom;
  
  // SQLite is a bit special so lets get that out of the way :)
  if ( ! vtksys::SystemTools::ParseURLProtocol( URL, protocol, dataglom))
    {
    vtkGenericWarningMacro( "Invalid URL: " << URL );
    return false;
    }
  
  if ( protocol == "sqlite" )
    {
    this->SetDatabaseType("QSQLITE");
    this->SetDatabaseName(dataglom.c_str());
    return true;
    }
    
  // Okay now for all the other database types get more detailed info
  if ( ! vtksys::SystemTools::ParseURL( URL, protocol, username,
                                        unused, hostname, dataport, database) )
    {
    vtkGenericWarningMacro( "Invalid URL: " << URL );
    return false;
    }
    
  // Create Qt 'version' of database prototcol type
  QString qtType;
  qtType = protocol.c_str();
  qtType = "Q" + qtType.toUpper();
  
  this->SetDatabaseType(qtType.toAscii());
  this->SetUserName(username.c_str());
  this->SetHostName(hostname.c_str());
  this->SetPort(atoi(dataport.c_str()));
  this->SetDatabaseName(database.c_str());
  return true;
}

// ----------------------------------------------------------------------
vtkSQLDatabase* vtkQtSQLDatabase::CreateFromURL( const char* URL )
{
  vtkQtSQLDatabase* qt_db = vtkQtSQLDatabase::New();
  if (qt_db->ParseURL(URL))
    {
    return qt_db;
    }
  qt_db->Delete();
  return NULL;
}

// ----------------------------------------------------------------------
vtkStdString vtkQtSQLDatabase::GetURL()
{
  vtkStdString url;
  url = this->GetDatabaseType();
  url += "://";
  url += this->GetUserName();
  url += "@";
  url += this->GetHostName();
  url += ":";
  url += this->GetPort();
  url += "/";
  url += this->GetDatabaseName();
  return url;
}

#endif // (QT_EDITION & QT_MODULE_SQL)
