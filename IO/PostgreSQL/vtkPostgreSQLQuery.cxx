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
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
#include "vtkPostgreSQLQuery.h"

#include "vtkObjectFactory.h"
#include "vtkPostgreSQLDatabase.h"
#include "vtkStringArray.h"
#include "vtkVariant.h"
#include "vtkVariantArray.h"
#include "vtkPostgreSQLDatabasePrivate.h"

#include <cassert>
#include <limits> // man, I hope all platforms have this nowadays

#include <vtksys/ios/sstream>

#define BEGIN_TRANSACTION "BEGIN"
#define COMMIT_TRANSACTION "COMMIT"
#define ROLLBACK_TRANSACTION "ROLLBACK"

#define DECLARE_CONVERTER(TargetType) vtkVariant ConvertStringTo##TargetType(bool, const char *);

DECLARE_CONVERTER(Boolean);
DECLARE_CONVERTER(SignedChar);
DECLARE_CONVERTER(UnsignedChar);
DECLARE_CONVERTER(SignedShort);
DECLARE_CONVERTER(UnsignedShort);
DECLARE_CONVERTER(SignedInt);
DECLARE_CONVERTER(UnsignedInt);
DECLARE_CONVERTER(SignedLong);
DECLARE_CONVERTER(UnsignedLong);
DECLARE_CONVERTER(Float);
DECLARE_CONVERTER(Double);
DECLARE_CONVERTER(VtkIdType);
DECLARE_CONVERTER(String);
#ifdef VTK_TYPE_USE_LONG_LONG
DECLARE_CONVERTER(SignedLongLong);
DECLARE_CONVERTER(UnsignedLongLong);
#endif

template<typename T>
void ConvertFromNetworkOrder(T &target, const char *rawBytes)
{
  for (unsigned int i = 0; i < sizeof(T); ++i)
    {
    int targetByte = sizeof(T) - (i+1);
    target |= (rawBytes[i] << (8*targetByte));
    }
}

// ----------------------------------------------------------------------

vtkStandardNewMacro(vtkPostgreSQLQuery);

// ----------------------------------------------------------------------

class vtkPostgreSQLQueryPrivate
{
public:
  vtkPostgreSQLQueryPrivate()
    {
      this->QueryResults = NULL;
      this->CurrentRow = -1;
    }
  ~vtkPostgreSQLQueryPrivate()
    {
      if (this->QueryResults)
        {
        PQclear(this->QueryResults);
        }
    }

  PGresult *QueryResults;
  int CurrentRow;
};

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
  else if (this->QueryInternals->CurrentRow < 0)
    {
    vtkWarningMacro( "DataValue() cannot be called before advancing to the first row with NextRow()." );
    return vtkVariant();
    }

  // Since null is independent of data type, check that next
  if (PQgetisnull(this->QueryInternals->QueryResults,
                  this->QueryInternals->CurrentRow,
                  column))
    {
    return vtkVariant();
    }

  int colType = this->GetFieldType( column );
  bool isBinary = this->IsColumnBinary(column);
  const char *rawData = this->GetColumnRawData(column);
  switch ( colType )
    {
    case VTK_VOID:
      return vtkVariant();
    case VTK_BIT:
    {
    return ConvertStringToBoolean(isBinary, rawData);
    }
    case VTK_CHAR:
    case VTK_SIGNED_CHAR:
    {
    return ConvertStringToSignedChar(isBinary, rawData);
    }
    case VTK_UNSIGNED_CHAR:
    {
    return ConvertStringToUnsignedChar(isBinary, rawData);
    }
    case VTK_SHORT:
    {
    return ConvertStringToSignedShort(isBinary, rawData);
    }
    case VTK_UNSIGNED_SHORT:
    {
    return ConvertStringToUnsignedShort(isBinary, rawData);
    }
    case VTK_INT:
    {
    return ConvertStringToSignedInt(isBinary, rawData);
    }
    case VTK_UNSIGNED_INT:
    {
    return ConvertStringToUnsignedInt(isBinary, rawData);
    }
    case VTK_LONG:
    {
    return ConvertStringToSignedLong(isBinary, rawData);
    }
    case VTK_UNSIGNED_LONG:
    {
    return ConvertStringToUnsignedLong(isBinary, rawData);
    }
#ifdef VTK_TYPE_USE_LONG_LONG
    case VTK_LONG_LONG:
    {
    return ConvertStringToSignedLongLong(isBinary, rawData);
    }
    case VTK_UNSIGNED_LONG_LONG:
    {
    return ConvertStringToUnsignedLongLong(isBinary, rawData);
    }
#endif // VTK_TYPE_USE_LONG_LONG
    case VTK_FLOAT:
    {
    return ConvertStringToFloat(isBinary, rawData);
    }
    case VTK_DOUBLE:
    {
    return ConvertStringToDouble(isBinary, rawData);
    }
    case VTK_ID_TYPE:
    {
    return ConvertStringToVtkIdType(isBinary, rawData);
    }
    case VTK_STRING:
    {
    return vtkVariant(rawData);
    }
    default:
    {
    return vtkVariant();
    }
    } // end of switch on column type
} // end of DataValue(int column)


