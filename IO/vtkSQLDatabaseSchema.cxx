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
#include "vtkStdString.h"

#include <vtkstd/vector>

vtkCxxRevisionMacro(vtkSQLDatabaseSchema, "1.10");
vtkStandardNewMacro(vtkSQLDatabaseSchema);

class vtkSQLDatabaseSchemaInternals
{
public:
  struct Column
  {
    vtkSQLDatabaseSchema::DatabaseColumnType Type;
    int Size; // used when required, ignored otherwise (e.g. varchar)
    vtkStdString Name; // DCT: Note use of string instead of char* here to avoid leaks on destruction.
    vtkStdString Attributes; // may have implementation-specific stuff
  };

  struct Index
  {
    vtkSQLDatabaseSchema::DatabaseIndexType Type;
    vtkStdString Name;
    vtkstd::vector<vtkStdString> ColumnNames;
  };

  struct Trigger
  {
    vtkSQLDatabaseSchema::DatabaseTriggerType Type;
    vtkStdString Name;
    vtkStdString Action; // may have implementation-specific stuff
  };

  struct Table
  {
    vtkStdString Name;
    vtkstd::vector<Column> Columns;
    vtkstd::vector<Index> Indices;
    vtkstd::vector<Trigger> Triggers;
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
    vtkErrorMacro( "Cannot add column to index of non-existent table " << tblHandle );
    return -1;
    }

  vtkSQLDatabaseSchemaInternals::Table* table = &this->Internals->Tables[tblHandle];
  if ( colHandle < 0 || colHandle >= static_cast<int>( table->Columns.size() ) )
    {
    vtkErrorMacro( "Cannot add non-existent column " << colHandle << " in table " << tblHandle );
    return -1;
    }

  if ( idxHandle < 0 || idxHandle >= static_cast<int>( table->Indices.size() ) )
    {
    vtkErrorMacro( "Cannot add column to non-existent index " << idxHandle << " of table " << tblHandle );
    return -1;
    }

  table->Indices[idxHandle].ColumnNames.push_back( table->Columns[colHandle].Name );
  return static_cast<int>( table->Indices[idxHandle].ColumnNames.size() - 1 );
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
    vtkErrorMacro( "Cannot add column to non-existent table " << tblHandle );
    return -1;
    }

  // DCT: This trick avoids copying a Column structure the way push_back would:
  int colHandle = this->Internals->Tables[tblHandle].Columns.size();
  this->Internals->Tables[tblHandle].Columns.resize( colHandle + 1 );
  vtkSQLDatabaseSchemaInternals::Column* column = &this->Internals->Tables[tblHandle].Columns[colHandle];
  column->Type = static_cast<DatabaseColumnType>( colType );
  column->Size = colSize;
  column->Name = colName;
  column->Attributes = colOpts;
  return colHandle;
}

// ----------------------------------------------------------------------
int vtkSQLDatabaseSchema::AddIndexToTable( int tblHandle, 
                                           int idxType, 
                                           const char* idxName )
{
  if ( tblHandle < 0 || tblHandle >= this->GetNumberOfTables() )
    {
    vtkErrorMacro( "Cannot add index to non-existent table " << tblHandle );
    return -1;
    }

  int idxHandle = this->Internals->Tables[tblHandle].Indices.size();
  this->Internals->Tables[tblHandle].Indices.resize( idxHandle + 1 );
  vtkSQLDatabaseSchemaInternals::Index* index = &this->Internals->Tables[tblHandle].Indices[idxHandle];
  index->Type = static_cast<DatabaseIndexType>( idxType );
  index->Name = idxName;
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
    vtkErrorMacro( "Cannot add trigger to non-existent table " << tblHandle );
    return -1;
    }

  int trgHandle = this->Internals->Tables[tblHandle].Triggers.size();
  this->Internals->Tables[tblHandle].Triggers.resize( trgHandle + 1 );
  vtkSQLDatabaseSchemaInternals::Trigger* trigger = &this->Internals->Tables[tblHandle].Triggers[trgHandle];
  trigger->Type = static_cast<DatabaseTriggerType>( trgType );
  trigger->Name = trgName;
  trigger->Action = trgAction;
  return trgHandle;
}

