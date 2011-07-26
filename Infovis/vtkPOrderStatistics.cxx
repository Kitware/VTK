/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkPContingencyStatistics.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2011 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------*/
#if defined(_MSC_VER)
#pragma warning (disable:4503)
#endif

#include "vtkToolkits.h" 
#include "vtkPOrderStatistics.h"

#include "vtkCommunicator.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

#include <vtkstd/map>
#include <vtkstd/set>
#include <vtkstd/vector>

// For debugging purposes, output message sizes and intermediate timings
#define DEBUG_PARALLEL_ORDER_STATISTICS 1

#if DEBUG_PARALLEL_ORDER_STATISTICS
#include "vtkTimerLog.h"
#endif // DEBUG_PARALLEL_ORDER_STATISTICS

vtkStandardNewMacro(vtkPOrderStatistics);
vtkCxxSetObjectMacro(vtkPOrderStatistics, Controller, vtkMultiProcessController);
//-----------------------------------------------------------------------------
vtkPOrderStatistics::vtkPOrderStatistics()
{
  this->Controller = 0;
  this->SetController( vtkMultiProcessController::GetGlobalController() );
}

//-----------------------------------------------------------------------------
vtkPOrderStatistics::~vtkPOrderStatistics()
{
  this->SetController( 0 );
}

//-----------------------------------------------------------------------------
void vtkPOrderStatistics::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}

//-----------------------------------------------------------------------------
static void PackValues( const vtkstd::vector<vtkStdString>& values,
                        vtkStdString& buffer )
{
  buffer.clear();

  for( vtkstd::vector<vtkStdString>::const_iterator it = values.begin();
       it != values.end(); ++ it )
    {
    buffer.append( *it );
    buffer.push_back( 0 );
    }
}

//-----------------------------------------------------------------------------
static void UnpackValues( const vtkStdString& buffer,
                          vtkstd::vector<vtkStdString>& values )
{
  values.clear();
  
  const char* const bufferEnd = &buffer[0] + buffer.size();
  
  for( const char* start = &buffer[0]; start != bufferEnd; ++ start )
    {
    for( const char* finish = start; finish != bufferEnd; ++ finish )
      {
      if( ! *finish )
        {
        values.push_back( vtkStdString( start ) );
        start = finish;
        break;
        }
      }
    }
}

//-----------------------------------------------------------------------------
bool vtkPOrderStatistics::ReduceData( vtkIdTypeArray* card_g,
                                      vtkDataArray* dvals_g )
{
  // Check consistency: we must have as many values as cardinality entries
  vtkIdType nRow_g = card_g->GetNumberOfTuples();
  if ( dvals_g->GetNumberOfTuples() != nRow_g )
    {
    vtkErrorMacro("Gathering error on process "
                  << this->Controller->GetCommunicator()->GetLocalProcessId()
                  << ": inconsistent number of values and cardinality entries: "
                  << dvals_g->GetNumberOfTuples()
                  << " <> "
                  << nRow_g
                  << ".");
    
    return true;
    }

  // Reduce to the global histogram table
  vtkstd::map<double,vtkIdType> histogram;
  double x;
  vtkIdType c;
  for ( vtkIdType r = 0; r < nRow_g; ++ r )
    {
    // First, fetch value
    x = dvals_g->GetTuple1( r );

    // Then, retrieve corresponding cardinality
    c = card_g->GetValue( r );

    // Last, update histogram count for corresponding value
    histogram[x] += c;
    }

  // Now resize global histogram arrays to reduced size
  nRow_g = static_cast<vtkIdType>( histogram.size() );
  dvals_g->SetNumberOfTuples( nRow_g );
  card_g->SetNumberOfTuples( nRow_g );

  // Then store reduced histogram into array
  vtkstd::map<double,vtkIdType>::iterator hit = histogram.begin();
  for ( vtkIdType r = 0; r < nRow_g; ++ r, ++ hit )
    {
    dvals_g->SetTuple1( r, hit->first );
    card_g->SetValue( r, hit->second );
    }

  return false;
}

