#ifndef __vtkPostgreSQLDatabasePrivate_h
#define __vtkPostgreSQLDatabasePrivate_h

#include "vtkStdString.h"
#include "vtkType.h"
#include "vtkTimeStamp.h"

#define PQXX_ALLOW_LONG_LONG
#include <pqxx/pqxx>

// Manage a connection to a Postgres database for the vtkPostgreSQLDatabase class.
class vtkPostgreSQLDatabasePrivate
{
 public:
  // Description:
  // The constructor requires connection options (effectively a URL).
  vtkPostgreSQLDatabasePrivate( const char options[] ) 
    : Connection( options )
    {
    this->Work = 0;
    this->UpdateColumnTypeMap();
    }
    
    // Description:
    // Destroy the database connection. Any uncommitted transaction will be aborted.
    virtual ~vtkPostgreSQLDatabasePrivate()
      {
      this->RollbackTransaction(); // deletes Work
      }

    // Description:
    // Open a new transaction.
    // Any currently-open, uncommitted transaction will be aborted before
    // the new transaction is created.
    bool BeginTransaction()
    {
      this->RollbackTransaction(); // deletes Work
      this->Work = new pqxx::transaction<>( this->Connection );
      return true;
    }

    // Description:
    // Commit all the queries since the transaction began.
    // This will fail if no transaction has been created with BeginTransaction().
    bool CommitTransaction()
    {
      if ( this->Work )
        {
        bool ok = true;
        try
          {
          this->Work->commit();
          }
        catch ( const vtkstd::exception& e )
          {
          this->LastErrorText = e.what();
          vtkGenericWarningMacro( << this->LastErrorText.c_str() );
          ok = false;
          }
        delete this->Work;
        this->Work = 0;
        return ok;
        }
      // No transaction, no commit
      vtkGenericWarningMacro( "Cannot commit without first beginning a transaction." );
      return false;
    }

    // Description:
    // Abort the current transaction, if any.
    // It is not an error to abort a non-existent transaction.
    bool RollbackTransaction()
    {
      if ( this->Work )
        {
        // Deletion causes transaction abortion
        delete this->Work;
        this->Work = 0;
        return true;
        }
      //vtkErrorMacro( "Cannot roll back a transaction without creating one first." );
      return false;
    }

    // Given a Postgres column type OID, return a VTK array type (see vtkType.h).
    int GetVTKTypeFromOID( pqxx::oid pgtype )
    {
      vtkstd::map<pqxx::oid,int>::const_iterator it = this->ColumnTypeMap.find( pgtype );
      if ( it == this->ColumnTypeMap.end() )
        {
        return VTK_STRING;
        }
      return it->second;
    }

    // Description:
    // Create or refresh the map from Postgres column types to VTK array types.
    //
    // Postgres defines a table for types so that users may define types.
    // This adaptor does not support user-defined types or even all of the
    // default types defined by Postgres (some are inherently difficult to
    // translate into VTK since Postgres allows columns to have composite types,
    // vector-valued types, and extended precision types that vtkVariant does
    // not support.
    //
    // This routine examines the pg_types table to get a map from Postgres column
    // type IDs (stored as OIDs) to VTK array types. It is called whenever a new
    // database connection is initiated.
    void UpdateColumnTypeMap()
    {
      this->ColumnTypeMap.clear();
      this->BeginTransaction();
      try
        {
        pqxx::result res = this->Work->exec( "SELECT oid,typname,typlen FROM pg_type" );
        pqxx::result::const_iterator rit = res.begin();
        pqxx::oid oid;
        vtkstd::string typname;
        int typlen;
        for ( ; rit != res.end(); ++rit )
          {
          rit[0].to( oid );
          rit[1].to( typname );
          rit[2].to( typlen );
          if ( typname == "int8" || ( typname == "oid" && typlen == 8 ) )
            {
            this->ColumnTypeMap[ oid ] = VTK_TYPE_INT64;
            }
          else if ( typname == "int4" || ( typname == "oid" && typlen == 4 ) )
            {
            this->ColumnTypeMap[ oid ] = VTK_TYPE_INT32;
            }
          else if ( typname == "int2" )
            {
            this->ColumnTypeMap[ oid ] = VTK_TYPE_INT16;
            }
          else if ( typname == "char" )
            {
            this->ColumnTypeMap[ oid ] = VTK_TYPE_INT8;
            }
          else if ( typname == "time_stamp" )
            {
            this->ColumnTypeMap[ oid ] = VTK_TYPE_INT64;
            }
          else if ( typname == "float4" )
            {
            this->ColumnTypeMap[ oid ] = VTK_FLOAT;
            }
          else if ( typname == "float8" )
            {
            this->ColumnTypeMap[ oid ] = VTK_DOUBLE;
            }
          else if ( typname == "abstime" || typname == "reltime" )
            {
            this->ColumnTypeMap[ oid ] = ( typlen == 4 ? VTK_TYPE_INT32 : VTK_TYPE_INT64 );
            }
          else if ( typname == "text" )
            {
            this->ColumnTypeMap[ oid ] = VTK_STRING;
            }
          }
        }
      catch ( vtkstd::exception& e )
        {
        this->LastErrorText = e.what();
        }
      this->RollbackTransaction();
    }

    // An error message. This will be null if no error has occurred. It is set after each transaction attempt.
    vtkStdString LastErrorText;
    // A database connection.
    pqxx::connection Connection;
    // A transaction.
    // This is only non-null after BeginTransaction() and before either
    // CommitTransaction() or RollbackTransaction() have been called.
    pqxx::work* Work;
    // Map Postgres column types to VTK types.
    vtkstd::map<pqxx::oid,int> ColumnTypeMap;
};

#endif // __vtkPostgreSQLDatabasePrivate_h