// ----------------------------------------------------------------------
vtkPostgreSQLQuery::vtkPostgreSQLQuery()
{
  this->TransactionInProgress = false;
  this->LastErrorText = NULL;
  this->QueryInternals = NULL;
}

// ----------------------------------------------------------------------
vtkPostgreSQLQuery::~vtkPostgreSQLQuery()
{
  this->SetDatabase(NULL);
  this->SetLastErrorText(NULL);
  delete this->QueryInternals;
}

// ----------------------------------------------------------------------
void vtkPostgreSQLQuery::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Transaction in progress: "
     << (this->TransactionInProgress ? "YES" : "NO") << "\n";
  os << indent << "Last error message: "
     << (this->LastErrorText ? this->LastErrorText : "(null)")
     << "\n";
  os << indent << "Internals: ";
  if (this->QueryInternals)
    {
    os << this->QueryInternals;
    }
  else
    {
    os << "(null)";
    }
  os << "\n";
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

  // If a query is already in progress clear out its results so we can
  // begin anew.
  if (this->QueryInternals)
    {
    this->DeleteQueryResults();
    }


  if (!db->IsOpen())
    {
    this->SetLastErrorText("Cannot execute query.  Database connection is closed.");
    vtkErrorMacro(<<"Cannot execute query.  Database connection is closed.");
    this->Active = false;
    return false;
    }

  this->QueryInternals = new vtkPostgreSQLQueryPrivate;
  this->QueryInternals->QueryResults = PQexec(db->Connection->Connection,
                                              this->Query);

  bool returnStatus;
  switch (PQresultStatus(this->QueryInternals->QueryResults))
    {
    case PGRES_EMPTY_QUERY:
    {
    returnStatus = true;
    this->Active = false;
    this->DeleteQueryResults();
    vtkWarningMacro(<<"Query string was set but empty.");
    this->SetLastErrorText(0);
    }; break;

    case PGRES_COMMAND_OK: // success on a command returning no data
    {
    returnStatus = true;
    this->Active = true;
    this->DeleteQueryResults();
    this->SetLastErrorText(0);
    }; break;

    case PGRES_TUPLES_OK:
    {
    returnStatus = true;
    this->Active = true;
    this->SetLastErrorText(0);
    }; break;

    case PGRES_BAD_RESPONSE:
    {
    returnStatus = false;
    this->Active = false;
    this->DeleteQueryResults();
    this->SetLastErrorText("Incomprehensible server response");
    }; break;

    case PGRES_FATAL_ERROR:
    {
    returnStatus = false;
    this->Active = false;
    this->SetLastErrorText(PQerrorMessage(db->Connection->Connection));
    vtkErrorMacro(<<"Fatal error during query: "
                  << this->GetLastErrorText());
    this->DeleteQueryResults();
    }; break;

    default:
    {
    returnStatus = false;
    this->Active = false;
    vtksys_ios::ostringstream sbuf;
    sbuf << "Unhandled server response: ";
    sbuf << PQresStatus(PQresultStatus(this->QueryInternals->QueryResults));
    this->SetLastErrorText(sbuf.str().c_str());
    vtkErrorMacro(<< "Unhandled server response: "
                  << this->GetLastErrorText());
    this->DeleteQueryResults();
    }; break;
    }

  return returnStatus;
}