//-----------------------------------------------------------------------------
bool vtkPOrderStatistics::ReduceString( vtkIdTypeArray* card_g,
                                        vtkIdType& ncTot,
                                        char* spack_g,
                                        vtkStringArray* svals_g )
{
  // First, unpack the packet of strings
  vtkstd::vector<vtkStdString> svect_g;
  UnpackValues( vtkStdString ( spack_g, ncTot ), svect_g );

  // Second, check consistency: we must have as many values as cardinality entries
  vtkIdType nRow_g = card_g->GetNumberOfTuples();
  if ( vtkIdType( svect_g.size() ) != nRow_g )
    {
    vtkErrorMacro("Gathering error on process "
                  << this->Controller->GetCommunicator()->GetLocalProcessId()
                  << ": inconsistent number of values and cardinality entries: "
                  << svect_g.size()
                  << " <> "
                  << nRow_g
                  << ".");

    return true;
    }

  // Third, reduce to the global histogram
  vtkstd::map<vtkStdString,vtkIdType> histogram;
  vtkIdType c;
  vtkIdType i = 0;
  for ( vtkstd::vector<vtkStdString>::iterator vit = svect_g.begin(); 
        vit != svect_g.end(); ++ vit , ++ i )
    {
    // First, retrieve cardinality
    c = card_g->GetValue( i );

    // Then, update histogram count for corresponding value
    histogram[*vit] += c;
    }

  // Now resize global histogram arrays to reduced size
  nRow_g = static_cast<vtkIdType>( histogram.size() );
  svals_g->SetNumberOfValues( nRow_g );
  card_g->SetNumberOfTuples( nRow_g );

  // Then store reduced histogram into array
  vtkstd::map<vtkStdString,vtkIdType>::iterator hit = histogram.begin();
  for ( vtkIdType r = 0; r < nRow_g; ++ r, ++ hit )
    {
    svals_g->SetValue( r, hit->first );
    card_g->SetValue( r, hit->second );
    }

  return false;
}