// ----------------------------------------------------------------------
int vtkSQLDatabaseSchema::GetTableHandleFromName( const char* tblName )
{
  int i;
  int ntab = this->Internals->Tables.size();
  vtkStdString tblNameStr( tblName );
  for ( i = 0; i < ntab; ++i )
    {
    if ( this->Internals->Tables[i].Name == tblNameStr )
      {
      return i;
      }
    }
  return -1;
}

// ----------------------------------------------------------------------
const char* vtkSQLDatabaseSchema::GetTableNameFromHandle( int tblHandle )
{
  if ( tblHandle < 0 || tblHandle >= this->GetNumberOfTables() )
    {
    vtkErrorMacro( "Cannot get name of non-existent table " << tblHandle );
    return 0;
    }
  
  return this->Internals->Tables[tblHandle].Name;
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
  vtkStdString idxNameStr( idxName );
  for ( i = 0; i < nidx ; ++ i )
    {
    if ( this->Internals->Tables[tblHandle].Indices[i].Name == idxNameStr )
      {
      return i;
      }
    }
  return -1;
}

// ----------------------------------------------------------------------
const char* vtkSQLDatabaseSchema::GetIndexNameFromHandle( int tblHandle, 
                                                          int idxHandle )
{
  if ( tblHandle < 0 || tblHandle >= this->GetNumberOfTables() )
    {
    vtkErrorMacro( "Cannot get name of an index in non-existent table " << tblHandle );
    return 0;
    }
  
  if ( idxHandle < 0 || idxHandle >= static_cast<int>( this->Internals->Tables[tblHandle].Indices.size() ) )
    {
    vtkErrorMacro( "Cannot get name of non-existent index " << idxHandle << " in table " << tblHandle );
    return 0;
    }
  
  return this->Internals->Tables[tblHandle].Indices[idxHandle].Name;
}

// ----------------------------------------------------------------------
int vtkSQLDatabaseSchema::GetIndexTypeFromHandle( int tblHandle, 
                                                  int idxHandle )
{
  if ( tblHandle < 0 || tblHandle >= this->GetNumberOfTables() )
    {
    vtkErrorMacro( "Cannot get type of an index in non-existent table " << tblHandle );
    return -1;
    }
  
  if ( idxHandle < 0 || idxHandle >= static_cast<int>( this->Internals->Tables[tblHandle].Indices.size() ) )
    {
    vtkErrorMacro( "Cannot get type of non-existent index " << idxHandle << " in table " << tblHandle );
    return -1;
    }
  
  return static_cast<int>( this->Internals->Tables[tblHandle].Indices[idxHandle].Type );
}

// ----------------------------------------------------------------------
const char* vtkSQLDatabaseSchema::GetIndexColumnNameFromHandle( int tblHandle, 
                                                                int idxHandle,
                                                                int cnmHandle )
{
  if ( tblHandle < 0 || tblHandle >= this->GetNumberOfTables() )
    {
    vtkErrorMacro( "Cannot get column name of an index in non-existent table " << tblHandle );
    return 0;
    }
  
  if ( idxHandle < 0 || idxHandle >= static_cast<int>( this->Internals->Tables[tblHandle].Indices.size() ) )
    {
    vtkErrorMacro( "Cannot get column name of non-existent index " << idxHandle << " in table " << tblHandle );
    return 0;
    }
  
  if ( cnmHandle < 0 || cnmHandle >= static_cast<int>( this->Internals->Tables[tblHandle].Indices[idxHandle].ColumnNames.size() ) )
    {
    vtkErrorMacro( "Cannot get column name of non-existent column " << cnmHandle << " of index " << idxHandle << " in table " << tblHandle );
    return 0;
    }

  return this->Internals->Tables[tblHandle].Indices[idxHandle].ColumnNames[cnmHandle];
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
  vtkStdString colNameStr( colName );
  for ( i = 0; i < ncol ; ++ i )
    {
    if ( this->Internals->Tables[tblHandle].Columns[i].Name == colNameStr )
      {
      return i;
      }
    }
  return -1;
}

// ----------------------------------------------------------------------
const char* vtkSQLDatabaseSchema::GetColumnNameFromHandle( int tblHandle, 
                                                           int colHandle )
{
  if ( tblHandle < 0 || tblHandle >= this->GetNumberOfTables() )
    {
    vtkErrorMacro( "Cannot get name of a column in non-existent table " << tblHandle );
    return 0;
    }
  
  if ( colHandle < 0 || colHandle >= static_cast<int>( this->Internals->Tables[tblHandle].Columns.size() ) )
    {
    vtkErrorMacro( "Cannot get name of non-existent column " << colHandle << " in table " << tblHandle );
    return 0;
    }
  
  return this->Internals->Tables[tblHandle].Columns[colHandle].Name;
}