// ----------------------------------------------------------------------
int vtkPostgreSQLQuery::GetNumberOfFields()
{
  if ( ! this->Active || ! this->QueryInternals )
    {
    vtkErrorMacro( "Query is not active!" );
    return 0;
    }

  return PQnfields(this->QueryInternals->QueryResults);
}

// ----------------------------------------------------------------------
const char* vtkPostgreSQLQuery::GetFieldName( int column )
{
  if ( ! this->Active || ! this->QueryInternals->QueryResults )
    {
    vtkErrorMacro( "Query is not active!" );
    return 0;
    }
  else if ( column < 0 || column >= this->GetNumberOfFields() )
    {
    vtkErrorMacro( "Illegal field index " << column );
    return 0;
    }
  return PQfname(this->QueryInternals->QueryResults, column);
}

// ----------------------------------------------------------------------
int vtkPostgreSQLQuery::GetFieldType( int column )
{
  if ( ! this->Active || ! this->QueryInternals )
    {
    vtkErrorMacro( "Query is not active!" );
    return -1;
    }
  else if ( column < 0 || column >= this->GetNumberOfFields() )
    {
    vtkErrorMacro( "Illegal field index " << column );
    return -1;
    }

  vtkPostgreSQLDatabase *db =
    vtkPostgreSQLDatabase::SafeDownCast(this->Database);
  if (!db)
    {
    vtkErrorMacro(<<"No database!  How did this happen?");
    return -1;
    }
  return db->Connection->GetVTKTypeFromOID(PQftype(this->QueryInternals->QueryResults,
                                                  column));
}

// ----------------------------------------------------------------------
bool vtkPostgreSQLQuery::NextRow()
{
  if ( ! this->IsActive() || ! this->QueryInternals )
    {
    vtkErrorMacro( "Query is not active!" );
    return false;
    }

  if (this->QueryInternals->CurrentRow < (this->GetNumberOfRows() - 1))
    {
    ++ this->QueryInternals->CurrentRow;
    return true;
    }
  else
    {
    return false;
    }
}

// ----------------------------------------------------------------------
const char * vtkPostgreSQLQuery::GetLastErrorText()
{
  if ( ! this->Database )
    {
    return "No database";
    }
  return this->LastErrorText;
}

// ----------------------------------------------------------------------

vtkStdString vtkPostgreSQLQuery::EscapeString( vtkStdString s, bool addSurroundingQuotes )
{
  vtkStdString retval;
  if ( addSurroundingQuotes )
    {
    retval = "'";
    }

  vtkPostgreSQLDatabase* db = static_cast<vtkPostgreSQLDatabase*>( this->Database );

  if ( db && db->Connection )
    {
    char *escaped = new char[2 * s.size() + 1];
    int error;
    PQescapeStringConn(db->Connection->Connection,
                       escaped,
                       s.c_str(),
                       s.size(),
                       &error);
    retval.append(escaped);
    delete [] escaped;
    if (error)
      {
      vtkErrorMacro(<<"Error while escaping string.  Expect the result to be unusable.");
      }
    }
  else
    {
    retval.append( this->Superclass::EscapeString( s, false ) );
    }

  if ( addSurroundingQuotes )
    {
    retval += "'";
    }

  return retval;
}

// ----------------------------------------------------------------------
bool vtkPostgreSQLQuery::HasError()
{
  if ( ! this->Database )
    {
    return false;
    }
  return this->LastErrorText != 0;
}