// ----------------------------------------------------------------------
void vtkPOrderStatistics::Learn( vtkTable* inData,
                                 vtkTable* inParameters,
                                 vtkMultiBlockDataSet* outMeta )
{
  if ( ! outMeta )
    {
    return;
    }

#if DEBUG_PARALLEL_ORDER_STATISTICS
  vtkTimerLog *timer = vtkTimerLog::New();
  timer->StartTimer();
#endif //DEBUG_PARALLEL_ORDER_STATISTICS

  // First calculate order statistics on local data set
  this->Superclass::Learn( inData, inParameters, outMeta );

  if ( ! outMeta
       || outMeta->GetNumberOfBlocks() < 1 )
    {
    // No statistics were calculated.
#if DEBUG_PARALLEL_ORDER_STATISTICS
    timer->Delete();
#endif //DEBUG_PARALLEL_ORDER_STATISTICS

    return;
    }

#if DEBUG_PARALLEL_ORDER_STATISTICS
  timer->StopTimer();

  cout << "## Process "
       << this->Controller->GetCommunicator()->GetLocalProcessId()
       << " serial engine executed in "
       << timer->GetElapsedTime()
       << " seconds."
       << "\n";
#endif //DEBUG_PARALLEL_ORDER_STATISTICS

  // Make sure that parallel updates are needed, otherwise leave it at that.
  int np = this->Controller->GetNumberOfProcesses();
  if ( np < 2 )
    {
#if DEBUG_PARALLEL_ORDER_STATISTICS
    timer->Delete();
#endif //DEBUG_PARALLEL_ORDER_STATISTICS

    return;
    }

  // Get ready for parallel calculations
  vtkCommunicator* com = this->Controller->GetCommunicator();
  if ( ! com )
    {
    vtkErrorMacro("No parallel communicator.");
    }

  // Figure local process id
  vtkIdType myRank = com->GetLocalProcessId();

  // NB: Use process 0 as sole reducer for now
  vtkIdType rProc = 0;

  // Iterate over primary tables
  unsigned int nBlocks = outMeta->GetNumberOfBlocks();
  for ( unsigned int b = 0; b < nBlocks; ++ b )
    {
    // Fetch histogram table
    vtkTable* histoTab = vtkTable::SafeDownCast( outMeta->GetBlock( b ) );
    if ( ! histoTab  )
      {
      continue;
      }

    // Downcast columns to typed arrays for efficient data access
    vtkAbstractArray* vals =  histoTab->GetColumnByName( "Value" );
    vtkIdTypeArray* card = vtkIdTypeArray::SafeDownCast( histoTab->GetColumnByName( "Cardinality" ) );
    if ( ! vals || ! card )
      {
      vtkErrorMacro("Column fetching error on process "
                    << myRank
                    << ".");

      return;
      }

#if DEBUG_PARALLEL_ORDER_STATISTICS
    timer->StartTimer();
#endif //DEBUG_PARALLEL_ORDER_STATISTICS

    // Create new table for global histogram
    vtkTable* histoTab_g = vtkTable::New();

    // Create column for global histogram cardinalities
    vtkIdTypeArray* card_g = vtkIdTypeArray::New();
    card_g->SetName( "Cardinality" );

    // Gather all histogram cardinalities on process rProc
    // NB: GatherV because the arrays have variable lengths
    if ( ! com->GatherV( card, card_g, rProc ) )
        {
        vtkErrorMacro("Process "
                      << com->GetLocalProcessId()
                      << " could not gather histogram cardinalities.");

        return;
        }

    // Gather all histogram values on rProc and perform reduction of the global histogram table
    if ( vals->IsA("vtkDataArray") )
      {
      // Downcast column to data array for subsequent typed message passing
      vtkDataArray* dvals = vtkDataArray::SafeDownCast( vals );

      // Create column for global histogram values of the same type as the values
      vtkDataArray* dvals_g = vtkDataArray::CreateDataArray( dvals->GetDataType() );
      dvals_g->SetName( "Value" );

      // Gather all histogram values on process rProc
      // NB: GatherV because the arrays have variable lengths
      if ( ! com->GatherV( dvals, dvals_g, rProc ) )
        {
        vtkErrorMacro("Process "
                      << com->GetLocalProcessId()
                      << " could not gather histogram values.");

        return;
        }

      // Reduce to global histogram table on process rProc
      if ( myRank == rProc )
        {
        if ( this->ReduceData( card_g, dvals_g ) )
          {
          return;
          }
        } // if ( myRank == rProc )

#if DEBUG_PARALLEL_ORDER_STATISTICS
      timer->StopTimer();
      
      cout << "## Process "
           << myRank
           << ( myRank == rProc ? " gathered and reduced in " : " gathered in " )
           << timer->GetElapsedTime()
           << " seconds."
           << "\n";
      
      timer->StartTimer();
#endif //DEBUG_PARALLEL_ORDER_STATISTICS

      // Finally broadcast reduced histogram values
      if ( ! com->Broadcast( dvals_g, rProc ) )
        {
        vtkErrorMacro("Process "
                      << com->GetLocalProcessId()
                      << " could not broadcast reduced histogram values.");

        return;
        }

      // Add column of data values to histogram table
      histoTab_g->AddColumn( dvals_g );
      
      // Clean up
      dvals_g->Delete();
      } // if ( vals->IsA("vtkDataArray") )
    else if ( vals->IsA("vtkStringArray") )
      {
      // Downcast column to string array for subsequent typed message passing
      vtkStringArray* svals = vtkStringArray::SafeDownCast( vals );

      // Packing step: concatenate all string values
      vtkStdString spack_l;
      if ( this->Pack( svals, spack_l ) )
        {
        vtkErrorMacro("Packing error on process "
                      << myRank
                      << ".");

        return;
        }

      // (All) gather all string sizes
      vtkIdType nc_l = spack_l.size();
      vtkIdType* nc_g = new vtkIdType[np];
      com->AllGather( &nc_l, nc_g, 1 );

      // Calculate total size and displacement arrays
      vtkIdType* offsets = new vtkIdType[np];
      vtkIdType ncTot = 0;

      for ( vtkIdType i = 0; i < np; ++ i )
        {
        offsets[i] = ncTot;
        ncTot += nc_g[i];
        }

      // Allocate receive buffer on reducer process, based on the global size obtained above
      char* spack_g = 0;
      if ( myRank == rProc )
        {
        spack_g = new char[ncTot];
        }

      // Create column for global histogram values of the same type as the values
      vtkStringArray* svals_g = vtkStringArray::New();
      svals_g->SetName( "Value" );

      // Gather all spack on process rProc
      // NB: GatherV because the packets have variable lengths
      if ( ! com->GatherV( &(*spack_l.begin()), spack_g, nc_l, nc_g, offsets, rProc ) )
        {
        vtkErrorMacro("Process "
                      << myRank
                      << "could not gather string values.");
        
        return;
        }

      // Reduce to global histogram on process rProc
      if ( myRank == rProc )
        {
        if ( this->ReduceString( card_g, ncTot, spack_g, svals_g ) )
          {
          return;
          }
        } // if ( myRank == rProc )

#if DEBUG_PARALLEL_ORDER_STATISTICS
      timer->StopTimer();
      
      cout << "## Process "
           << myRank
           << ( myRank == rProc ? " gathered and reduced in " : " gathered in " )
           << timer->GetElapsedTime()
           << " seconds."
           << "\n";
      
      timer->StartTimer();
#endif //DEBUG_PARALLEL_ORDER_STATISTICS

      // Add column of string values to histogram table
      histoTab_g->AddColumn( svals_g );
      
      // Clean up
      svals_g->Delete();
      } // else if ( vals->IsA("vtkStringArray") )
    else if ( vals->IsA("vtkVariantArray") )
      {
      vtkErrorMacro( "Unsupported data type (variant array) for column "
                       << vals->GetName()
                       << ". Ignoring it." );
      return;
      }
    else
      {
      vtkErrorMacro( "Unsupported data type for column "
                       << vals->GetName()
                       << ". Ignoring it." );
      return;
      }


    // Finally broadcast reduced histogram cardinalities
    if ( ! com->Broadcast( card_g, rProc ) )
      {
      vtkErrorMacro("Process "
                    << com->GetLocalProcessId()
                    << " could not broadcast reduced histogram cardinalities.");

      return;
      }

#if DEBUG_PARALLEL_ORDER_STATISTICS
    timer->StopTimer();

    cout << "## Process "
         << myRank
         << " broadcasted in "
         << timer->GetElapsedTime()
         << " seconds."
         << "\n";

    timer->Delete();
#endif //DEBUG_PARALLEL_ORDER_STATISTICS

    //
    histoTab_g->AddColumn( card_g );
    card_g->Delete();

    // Replace local histogram table with globally reduced one
    outMeta->SetBlock( b, histoTab_g );

    // Clean up
    histoTab_g->Delete();
    } // for ( unsigned int b = 0; b < nBlocks; ++ b )
}

// ----------------------------------------------------------------------
bool vtkPOrderStatistics::Pack( vtkStringArray* svals,
                                vtkStdString& spack )
{
  vtkstd::vector<vtkStdString> svect; // consecutive strings

  vtkIdType nv = svals->GetNumberOfValues();
  for ( vtkIdType i = 0; i < nv; ++ i )
    {
    // Push back current string value
    svect.push_back( svals->GetValue( i ) );
    }

  // Concatenate vector of strings into single string
  PackValues( svect, spack );

  return false;
}