// ----------------------------------------------------------------------
int vtkSQLDatabaseSchema::GetColumnTypeFromHandle( int tblHandle, 
                                                   int colHandle )
{
  if ( tblHandle < 0 || tblHandle >= this->GetNumberOfTables() )
    {
    vtkErrorMacro( "Cannot get type of a column in non-existent table " << tblHandle );
    return -1;
    }
  
  if ( colHandle < 0 || colHandle >= static_cast<int>( this->Internals->Tables[tblHandle].Columns.size() ) )
    {
    vtkErrorMacro( "Cannot get type of non-existent column " << colHandle << " in table " << tblHandle );
    return -1;
    }
  
  return static_cast<int>( this->Internals->Tables[tblHandle].Columns[colHandle].Type );
}

// ----------------------------------------------------------------------
const char* vtkSQLDatabaseSchema::GetColumnAttributesFromHandle( int tblHandle,  
                                                                 int colHandle )
{
  if ( tblHandle < 0 || tblHandle >= this->GetNumberOfTables() )
    {
    vtkErrorMacro( "Cannot get attributes of a column in non-existent table " << tblHandle );
    return 0;
    }
  
  if ( colHandle < 0 || colHandle >= static_cast<int>( this->Internals->Tables[tblHandle].Columns.size() ) )
    {
    vtkErrorMacro( "Cannot get attributes of non-existent column " << colHandle << " in table " << tblHandle );
    return 0;
    }
  
  return this->Internals->Tables[tblHandle].Columns[colHandle].Attributes;
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
  vtkStdString trgNameStr( trgName );
  for ( i = 0; i < ntrg ; ++ i )
    {
    if ( this->Internals->Tables[tblHandle].Triggers[i].Name == trgNameStr )
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

#if !defined(__sgi) || defined(__GNUC__)
  va_list args;
#else // SGI CC
  vtkstd::va_list args;
#endif // SGI CC
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
void vtkSQLDatabaseSchema::Reset()
{
  this->Internals->Tables.clear();
}

// ----------------------------------------------------------------------
int vtkSQLDatabaseSchema::GetNumberOfTables()
{
  return this->Internals->Tables.size();
}

// ----------------------------------------------------------------------
int vtkSQLDatabaseSchema::GetNumberOfColumnsInTable( int tblHandle )
{
  if ( tblHandle < 0 || tblHandle >= this->GetNumberOfTables() )
    {
    vtkErrorMacro( "Cannot get the number of columns of non-existent table " << tblHandle );
    return -1;
    }

  return this->Internals->Tables[tblHandle].Columns.size();
}

// ----------------------------------------------------------------------
int vtkSQLDatabaseSchema::GetNumberOfIndicesInTable( int tblHandle )
{
  if ( tblHandle < 0 || tblHandle >= this->GetNumberOfTables() )
    {
    vtkErrorMacro( "Cannot get the number of indices of non-existent table " << tblHandle );
    return -1;
    }

  return this->Internals->Tables[tblHandle].Indices.size();
}

// ----------------------------------------------------------------------
int vtkSQLDatabaseSchema:: GetNumberOfColumnNamesInIndex( int tblHandle, int idxHandle )
{
  if ( tblHandle < 0 || tblHandle >= this->GetNumberOfTables() )
    {
    vtkErrorMacro( "Cannot get the number of column names in index of non-existent table " << tblHandle );
    return -1;
    }

  if ( idxHandle < 0 || idxHandle >= static_cast<int>( this->Internals->Tables[tblHandle].Indices.size() ) )
    {
    vtkErrorMacro( "Cannot get the number of column names of non-existent index " << idxHandle << " in table " << tblHandle );
    return -1;
    }

  return this->Internals->Tables[tblHandle].Indices[idxHandle].ColumnNames.size();
}

// ----------------------------------------------------------------------
int vtkSQLDatabaseSchema::GetNumberOfTriggersInTable( int tblHandle )
{
  if ( tblHandle < 0 || tblHandle >= this->GetNumberOfTables() )
    {
    vtkErrorMacro( "Cannot get the number of triggers of non-existent table " << tblHandle );
    return -1;
    }

  return this->Internals->Tables[tblHandle].Triggers.size();
}
