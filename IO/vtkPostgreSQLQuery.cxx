/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPostgreSQLQuery.cxx

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
#include "vtkPostgreSQLQuery.h"

#include "vtkObjectFactory.h"
#include "vtkPostgreSQLDatabase.h"
#include "vtkStringArray.h"
#include "vtkVariant.h"
#include "vtkVariantArray.h"
#include "vtkPostgreSQLDatabasePrivate.h"

#include <assert.h>

#include <vtksys/ios/sstream>

#define BEGIN_TRANSACTION "BEGIN TRANSACTION"
#define COMMIT_TRANSACTION "COMMIT"
#define ROLLBACK_TRANSACTION "ROLLBACK"

vtkCxxRevisionMacro(vtkPostgreSQLQuery, "1.2");
vtkStandardNewMacro(vtkPostgreSQLQuery);

class vtkPostgreSQLQueryPrivate : public vtkObject
{
public:
  vtkPostgreSQLQueryPrivate( vtkPostgreSQLDatabase* db )
    {
    this->Database = 0;
    this->LastErrorText = 0;

    assert( db );
    this->SetDatabase( db );
    }
  ~vtkPostgreSQLQueryPrivate()
    {
    this->SetDatabase( 0 );
    this->SetLastErrorText( 0 );
    }
  bool Execute( bool& active, const char* query )
    {
    if ( ! this->Database || ! this->Database->IsOpen() )
      {
      bool err = true;
      if ( this->Database )
        { // Perhaps the connection parameters were modified since the last open?
        err = this->Database->Open() ? false : true;
        }
      if ( err )
        {
        vtkErrorMacro( "Need a valid database connection to execute query \"" << query << "\"" );
        return false;
        }
      }

    pqxx::transaction<>* work = this->Database->Connection->Work;
    bool localWork = false; // automatically commit when done?
    try
      {
      if ( ! work )
        {
        work = new pqxx::transaction<>( this->Database->Connection->Connection, "LocalWork" );
        localWork = true;
        }
      this->Result = work->exec( query );
      this->Iterator = this->Result.begin();
      if ( localWork )
        {
        work->commit();
        delete work;
        }
      active = 1;
      }
    catch ( vtkstd::exception& e )
      {
      active = 0;
      this->SetLastErrorText( e.what() );
      return false;
      }
    this->SetLastErrorText( 0 );
    return true;
    }

  bool BeginTransaction()
    {
    if ( ! this->Database || ! this->Database->IsOpen() )
      {
      vtkErrorMacro( "Need a valid database connection to open a transaction" );
      return false;
      }
    return this->Database->Connection->BeginTransaction();
    }

  bool CommitTransaction()
    {
    if ( ! this->Database || ! this->Database->IsOpen() )
      {
      vtkErrorMacro( "Need a valid database connection to commit a transaction" );
      return false;
      }
    return this->Database->Connection->CommitTransaction();
    }

  bool RollbackTransaction()
    {
    if ( ! this->Database || ! this->Database->IsOpen() )
      {
      vtkErrorMacro( "Need a valid database connection to rollback a transaction" );
      return false;
      }
    return this->Database->Connection->RollbackTransaction();
    }

  int GetNumberOfFields()
    {
    return this->Result.columns();
    }

  const char* GetFieldName( int column )
    {
    return this->Result.column_name( column );
    }

  int GetFieldType( int column )
    {
    pqxx::oid ctyp = this->Result.column_type( column );
    return this->Database->Connection->GetVTKTypeFromOID( ctyp );
    }

  bool NextRow()
    {
    ++this->Iterator;
    return this->Iterator != this->Result.end();
    }