// ----------------------------------------------------------------------
bool vtkPostgreSQLQuery::BeginTransaction()
{
  if (this->TransactionInProgress)
    {
    vtkErrorMacro(<<"Cannot start a transaction.  One is already in progress.");
    return false;
    }

  vtkPostgreSQLDatabase* db = vtkPostgreSQLDatabase::SafeDownCast( this->Database );
  assert( db );
  bool status;
  PGresult *result = PQexec(db->Connection->Connection, BEGIN_TRANSACTION);
  switch (PQresultStatus(result))
    {
    case PGRES_COMMAND_OK:
    {
    this->SetLastErrorText(0);
    this->TransactionInProgress = true;
    status = true;
    }; break;
    case PGRES_FATAL_ERROR:
    {
    this->SetLastErrorText(PQresultErrorMessage(result));
    vtkErrorMacro(<< "Error in BeginTransaction: "
                  << this->GetLastErrorText());
    status = false;
    }; break;
    default:
    {
    this->SetLastErrorText(PQresultErrorMessage(result));
    vtkWarningMacro(<<"Unexpected return code "
                    << PQresultStatus(result) << " ("
                    << PQresStatus(PQresultStatus(result))
                    << ") with error message "
                    << (this->LastErrorText ? this->LastErrorText
                        : "(null)"));
    status = false;
    }; break;
    }
  PQclear(result);
  return status;
}

// ----------------------------------------------------------------------
bool vtkPostgreSQLQuery::CommitTransaction()
{
  if (!this->TransactionInProgress)
    {
    vtkErrorMacro(<<"Cannot commit: no transaction is in progress.");
    return false;
    }

  vtkPostgreSQLDatabase *db = vtkPostgreSQLDatabase::SafeDownCast(this->Database);
  assert(db);

  PGresult *result = PQexec(db->Connection->Connection, COMMIT_TRANSACTION);
  bool status;
  switch (PQresultStatus(result))
    {
    case PGRES_COMMAND_OK:
    {
    this->SetLastErrorText(0);
    this->TransactionInProgress = false;
    status = true;
    }; break;
    case PGRES_FATAL_ERROR:
    {
    this->SetLastErrorText(PQresultErrorMessage(result));
    vtkErrorMacro(<< "Error in CommitTransaction: "
                  << this->GetLastErrorText());
    this->TransactionInProgress = false;
    status = false;
    }; break;
    default:
    {
    this->SetLastErrorText(PQresultErrorMessage(result));
    vtkWarningMacro(<<"Unexpected return code "
                    << PQresultStatus(result) << " ("
                    << PQresStatus(PQresultStatus(result))
                    << ") with error message "
                    << (this->LastErrorText ? this->LastErrorText
                        : "(null)"));
    this->TransactionInProgress = false;
    status = false;
    }; break;
    }
  PQclear(result);
  return status;
}

// ----------------------------------------------------------------------
bool vtkPostgreSQLQuery::RollbackTransaction()
{
  if (!this->TransactionInProgress)
    {
    vtkErrorMacro(<<"Cannot rollback: no transaction is in progress.");
    return false;
    }

  vtkPostgreSQLDatabase *db = vtkPostgreSQLDatabase::SafeDownCast(this->Database);
  assert(db);

  PGresult *result = PQexec(db->Connection->Connection, ROLLBACK_TRANSACTION);
  bool status;
  switch (PQresultStatus(result))
    {
    case PGRES_COMMAND_OK:
    {
    this->SetLastErrorText(0);
    this->TransactionInProgress = false;
    status = true;
    }; break;
    case PGRES_FATAL_ERROR:
    {
    this->SetLastErrorText(PQresultErrorMessage(result));
    vtkErrorMacro(<< "Error in RollbackTransaction: "
                  << this->GetLastErrorText());
    this->TransactionInProgress = false;
    status = false;
    }; break;
    default:
    {
    this->SetLastErrorText(PQresultErrorMessage(result));
    vtkWarningMacro(<<"Unexpected return code "
                    << PQresultStatus(result) << " ("
                    << PQresStatus(PQresultStatus(result))
                    << ") with error message "
                    << (this->LastErrorText ? this->LastErrorText
                        : "(null)"));
    this->TransactionInProgress = false;
    status = false;
    }; break;
    }
  PQclear(result);
  return status;
}

