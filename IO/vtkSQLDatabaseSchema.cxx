/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkSQLDatabaseSchema.cxx

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

#include "vtkToolkits.h"
#include "vtkSQLDatabaseSchema.h"

#include "vtkObjectFactory.h"

#include <vtkstd/vector>

vtkCxxRevisionMacro(vtkSQLDatabaseSchema, "1.1");
vtkStandardNewMacro(vtkSQLDatabaseSchema);

class vtkSQLDatabaseSchemaInternals
{
public:
  struct Column
  {
    vtkSQLDatabaseSchema::DatabaseColumnType Type; // DCT: OK to use ColumnType enums instead of int here
    int Size; // used when required, ignored otherwise (e.g. varchar)
    vtkstd::string Name; // DCT: Note use of string instead of char* here to avoid leaks on destruction.
    vtkstd::string Attributes; // may have implementation-specific stuff
  };

  struct Index
  {
    vtkSQLDatabaseSchema::DatabaseIndexType Type;
    vtkstd::string Name;
    vtkstd::vector<vtkstd::string> ColumnNames;
  };

  struct Trigger
  {
    vtkSQLDatabaseSchema::DatabaseTriggerType Type;
    vtkstd::string Name;
    vtkstd::string Action; // may have implementation-specific stuff
  };

  struct Table
  {
    vtkstd::string Name;
    vtkstd::vector<vtkSQLDatabaseSchemaInternals::Column> Columns;
    vtkstd::vector<vtkSQLDatabaseSchemaInternals::Index> Indices;
    vtkstd::vector<vtkSQLDatabaseSchemaInternals::Trigger> Triggers;
  };

  vtkstd::vector<Table> Tables;
};

// ----------------------------------------------------------------------
vtkSQLDatabaseSchema::vtkSQLDatabaseSchema()
{
  this->Name = 0;
  this->Internals = new vtkSQLDatabaseSchemaInternals;
}

// ----------------------------------------------------------------------
vtkSQLDatabaseSchema::~vtkSQLDatabaseSchema()
{
  this->SetName( 0 );
  delete this->Internals;
}

// ----------------------------------------------------------------------
void vtkSQLDatabaseSchema::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  os << indent << "Name: " << this->Name << "\n";
  os << indent << "Internals: " << this->Internals << "\n";
  //os << indent << "Tables: " << this->GetNumberOfTables() << "entries.\n";
}

// ----------------------------------------------------------------------
int vtkSQLDatabaseSchema::AddTable( const char* tblName )
{
  vtkSQLDatabaseSchemaInternals::Table newTab;
  int tblHandle = this->Internals->Tables.size();
  newTab.Name = tblName;
  this->Internals->Tables.push_back( newTab );
  return tblHandle;
}

// ----------------------------------------------------------------------
int vtkSQLDatabaseSchema::AddColumnToIndex( int tblHandle, 
                                            int idxHandle, 
                                            int colHandle )
{
  if ( tblHandle < 0 || tblHandle >= this->GetNumberOfTables() )
    {
    vtkErrorMacro( "Can not add column to index of non-existent table " << tblHandle );
    return -1;
    }

  vtkSQLDatabaseSchemaInternals::Table& table( this->Internals->Tables[tblHandle] );
  if ( colHandle < 0 || colHandle >= static_cast<int>( table.Columns.size() ) )
    {
    vtkErrorMacro( "Can not add non-existent column " << colHandle << " in table " << tblHandle );
    return -1;
    }

  if ( idxHandle < 0 || idxHandle >= static_cast<int>( table.Indices.size() ) )
    {
    vtkErrorMacro( "Can not add column to non-existent index " << idxHandle << " of table " << tblHandle );
    return -1;
    }

  table.Indices[idxHandle].ColumnNames.push_back( table.Columns[colHandle].Name );
  return static_cast<int>( table.Indices[idxHandle].ColumnNames.size() - 1 );
}

// ----------------------------------------------------------------------
int vtkSQLDatabaseSchema::AddColumnToTable( int tblHandle,
                                            int colType, 
                                            const char* colName, 
                                            int colSize, 
                                            const char* colOpts )
{
  if ( tblHandle < 0 || tblHandle >= this->GetNumberOfTables() )
    {
    vtkErrorMacro( "Can not add column to non-existent table " << tblHandle );
    return -1;
    }

  // DCT: This trick avoids copying a Column structure the way push_back would:
  int colHandle = this->Internals->Tables[tblHandle].Columns.size();
  this->Internals->Tables[tblHandle].Columns.resize( colHandle + 1 );
  vtkSQLDatabaseSchemaInternals::Column& result( this->Internals->Tables[tblHandle].Columns[colHandle] );
  result.Type = static_cast<DatabaseColumnType>( colType );
  result.Size = colSize;
  result.Name = colName;
  result.Attributes = colOpts;
  return colHandle;
}

// ----------------------------------------------------------------------
int vtkSQLDatabaseSchema::AddIndexToTable( int tblHandle, 
                                           int idxType, 
                                           const char* idxName )
{
  if ( tblHandle < 0 || tblHandle >= this->GetNumberOfTables() )
    {
    vtkErrorMacro( "Can not add index to non-existent table " << tblHandle );
    return -1;
    }

  int idxHandle = this->Internals->Tables[tblHandle].Indices.size();
  this->Internals->Tables[tblHandle].Indices.resize( idxHandle + 1 );
  vtkSQLDatabaseSchemaInternals::Index& result( this->Internals->Tables[tblHandle].Indices[idxHandle] );
  result.Type = static_cast<DatabaseIndexType>( idxType );
  result.Name = idxName;
  return idxHandle;
}

