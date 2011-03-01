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

    // Packing step: concatenate all x and c values
    vtkstd::vector<vtkIdType> cValues_l;

    if ( vals->IsA("vtkDataArray") )
      {
      }
    else if ( vals->IsA("vtkStringArray") )
      {
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

    // (All) gather all x and c sizes

    // Calculate total size and displacement arrays

    // Allocate receive buffers on reducer process, based on the global sizes obtained above

    // Gather all xPacked and cValues on process reduceProc
    // NB: GatherV because the packets have variable lengths

    // Reduction step: have process reduceProc perform the reduction of the global histogram table
    if ( myRank == reduceProc )
      {
      } // if ( myRank == reduceProc )

#if DEBUG_PARALLEL_ORDER_STATISTICS
    vtkTimerLog *timerB=vtkTimerLog::New();
    timerB->StartTimer();
#endif //DEBUG_PARALLEL_ORDER_STATISTICS

    // Broadcasting step: broadcast reduced histogram table to all processes

#if DEBUG_PARALLEL_ORDER_STATISTICS
    timerB->StopTimer();

    cout << "## Process "
         << myRank
         << " broadcasted in "
         << timerB->GetElapsedTime()
         << " seconds."
         << "\n";

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