// ----------------------------------------------------------------------

void
vtkPostgreSQLQuery::DeleteQueryResults()
{
  this->Active = false;
  if (this->QueryInternals)
    {
    delete this->QueryInternals;
    this->QueryInternals = NULL;
    }
}

// ----------------------------------------------------------------------

vtkVariant ConvertStringToBoolean(bool, const char *rawData)
{
  // Since there are only a few possibilities I'm going to check
  // them all by hand.
  switch (rawData[0])
    {
    case 'T':
    case 't':
    case 'Y':
    case 'y':
    case '1':
    case 1:
    {
    return vtkVariant(true);
    }; break;

    case 'F':
    case 'f':
    case 'N':
    case 'n':
    case '0':
    case 0:
    {
    return vtkVariant(false);
    }; break;

    default:
    {
    vtkGenericWarningMacro(<<"Unable to convert raw data to boolean.  Data length is "
                           << strlen(rawData) << " and string is '"
                           << rawData << "'");
    return vtkVariant();
    }; break;
    }
}

// ----------------------------------------------------------------------

vtkVariant ConvertStringToSignedChar(bool isBinary, const char *rawData)
{
  if (isBinary)
    {
    return vtkVariant(rawData[0]);
    }
  else
    {
    vtkVariant converter(rawData);
    return vtkVariant(converter.ToChar());
    }
}

// ----------------------------------------------------------------------

vtkVariant ConvertStringToUnsignedChar(bool isBinary, const char *rawData)
{
  if (isBinary)
    {
    return vtkVariant(static_cast<unsigned char>(rawData[0]));
    }
  else
    {
    vtkVariant converter(rawData);
    return vtkVariant(converter.ToUnsignedChar());
    }
}

// ----------------------------------------------------------------------

vtkVariant ConvertStringToSignedShort(bool isBinary, const char *rawData)
{
  if (isBinary)
    {
    short result = 0;
    ConvertFromNetworkOrder(result, rawData);
    return vtkVariant(result);
    }
  else
    {
    vtkVariant converter(rawData);
    return vtkVariant(converter.ToShort());
    }
}

// ----------------------------------------------------------------------

vtkVariant ConvertStringToUnsignedShort(bool isBinary, const char *rawData)
{
  if (isBinary)
    {
    unsigned short result = 0;
    ConvertFromNetworkOrder(result, rawData);
    return vtkVariant(result);
    }
  else
    {
    vtkVariant converter(rawData);
    return vtkVariant(converter.ToUnsignedShort());
    }
}

// ----------------------------------------------------------------------

vtkVariant ConvertStringToSignedInt(bool isBinary, const char *rawData)
{
  if (isBinary)
    {
    int result = 0;
    ConvertFromNetworkOrder(result, rawData);
    return vtkVariant(result);
    }
  else
    {
    vtkVariant converter(rawData);
    return vtkVariant(converter.ToInt());
    }
}

// ----------------------------------------------------------------------

vtkVariant ConvertStringToUnsignedInt(bool isBinary, const char *rawData)
{
  if (isBinary)
    {
    unsigned int result = 0;
    ConvertFromNetworkOrder(result, rawData);
    return vtkVariant(result);
    }
  else
    {
    vtkVariant converter(rawData);
    return vtkVariant(converter.ToUnsignedInt());
    }
}

// ----------------------------------------------------------------------