// ----------------------------------------------------------------------
int vtkSQLDatabaseSchema::AddTriggerToTable( int tblHandle,
                                             int trgType, 
                                             const char* trgName, 
                                             const char* trgAction )
{
  if ( tblHandle < 0 || tblHandle >= this->GetNumberOfTables() )
    {
    vtkErrorMacro( "Can not add trigger to non-existent table " << tblHandle );
    return -1;
    }

  int trgHandle = this->Internals->Tables[tblHandle].Triggers.size();
  this->Internals->Tables[tblHandle].Triggers.resize( trgHandle + 1 );
  vtkSQLDatabaseSchemaInternals::Trigger& result( this->Internals->Tables[tblHandle].Triggers[trgHandle] );
  result.Type = static_cast<DatabaseTriggerType>( trgType );
  result.Name = trgName;
  result.Action = trgAction;
  return trgHandle;
}

// ----------------------------------------------------------------------
int vtkSQLDatabaseSchema::GetTableHandleFromName( const char* tblName )
{
  int i;
  int ntab = this->Internals->Tables.size();
  for ( i = 0; i < ntab; ++i )
    {
    if ( this->Internals->Tables[i].Name == tblName )
      {
      return i;
      }
    }
  return -1;
}


// ----------------------------------------------------------------------
int vtkSQLDatabaseSchema::GetIndexHandleFromName( const char* tblName, 
                                                  const char* idxName )
{
  int tblHandle = this->GetTableHandleFromName( tblName );
  if ( tblHandle < 0 )
    {
    return -1;
    }

  int i;
  int nidx = this->Internals->Tables[tblHandle].Indices.size();
  for ( i = 0; i < nidx ; ++ i )
    {
    if ( this->Internals->Tables[tblHandle].Indices[i].Name == idxName )
      {
      return i;
      }
    }
  return -1;
}

// ----------------------------------------------------------------------
int vtkSQLDatabaseSchema::GetColumnHandleFromName( const char* tblName, 
                                                   const char* colName )
{
  int tblHandle = this->GetTableHandleFromName( tblName );
  if ( tblHandle < 0 )
    {
    return -1;
    }

  int i;
  int ncol = this->Internals->Tables[tblHandle].Columns.size();
  for ( i = 0; i < ncol ; ++ i )
    {
    if ( this->Internals->Tables[tblHandle].Columns[i].Name == colName )
      {
      return i;
      }
    }
  return -1;
}

// ----------------------------------------------------------------------
int vtkSQLDatabaseSchema::GetTriggerHandleFromName( const char* tblName, 
                                                    const char* trgName )
{
  int tblHandle = this->GetTableHandleFromName( tblName );
  if ( tblHandle < 0 )
    {
    return -1;
    }

  int i;
  int ntrg = this->Internals->Tables[tblHandle].Triggers.size();
  for ( i = 0; i < ntrg ; ++ i )
    {
    if ( this->Internals->Tables[tblHandle].Triggers[i].Name == trgName )
      {
      return i;
      }
    }
  return -1;
}


// ----------------------------------------------------------------------
int vtkSQLDatabaseSchema::AddTableMultipleArguments( const char* tblName, ... )
{
  int tblHandle = this->AddTable( tblName );
  int token;
  int dtyp;
  int size;
  int curIndexHandle = -1;
  const char* name;
  const char* attr;

  va_list args;
  va_start( args, tblName );
  while ( ( token = va_arg( args, int ) ) != END_TABLE_TOKEN )
    {
    switch ( token )
      {
      case COLUMN_TOKEN:
        dtyp = va_arg( args, int );
        name = va_arg( args, const char* );
        size = va_arg( args, int );
        attr = va_arg( args, const char* );
        this->AddColumnToTable( tblHandle, dtyp, name, size, attr );
        curIndexHandle = -1; // Don't allow bad state.
        break;
      case INDEX_TOKEN:
        dtyp = va_arg( args, int );
        name = va_arg( args, const char* );
        curIndexHandle = this->AddIndexToTable( tblHandle, dtyp, name );
        while ( ( token = va_arg( args, int ) ) != END_INDEX_TOKEN )
          {
          name = va_arg( args, const char* );
          dtyp = this->GetColumnHandleFromName( tblName, name );
          this->AddColumnToIndex( tblHandle, curIndexHandle, dtyp );
          }
        break;
      case TRIGGER_TOKEN:
        dtyp = va_arg( args, int );
        name = va_arg( args, const char* );
        attr = va_arg( args, const char* );
        this->AddTriggerToTable( tblHandle, dtyp, name, attr );
        break;
      default:
        {
        vtkErrorMacro( "Bad token " << token << " passed to AddTable" );
        va_end( args );
        return -1;
        }
      }
    }
  return tblHandle;
}

// ----------------------------------------------------------------------
int vtkSQLDatabaseSchema::GetNumberOfTables()
{
  return this->Internals->Tables.size();
}