  vtkVariant DataValue( int column )
    {
    int colType = this->GetFieldType( column );
    switch ( colType )
      {
    case VTK_VOID:
      return vtkVariant();
    case VTK_BIT:
        {
        bool val;
        if ( ! this->Iterator[column].to( val ) )
          {
          return vtkVariant();
          }
        return vtkVariant( val );
        }
    case VTK_CHAR:
    case VTK_SIGNED_CHAR:
        {
        int val;
        if ( ! this->Iterator[column].to( val ) )
          {
          return vtkVariant();
          }
        return vtkVariant( (char)val );
        }
    case VTK_UNSIGNED_CHAR:
        {
        int val;
        if ( ! this->Iterator[column].to( val ) )
          {
          return vtkVariant();
          }
        return vtkVariant( (unsigned char)val );
        }
    case VTK_SHORT:
        {
        short val;
        if ( ! this->Iterator[column].to( val ) )
          {
          return vtkVariant();
          }
        return vtkVariant( val );
        }
    case VTK_UNSIGNED_SHORT:
        {
        unsigned short val;
        if ( ! this->Iterator[column].to( val ) )
          {
          return vtkVariant();
          }
        return vtkVariant( val );
        }
    case VTK_INT:
        {
        int val;
        if ( ! this->Iterator[column].to( val ) )
          {
          return vtkVariant();
          }
        return vtkVariant( val );
        }
    case VTK_UNSIGNED_INT:
        {
        unsigned int val;
        if ( ! this->Iterator[column].to( val ) )
          {
          return vtkVariant();
          }
        return vtkVariant( val );
        }
    case VTK_LONG:
        {
        long val;
        if ( ! this->Iterator[column].to( val ) )
          {
          return vtkVariant();
          }
        return vtkVariant( val );
        }
    case VTK_UNSIGNED_LONG:
        {
        unsigned long val;
        if ( ! this->Iterator[column].to( val ) )
          {
          return vtkVariant();
          }
        return vtkVariant( val );
        }
#ifdef VTK_TYPE_USE_LONG_LONG
    case VTK_LONG_LONG:
        {
        long long val;
        if ( ! this->Iterator[column].to( val ) )
          {
          return vtkVariant();
          }
        return vtkVariant( val );
        }
    case VTK_UNSIGNED_LONG_LONG:
        {
        unsigned long long val;
        if ( ! this->Iterator[column].to( val ) )
          {
          return vtkVariant();
          }
        return vtkVariant( val );
        }
#endif // VTK_TYPE_USE_LONG_LONG
    case VTK_FLOAT:
        {
        float val;
        if ( ! this->Iterator[column].to( val ) )
          {
          return vtkVariant();
          }
        return vtkVariant( val );
        }
    case VTK_DOUBLE:
        {
        double val;
        if ( ! this->Iterator[column].to( val ) )
          {
          return vtkVariant();
          }
        return vtkVariant( val );
        }
    case VTK_ID_TYPE:
        {
        vtkIdType val;
        if ( ! this->Iterator[column].to( val ) )
          {
          return vtkVariant();
          }
        return vtkVariant( val );
        }
    case VTK_STRING:
        {
        vtkstd::string val;
        if ( ! this->Iterator[column].to( val ) )
          {
          return vtkVariant();
          }
        return vtkVariant( val );
        }
      }
    return vtkVariant();
    }


  vtkSetObjectMacro(Database,vtkPostgreSQLDatabase);
  vtkSetStringMacro(LastErrorText);

  vtkPostgreSQLDatabase* Database;
  pqxx::result Result;
  pqxx::result::const_iterator Iterator;
  char* LastErrorText;
};

// ----------------------------------------------------------------------
vtkPostgreSQLQuery::vtkPostgreSQLQuery() 
{
  this->Transactor = 0;
}

// ----------------------------------------------------------------------
vtkPostgreSQLQuery::~vtkPostgreSQLQuery()
{
  if ( this->Transactor )
    {
    this->Transactor->Delete();
    }
}

// ----------------------------------------------------------------------
void vtkPostgreSQLQuery::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Transactor: ";
  if ( this->Transactor )
    {
    os << this->Transactor << "\n";
    }
  else
    {
    os << "(null)\n";
    }
}

// ----------------------------------------------------------------------
bool vtkPostgreSQLQuery::Execute()
{
  if ( this->Query == 0 )
    {
    vtkErrorMacro( "Cannot execute before a query has been set." );
    return false;
    }

  vtkPostgreSQLDatabase* db = vtkPostgreSQLDatabase::SafeDownCast( this->Database );
  assert( db );

  if ( this->Transactor && this->Transactor->Database != db )
    {
    this->Active = 0;
    // Delete any transactor not attached to the current database.
    this->Transactor->Delete();
    this->Transactor = 0;
    }

  if ( ! this->Transactor )
    {
    if ( ! db )
      {
      vtkErrorMacro( "No database on which to execute the query." );
      return false;
      }
    this->Transactor = new vtkPostgreSQLQueryPrivate( db );
    }

  return this->Transactor->Execute( this->Active, this->Query );
}

