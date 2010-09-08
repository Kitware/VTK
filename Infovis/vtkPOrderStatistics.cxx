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
#define DEBUG_PARALLEL_ORDER_STATISTICS 0

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
void PackUnivariateValues( const vtkstd::vector<vtkStdString>& values,
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
void UnpackUnivariateValues( const vtkStdString& buffer,
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

// ----------------------------------------------------------------------
void vtkPOrderStatistics::Learn( vtkTable* inData,
                                 vtkTable* inParameters,
                                 vtkMultiBlockDataSet* outMeta )
{
#if DEBUG_PARALLEL_ORDER_STATISTICS
  vtkTimerLog *timer=vtkTimerLog::New();
  timer->StartTimer();
#endif //DEBUG_PARALLEL_ORDER_STATISTICS

  if ( ! outMeta )
    {
#if DEBUG_PARALLEL_ORDER_STATISTICS
    timer->Delete();
#endif //DEBUG_PARALLEL_ORDER_STATISTICS

    return;
    }

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

  // Get a hold of the summary table
  vtkTable* summaryTab = vtkTable::SafeDownCast( outMeta->GetBlock( 0 ) );
  if ( ! summaryTab )
    {
#if DEBUG_PARALLEL_ORDER_STATISTICS
    timer->Delete();
#endif //DEBUG_PARALLEL_ORDER_STATISTICS

    return;
    }

  // Determine how many variables are present
  vtkIdType nRowSumm = summaryTab->GetNumberOfRows();
  if ( nRowSumm <= 0 )
    {
    // No statistics were calculated in serial.
#if DEBUG_PARALLEL_ORDER_STATISTICS
    timer->Delete();
#endif //DEBUG_PARALLEL_ORDER_STATISTICS

    return;
    }

  // Get a hold of the histogram table
  vtkTable* histoTab = vtkTable::SafeDownCast( outMeta->GetBlock( 1 ) );
  if ( ! histoTab )
    {
#if DEBUG_PARALLEL_ORDER_STATISTICS
    timer->Delete();
#endif //DEBUG_PARALLEL_ORDER_STATISTICS

    return;
    }

  // Determine how many realizations are present
  vtkIdType nRowHisto = histoTab->GetNumberOfRows();
  if ( nRowHisto <= 0 )
    {
    // No statistics were calculated in serial.
#if DEBUG_PARALLEL_ORDER_STATISTICS
    timer->Delete();
#endif //DEBUG_PARALLEL_ORDER_STATISTICS

    return;
    }

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

  vtkIdType myRank = com->GetLocalProcessId();

  // Packing step: concatenate all x values in a single string and all (k,c) pairs in single vector
  vtkStdString xPacked_l;
  vtkstd::vector<vtkIdType> kcValues_l;
  if ( this->Pack( histoTab,
                   xPacked_l,
                   kcValues_l ) )
    {
    vtkErrorMacro("Packing error on process "
                  << myRank
                  << ".");

    return;
    }

  // NB: Use process 0 as sole reducer for now
  vtkIdType reduceProc = 0;

  // (All) gather all x and kc sizes
  vtkIdType xSize_l = xPacked_l.size();
  vtkIdType* xSize_g = new vtkIdType[np];

  vtkIdType kcSize_l = kcValues_l.size();
  vtkIdType* kcSize_g = new vtkIdType[np];

  com->AllGather( &xSize_l,
                  xSize_g,
                  1 );

  com->AllGather( &kcSize_l,
                  kcSize_g,
                  1 );

  // Calculate total size and displacement arrays
  vtkIdType* xOffset = new vtkIdType[np];
  vtkIdType* kcOffset = new vtkIdType[np];

  vtkIdType xSizeTotal = 0;
  vtkIdType kcSizeTotal = 0;

  for ( vtkIdType i = 0; i < np; ++ i )
    {
    xOffset[i] = xSizeTotal;
    kcOffset[i] = kcSizeTotal;

    xSizeTotal += xSize_g[i];
    kcSizeTotal += kcSize_g[i];
    }

  // Allocate receive buffers on reducer process, based on the global sizes obtained above
  char* xPacked_g = 0;
  vtkIdType*  kcValues_g = 0;
  if ( myRank == reduceProc )
    {
    xPacked_g = new char[xSizeTotal];
    kcValues_g = new vtkIdType[kcSizeTotal];
    }

  // Gather all xPacked and kcValues on process reduceProc
  // NB: GatherV because the packets have variable lengths
  if ( ! com->GatherV( &(*xPacked_l.begin()),
                       xPacked_g,
                       xSize_l,
                       xSize_g,
                       xOffset,
                       reduceProc ) )
    {
    vtkErrorMacro("Process "
                  << myRank
                  << "could not gather x values.");

    return;
    }

  if ( ! com->GatherV( &(*kcValues_l.begin()),
                       kcValues_g,
                       kcSize_l,
                       kcSize_g,
                       kcOffset,
                       reduceProc ) )
    {
    vtkErrorMacro("Process "
                  << myRank
                  << "could not gather (k,c) values.");

    return;
    }

  // Reduction step: have process reduceProc perform the reduction of the global histogram table
  if ( myRank == reduceProc )
    {
    if ( this->Reduce( xSizeTotal,
                       xPacked_g,
                       xPacked_l,
                       kcSizeTotal,
                       kcValues_g,
                       kcValues_l ) )
      {
      return;
      }
    } // if ( myRank == reduceProc )

#if DEBUG_PARALLEL_ORDER_STATISTICS
  vtkTimerLog *timerB=vtkTimerLog::New();
  timerB->StartTimer();
#endif //DEBUG_PARALLEL_ORDER_STATISTICS

  // Broadcasting step: broadcast reduced histogram table to all processes
  vtkstd::vector<vtkStdString> xValues_l; // local consecutive x values
  if ( this->Broadcast( xSizeTotal,
                        xPacked_l,
                        xValues_l,
                        kcSizeTotal,
                        kcValues_l,
                        reduceProc ) )
    {
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

  timerB->Delete();
#endif //DEBUG_PARALLEL_ORDER_STATISTICS

  // Finally, fill the new, global histogram (everyone does this so everyone ends up with the same model)
  vtkVariantArray* row3 = vtkVariantArray::New();
  row3->SetNumberOfValues( 3 );

  vtkstd::vector<vtkStdString>::iterator xit = xValues_l.begin();
  vtkstd::vector<vtkIdType>::iterator    kcit = kcValues_l.begin();

  // First replace existing rows
  // Start with row 1 and not 0 because of cardinality row (cf. superclass for a detailed explanation)
  for ( vtkIdType r = 1 ; r < nRowHisto; ++ r, xit ++, kcit += 2 )
    {
    row3->SetValue( 0, *kcit );
    row3->SetValue( 1, *xit );
    row3->SetValue( 2, *(kcit + 1) );

    histoTab->SetRow( r, row3 );
    }

  // Then insert new rows
  for ( ; xit != xValues_l.end() ; xit ++, kcit += 2 )
    {
    row3->SetValue( 0, *kcit );
    row3->SetValue( 1, *xit );
    row3->SetValue( 2, *(kcit + 1) );

    histoTab->InsertNextRow( row3 );
    }

  // Clean up
  row3->Delete();

  if ( myRank == reduceProc )
    {
    delete [] xPacked_g;
    delete [] kcValues_g;
    }

  delete [] xSize_g;
  delete [] kcSize_g;
  delete [] xOffset;
  delete [] kcOffset;

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

// ----------------------------------------------------------------------
bool vtkPOrderStatistics::Pack( vtkTable* histoTab,
                                vtkStdString& xPacked,
                                vtkstd::vector<vtkIdType>& kcValues )
{
  // Downcast meta columns to string arrays for efficient data access
  vtkIdTypeArray* keys = vtkIdTypeArray::SafeDownCast( histoTab->GetColumnByName( "Key" ) );
  vtkStringArray* vals = vtkStringArray::SafeDownCast( histoTab->GetColumnByName( "Value" ) );
  vtkIdTypeArray* card = vtkIdTypeArray::SafeDownCast( histoTab->GetColumnByName( "Cardinality" ) );
  if ( ! keys || ! vals || ! card )
    {
    return true;
    }

  vtkstd::vector<vtkStdString> xValues; // consecutive x values

  vtkIdType nRowHisto = histoTab->GetNumberOfRows();
  for ( vtkIdType r = 1; r < nRowHisto; ++ r ) // Skip first row which is reserved for data set cardinality
    {
    // Push back x to list of strings
    xValues.push_back( vals->GetValue( r ) );

    // Push back X index and #(x) to list of strings
    kcValues.push_back( keys->GetValue( r ) );
    kcValues.push_back( card->GetValue( r ) );
    }

  // Concatenate vector of strings into single string
  PackUnivariateValues( xValues, xPacked );

  return false;
}

// ----------------------------------------------------------------------
bool vtkPOrderStatistics::Reduce( vtkIdType& xSizeTotal,
                                  char* xPacked_g,
                                  vtkStdString& xPacked_l,
                                  vtkIdType& kcSizeTotal,
                                  vtkIdType*  kcValues_g,
                                  vtkstd::vector<vtkIdType>& kcValues_l )
{
#if DEBUG_PARALLEL_ORDER_STATISTICS
  vtkTimerLog *timer=vtkTimerLog::New();
  timer->StartTimer();

  cout << "\n## Reduce received character string of size "
       << xSizeTotal
       << " and integer array of size "
       << kcSizeTotal
       << "... ";
#endif //DEBUG_PARALLEL_ORDER_STATISTICS

  // First, unpack the packet of strings
  vtkstd::vector<vtkStdString> xValues_g;
  UnpackUnivariateValues( vtkStdString ( xPacked_g, xSizeTotal ), xValues_g );

  // Second, check consistency: we must have half as many x than kc entries
  if ( 2 * vtkIdType( xValues_g.size() ) != kcSizeTotal )
    {
    vtkErrorMacro("Reduction error on process "
                  << this->Controller->GetCommunicator()->GetLocalProcessId()
                  << ": inconsistent number of x values and (k,c) pairs: "
                  << xValues_g.size()
                  << " <> "
                  << kcSizeTotal / 2
                  << ".");

    return true;
    }

  // Third, reduce to the global histogram table
  typedef vtkstd::map<vtkStdString,vtkIdType> Distribution;
  vtkstd::map<vtkIdType,Distribution> histoTable;

  vtkIdType i = 0;
  for ( vtkstd::vector<vtkStdString>::iterator vit = xValues_g.begin(); vit != xValues_g.end(); vit ++, i += 2 )
    {
    histoTable
      [kcValues_g[i]]
      [*vit]
      += kcValues_g[i + 1];
    }

  // Fourth, prepare send buffers of (global) x and kc values
  vtkstd::vector<vtkStdString> xValues_l;
  kcValues_l.clear();
  for ( vtkstd::map<vtkIdType,Distribution>::iterator mit = histoTable.begin();
        mit != histoTable.end(); ++ mit )
    {
    Distribution histogram = mit->second;
    for ( Distribution::iterator hit = histogram.begin();
          hit != histogram.end(); ++ hit )
      {
      // Push back x to list of strings
      xValues_l.push_back( hit->first );  // x

      // Push back X index and #(x) to list of strings
      kcValues_l.push_back( mit->first );  // k
      kcValues_l.push_back( hit->second ); // c
      }
    } // mit
  PackUnivariateValues( xValues_l, xPacked_l );

  // Last, update x and kc buffer sizes (which have changed because of the reduction)
  xSizeTotal = xPacked_l.size();
  kcSizeTotal = kcValues_l.size();

#if DEBUG_PARALLEL_ORDER_STATISTICS
  timer->StopTimer();

  cout<< " and completed in "
      << timer->GetElapsedTime()
      << " seconds."
      << "\n\n";

  timer->Delete();
#endif //DEBUG_PARALLEL_ORDER_STATISTICS

  return false;
}


// ----------------------------------------------------------------------
bool vtkPOrderStatistics::Broadcast( vtkIdType xSizeTotal,
                                     vtkStdString& xPacked,
                                     vtkstd::vector<vtkStdString>& xValues,
                                     vtkIdType kcSizeTotal,
                                     vtkstd::vector<vtkIdType>& kcValues,
                                     vtkIdType reduceProc )
{
  vtkCommunicator* com = this->Controller->GetCommunicator();

  // Broadcast the x and kc buffer sizes
  if ( ! com->Broadcast( &xSizeTotal,
                         1,
                         reduceProc ) )
    {
    vtkErrorMacro("Process "
                  << com->GetLocalProcessId()
                  << " could not broadcast x buffer size.");

    return true;
    }

  if ( ! com->Broadcast( &kcSizeTotal,
                         1,
                         reduceProc ) )
    {
    vtkErrorMacro("Process "
                  << com->GetLocalProcessId()
                  << " could not broadcast (k,c) buffer size.");

    return true;
    }

  // Resize vectors so they can receive the broadcasted x and kc values
  xPacked.resize( xSizeTotal );
  kcValues.resize( kcSizeTotal );

  // Broadcast the contents of histogram table to everyone
  if ( ! com->Broadcast( &(*xPacked.begin()),
                         xSizeTotal,
                         reduceProc ) )
    {
    vtkErrorMacro("Process "
                  << com->GetLocalProcessId()
                  << " could not broadcast x values.");

    return true;
    }

  if ( ! com->Broadcast( &(*kcValues.begin()),
                         kcSizeTotal,
                         reduceProc ) )
    {
    vtkErrorMacro("Process "
                  << com->GetLocalProcessId()
                  << " could not broadcast (k,c) values.");

    return true;
    }

  // Unpack the packet of strings
  UnpackUnivariateValues( xPacked, xValues );

  return false;
}
