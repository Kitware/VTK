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
int vtkPOrderStatisticsDataArrayReduce( vtkIdTypeArray* card_g,
                                        vtkDataArray* dvals_g )
{
  // Check consistency: we must have as many values as cardinality entries
  vtkIdType nRow_g = card_g->GetNumberOfTuples();
  if ( nRow_g != dvals_g->GetNumberOfTuples() )
    {
    return 0;
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

  return 1;
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
  vtkIdType reduceProc = 0;

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

    // Gather all histogram cardinalities on process reduceProc
    // NB: GatherV because the arrays have variable lengths
    if ( ! com->GatherV( card,
                         card_g,
                         reduceProc ) )
        {
        vtkErrorMacro("Process "
                      << com->GetLocalProcessId()
                      << " could not gather histogram cardinalities.");

        return;
        }

    // Gather all histogram values on reduceProc and perform reduction of the global histogram table
    if ( vals->IsA("vtkDataArray") )
      {
      // Downcast column to data array for subsequent typed message passing
      vtkDataArray* dvals = vtkDataArray::SafeDownCast( vals );

      // Create column for global histogram values of the same type as the values
      vtkDataArray* dvals_g = vtkDataArray::CreateDataArray( dvals->GetDataType() );
      dvals_g->SetName( "Value" );

      // Gather all histogram values on process reduceProc
      // NB: GatherV because the arrays have variable lengths
      if ( ! com->GatherV( dvals,
                           dvals_g,
                           reduceProc ) )
        {
        vtkErrorMacro("Process "
                      << com->GetLocalProcessId()
                      << " could not gather histogram values.");

        return;
        }

      // Reduce to global histogram table on process reduceProc
      if ( myRank == reduceProc )
        {
        if ( ! vtkPOrderStatisticsDataArrayReduce( card_g, dvals_g ) )
          {
          vtkErrorMacro("Gathering error on process "
                        << this->Controller->GetCommunicator()->GetLocalProcessId()
                        << ": inconsistent number of values and cardinality entries: "
                        << dvals_g->GetNumberOfTuples()
                        << " <> "
                        << card_g->GetNumberOfTuples()
                        << ".");

          return;
          }
        } // if ( myRank == reduceProc )

#if DEBUG_PARALLEL_ORDER_STATISTICS
    timer->StopTimer();

    cout << "## Process "
         << myRank
         << ( myRank == reduceProc ? " gathered and reduced in " : " gathered in " )
         << timer->GetElapsedTime()
         << " seconds."
         << "\n";

    timer->StartTimer();
#endif //DEBUG_PARALLEL_ORDER_STATISTICS

      // Finally broadcast reduced histogram values
      if ( ! com->Broadcast( dvals_g,
                             reduceProc ) )
        {
        vtkErrorMacro("Process "
                      << com->GetLocalProcessId()
                      << " could not broadcast reduced histogram values.");

        return;
        }

        //
        histoTab_g->AddColumn( dvals_g );
        dvals_g->Delete();
      } // if ( vals->IsA("vtkDataArray") )
    else if ( vals->IsA("vtkStringArray") )
      {
      // Packing step: concatenate all x and c values
      vtkstd::vector<vtkIdType> cValues_l;

      // (All) gather all x and c sizes

      // Calculate total size and displacement arrays

      // Allocate receive buffers on reducer process, based on the global sizes obtained above

      // Gather all xPacked and cValues on process reduceProc
      // NB: GatherV because the packets have variable lengths

      }
    else if ( vals->IsA("vtkVariantArray") )
      {
      }
    else
      {
      vtkErrorMacro( "Unsupported data type for column "
                       << vals->GetName()
                       << ". Ignoring it." );
      return;
      }


    // Finally broadcast reduced histogram cardinalities
    if ( ! com->Broadcast( card_g,
                           reduceProc ) )
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