// ----------------------------------------------------------------------
int vtkPostgreSQLQuery::GetNumberOfFields()
{
  if ( ! this->Active || ! this->Transactor )
    {
    vtkErrorMacro( "Query is not active!" );
    return 0;
    }

  return this->Transactor->GetNumberOfFields();
}

// ----------------------------------------------------------------------
const char* vtkPostgreSQLQuery::GetFieldName( int column )
{
  if ( ! this->Active || ! this->Transactor )
    {
    vtkErrorMacro( "Query is not active!" );
    return 0;
    }
  else if ( column < 0 || column >= this->Transactor->GetNumberOfFields() )
    {
    vtkErrorMacro( "Illegal field index " << column );
    return 0;
    }
  return this->Transactor->GetFieldName( column );
}

// ----------------------------------------------------------------------
int vtkPostgreSQLQuery::GetFieldType( int column )
{
  if ( ! this->Active || ! this->Transactor )
    {
    vtkErrorMacro( "Query is not active!" );
    return -1;
    }
  else if ( column < 0 || column >= this->Transactor->GetNumberOfFields() )
    {
    vtkErrorMacro( "Illegal field index " << column );
    return -1;
    }
  return this->Transactor->GetFieldType( column );
}

// ----------------------------------------------------------------------
bool vtkPostgreSQLQuery::NextRow()
{
  if ( ! this->IsActive() || ! this->Transactor )
    {
    vtkErrorMacro( "Query is not active!" );
    return false;
    }

  return this->Transactor->NextRow();
}

// ----------------------------------------------------------------------
vtkVariant vtkPostgreSQLQuery::DataValue( vtkIdType column )
{
  if ( this->IsActive() == false )
    {
    vtkWarningMacro( "DataValue() called on inactive query" );
    return vtkVariant();
    }
  else if ( column < 0 || column >= this->GetNumberOfFields() )
    {
    vtkWarningMacro( "DataValue() called with out-of-range column index " << column );
    return vtkVariant();
    }

  return this->Transactor->DataValue( column );
}

// ----------------------------------------------------------------------
const char * vtkPostgreSQLQuery::GetLastErrorText()
{
  if ( ! this->Database )
    {
    return "No database";
    }
  if ( ! this->Transactor )
    {
    return "No active query";
    }
  return this->Transactor->LastErrorText;
}

// ----------------------------------------------------------------------
bool vtkPostgreSQLQuery::HasError()
{
  if ( ! this->Database )
    {
    return false;
    }
  if ( ! this->Transactor )
    {
    return false;
    }
  return ( this->Transactor->LastErrorText != 0 );
}

// ----------------------------------------------------------------------
bool vtkPostgreSQLQuery::BeginTransaction()
{
  if ( this->Transactor )
    {
    vtkErrorMacro(<<"Cannot start a transaction.  One is already in progress.");
    return false;
    }

  vtkPostgreSQLDatabase* db = vtkPostgreSQLDatabase::SafeDownCast( this->Database );
  assert( db );

  this->Transactor = new vtkPostgreSQLQueryPrivate( db );
  if ( this->Transactor )
    {
    vtkErrorMacro(<<"Cannot create a new transaction.");
    return false;
    }

  return true;
}
 
// ----------------------------------------------------------------------
bool vtkPostgreSQLQuery::CommitTransaction()
{
  if ( this->Transactor )
    {
    if ( this->Transactor->CommitTransaction() )
      {
      this->Transactor->Delete();
      this->Transactor = 0;
      this->Active = false;
      return true;
      }
    // don't delete the transactor on failure... we want the error message!
    return false;
    }
  vtkErrorMacro( "Cannot commit.  There is no transaction in progress.");
  return false;
}

// ----------------------------------------------------------------------
bool vtkPostgreSQLQuery::RollbackTransaction()
{
  if ( ! this->Transactor )
    {
    vtkErrorMacro("Cannot rollback.  There is no transaction in progress.");
    return false;
    }

  return this->Transactor->RollbackTransaction();
}

// ----------------------------------------------------------------------
void vtkPostgreSQLQuery::SetLastErrorText( const char* msg )
{
  if ( this->Transactor )
    {
    this->Transactor->SetLastErrorText( msg );
    }
}

