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
  vtkTimerLog *timer=vtkTimerLog::New();
  timer->StartTimer();
#endif //DEBUG_PARALLEL_ORDER_STATISTICS

#if DEBUG_PARALLEL_ORDER_STATISTICS
  vtkTimerLog *timers=vtkTimerLog::New();
  timers->StartTimer();
#endif //DEBUG_PARALLEL_ORDER_STATISTICS
  // First calculate order statistics on local data set
  this->Superclass::Learn( inData, inParameters, outMeta );
#if DEBUG_PARALLEL_ORDER_STATISTICS
  timers->StopTimer();

  cout << "## Process "
       << this->Controller->GetCommunicator()->GetLocalProcessId()
       << " serial engine executed in "
       << timers->GetElapsedTime()
       << " seconds."
       << "\n";

  timers->Delete();
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

  if ( ! outMeta || outMeta->GetNumberOfBlocks() < 1 )
    {
#if DEBUG_PARALLEL_ORDER_STATISTICS
    timer->Delete();
#endif //DEBUG_PARALLEL_ORDER_STATISTICS

    return;
    }

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
    vtkTimerLog *timerA=vtkTimerLog::New();
    vtkTimerLog *timerB=vtkTimerLog::New();
    timerA->StartTimer();
#endif //DEBUG_PARALLEL_ORDER_STATISTICS

    // Create column for global histogram cardinalities
    vtkIdTypeArray* card_g = vtkIdTypeArray::New();
    card_g->SetName( "Cardinality" );

    // Gather all histogram cardinalities on process reduceProc
    // NB: GatherV because the arrays have variable lengths
    com->GatherV( card,
                  card_g,
                  reduceProc );

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
      com->GatherV( dvals,
                    dvals_g,
                    reduceProc );

      // Reduce to global histogram table on process reduceProc
      if ( myRank == reduceProc )
        {
        // Check consistency: we must have as many values as cardinality entries
        vtkIdType nRow_g = card_g->GetNumberOfTuples();
        if ( nRow_g != dvals_g->GetNumberOfTuples() )
          {
          vtkErrorMacro("Gathering error on process "
                        << this->Controller->GetCommunicator()->GetLocalProcessId()
                        << ": inconsistent number of values and cardinality entries: "
                        << dvals_g->GetNumberOfTuples()
                        << " <> "
                        <<  nRow_g
                        << ".");

          return;
          }

        // Reduce to the global histogram table
        vtkstd::map<double,vtkIdType> histogram;
        vtkIdType c;
        for ( vtkIdType r = 1; r < nRow_g; ++ r ) // Skip first row where data set cardinality is stored
          {
          // First retrieve cardinality
          c = card_g->GetValue( r );

          // Then update histogram count for corresponding value
          histogram[dvals->GetTuple1( r )] += c;
          }

        // Now resize global histogram arrays to reduced size
        nRow_g = static_cast<vtkIdType>( histogram.size() );
        dvals_g->SetNumberOfTuples( nRow_g );
        card_g->SetNumberOfTuples( nRow_g );

        // Insert first invalid cardinality value of -1 for data set cardinality to be calculated by Derive
        // Cf. superclass for detailed explanation
        card_g->SetValue( 0, -1 );

        // Value of cardinality row is NaN
        double noVal = vtkMath::Nan();
        dvals_g->SetTuple1( 0, noVal );

        // Then store reduced histogram into array
        vtkstd::map<double,vtkIdType>::iterator hit = histogram.begin();
        for ( vtkIdType r = 1; r < nRow_g; ++ r, ++ hit ) // Skip first row where data set cardinality is stored
          {
          dvals_g->SetTuple1( r, hit->first );
          card_g->SetValue( r, hit->second );
          }

#if DEBUG_PARALLEL_ORDER_STATISTICS
    timerA->StopTimer();

    if ( myRank == reduceProc )
      {
      cout << "## Process "
           << myRank
           << " gathered and reduced in "
           << timerA->GetElapsedTime()
           << " seconds."
           << "\n";
      }
    else // if ( myRank == reduceProc )
      {
      cout << "## Process "
           << myRank
           << " gathered in "
           << timerA->GetElapsedTime()
           << " seconds."
           << "\n";
      }
#endif //DEBUG_PARALLEL_ORDER_STATISTICS

        } // if ( myRank == reduceProc )

#if DEBUG_PARALLEL_ORDER_STATISTICS
    timerB->StartTimer();
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

      // Clean up
      dvals_g->Delete();
      }
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
    timerB->StopTimer();

    cout << "## Process "
         << myRank
         << " broadcasted in "
         << timerB->GetElapsedTime()
         << " seconds."
         << "\n";

    timerA->Delete();
    timerB->Delete();
#endif //DEBUG_PARALLEL_ORDER_STATISTICS

    // Finally, fill the new, global histogram (everyone does this so everyone ends up with the same model)
    vtkVariantArray* row = vtkVariantArray::New();
    row->SetNumberOfValues( 2 );

    // First replace existing rows
    // Start with row 1 and not 0 because of cardinality row (cf. superclass for a detailed explanation)
    //vtkIdType nRowHist = histoTab->GetNumberOfRows();

    // Clean up
    row->Delete();
    card_g->Delete();

    } // for ( unsigned int b = 0; b < nBlocks; ++ b )

#if DEBUG_PARALLEL_ORDER_STATISTICS
  timer->StopTimer();

  cout << "## Process "
       << myRank
       << " parallel Learn took "
       << timer->GetElapsedTime()
       << " seconds."
       << "\n";

  timer->Delete();
#endif //DEBUG_PARALLEL_ORDER_STATISTICS
}