vtkVariant ConvertStringToSignedLong(bool isBinary, const char *rawData)
{
  if (isBinary)
    {
    signed long result = 0;
    ConvertFromNetworkOrder(result, rawData);
    return vtkVariant(result);
    }
  else
    {
    vtkVariant converter(rawData);
    return vtkVariant(converter.ToLong());
    }
}

// ----------------------------------------------------------------------

vtkVariant ConvertStringToUnsignedLong(bool isBinary, const char *rawData)
{
  if (isBinary)
    {
    unsigned long result = 0;
    ConvertFromNetworkOrder(result, rawData);
    return vtkVariant(result);
    }
  else
    {
    vtkVariant converter(rawData);
    return vtkVariant(converter.ToLong());
    }
}

// ----------------------------------------------------------------------

#ifdef VTK_TYPE_USE_LONG_LONG

vtkVariant ConvertStringToSignedLongLong(bool isBinary, const char *rawData)
{
  if (isBinary)
    {
    long long result = 0;
    ConvertFromNetworkOrder(result, rawData);
    return vtkVariant(result);
    }
  else
    {
    vtkVariant converter(rawData);
    return vtkVariant(converter.ToLongLong());
    }
}

// ----------------------------------------------------------------------

vtkVariant ConvertStringToUnsignedLongLong(bool isBinary, const char *rawData)
{
  if (isBinary)
    {
    unsigned long long result = 0;
    ConvertFromNetworkOrder(result, rawData);
    return vtkVariant(result);
    }
  else
    {
    vtkVariant converter(rawData);
    return vtkVariant(converter.ToUnsignedLongLong());
    }
}

#endif // VTK_TYPE_USE_LONG_LONG

// ----------------------------------------------------------------------

vtkVariant ConvertStringToFloat(bool isBinary, const char *rawData)
{
  if (isBinary)
    {
    // As of PostgreSQL version 8.3.0, libpq transmits a float in network
    // byte order -- that is, it reinterprets the bits as an unsigned int
    // and then transmits them that way.  This... frightens me.  It assumes
    // that both sender and recipient use IEEE floats.  Still, I'm not sure
    // there's any other good way to do it.
    unsigned int intResult = 0;
    ConvertFromNetworkOrder(intResult, rawData);

    // This is the idiom that libpq uses internally to convert between the
    // two types.
    union {
      unsigned int i;
      float f;
    } swap;
    swap.i = intResult;
    float floatResult = swap.f;

    return vtkVariant(floatResult);
    }
  else
    {
    vtkStdString rawString(rawData);
    float finalResult;

    // Catch NaN
    if (rawData[0] == 'N' || rawData[0] == 'n')
      {
      if (std::numeric_limits<float>::has_quiet_NaN)
        {
        finalResult = std::numeric_limits<float>::quiet_NaN();
        }
      else
        {
        // C99 defines a NAN macro.  If it's there, that solves our problem.
#if defined(NAN)
        finalResult = NAN;
#else
        float zero = 0.0;
        finalResult = zero / zero;
#endif
        }
      }
    else if (rawString == "Infinity")
      {
      if (std::numeric_limits<float>::has_infinity)
        {
        finalResult = std::numeric_limits<float>::infinity();
        }
      else
        {
        finalResult = VTK_FLOAT_MAX;
        }
      }
    else if (rawString == "-Infinity")
      {
      if (std::numeric_limits<float>::has_infinity)
        {
        finalResult = - std::numeric_limits<float>::infinity();
        }
      else
        {
        finalResult = -VTK_FLOAT_MAX;
        }
      }
    else
      {
      // hurray, it's an ordinary float
      vtkVariant converter(rawData);
      finalResult = converter.ToFloat();
      }
    return vtkVariant(finalResult);
    } // end of handling string representation

}

// ----------------------------------------------------------------------

vtkVariant ConvertStringToVtkIdType(bool isBinary, const char *rawData)
{
  if (isBinary)
    {
    vtkIdType result = 0;
    ConvertFromNetworkOrder(result, rawData);
    return vtkVariant(result);
    }
  else
    {
    vtksys_ios::stringstream convertStream;
    convertStream.str(rawData);
    vtkIdType result;
    convertStream >> result;
    return vtkVariant(result);
    }
}

// ----------------------------------------------------------------------

vtkVariant ConvertStringToDouble(bool isBinary, const char *rawData)
{
  if (isBinary)
    {
    // As of PostgreSQL version 8.3.0, libpq transmits a float in network
    // byte order -- that is, it reinterprets the bits as an unsigned int
    // and then transmits them that way.  This... frightens me.  It assumes
    // that both sender and recipient use IEEE floats.  Still, I'm not sure
    // there's any other good way to do it.

    // Let's hope that we always have a 64-bit type.
    vtkTypeUInt64 intResult = 0;
    ConvertFromNetworkOrder(intResult, rawData);
    union {
      vtkTypeUInt64 i;
      double d;
    } swap;
    swap.i = intResult;
    return vtkVariant(swap.d);
    }
  else
    {
    double finalResult;
    vtkStdString rawString(rawData);

    // Catch NaN
    if (rawData[0] == 'N' || rawData[0] == 'n')
      {
      if (std::numeric_limits<double>::has_quiet_NaN)
        {
        finalResult = std::numeric_limits<double>::quiet_NaN();
        }
      else
        {
        // C99 defines a NAN macro.  If it's there, that solves our problem.
#if defined(NAN)
        finalResult = NAN;
#else
        double zero = 0.0;
        finalResult = zero / zero;
#endif
        }
      }
    else if (rawString == "Infinity")
      {
      if (std::numeric_limits<double>::has_infinity)
        {
        finalResult = std::numeric_limits<double>::infinity();
        }
      else
        {
        finalResult = VTK_DOUBLE_MAX;
        }
      }
    else if (rawString == "-Infinity")
      {
      if (std::numeric_limits<double>::has_infinity)
        {
        finalResult = - std::numeric_limits<double>::infinity();
        }
      else
        {
        finalResult = -VTK_DOUBLE_MAX;
        }
      }
    else
      {
      // hurray, it's an ordinary double
      vtkVariant converter(rawData);
      finalResult = converter.ToDouble();
      }
    return vtkVariant(finalResult);
    } // end of handling string representation
}

// ----------------------------------------------------------------------

bool
vtkPostgreSQLQuery::IsColumnBinary(int whichColumn)
{
  if ((!this->Active) ||
      (!this->Database) ||
      (!this->QueryInternals->QueryResults))
    {
    vtkWarningMacro(<<"No active query!");
    return false;
    }
  else if (whichColumn < 0 || whichColumn >= this->GetNumberOfFields())
    {
    vtkWarningMacro(<<"Illegal column index " << whichColumn);
    return false;
    }
  else
    {
    return (PQfformat(this->QueryInternals->QueryResults, whichColumn) == 1);
    }
}

// ----------------------------------------------------------------------

const char *
vtkPostgreSQLQuery::GetColumnRawData(int whichColumn)
{
  if ((!this->Active) ||
      (!this->Database) ||
      (!this->QueryInternals->QueryResults))
    {
    vtkWarningMacro(<<"No active query!");
    return NULL;
    }
  else if (whichColumn < 0 || whichColumn >= this->GetNumberOfFields())
    {
    vtkWarningMacro(<<"Illegal column index " << whichColumn);
    return NULL;
    }
  else
    {
    return PQgetvalue(this->QueryInternals->QueryResults,
                      this->QueryInternals->CurrentRow,
                      whichColumn);
    }
}

// ----------------------------------------------------------------------

int
vtkPostgreSQLQuery::GetNumberOfRows()
{
  if (!this->Database ||
      !this->Database->IsOpen() ||
      !this->QueryInternals ||
      !this->Active)
    {
    vtkWarningMacro(<<"No active query.  Cannot retrieve number of rows.");
    return 0;
    }
  else
    {
    return PQntuples(this->QueryInternals->QueryResults);
    }
}

