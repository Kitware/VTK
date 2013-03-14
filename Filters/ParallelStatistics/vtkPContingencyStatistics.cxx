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

#include "vtkPContingencyStatistics.h"

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

#include <map>
#include <set>
#include <vector>

// For debugging purposes, output message sizes and intermediate timings
#define DEBUG_PARALLEL_CONTINGENCY_STATISTICS 0

#if DEBUG_PARALLEL_CONTINGENCY_STATISTICS
#include "vtkTimerLog.h"
#endif // DEBUG_PARALLEL_CONTINGENCY_STATISTICS

vtkStandardNewMacro(vtkPContingencyStatistics);
vtkCxxSetObjectMacro(vtkPContingencyStatistics, Controller, vtkMultiProcessController);
//-----------------------------------------------------------------------------
vtkPContingencyStatistics::vtkPContingencyStatistics()
{
  this->Controller = 0;
  this->SetController( vtkMultiProcessController::GetGlobalController() );
}

//-----------------------------------------------------------------------------
vtkPContingencyStatistics::~vtkPContingencyStatistics()
{
  this->SetController( 0 );
}

//-----------------------------------------------------------------------------
void vtkPContingencyStatistics::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}

//-----------------------------------------------------------------------------
static void StringVectorToStringBuffer( const std::vector<vtkStdString>& strings,
                                        vtkStdString& buffer )
{
  buffer.clear();

  for( std::vector<vtkStdString>::const_iterator it = strings.begin();
       it != strings.end(); ++ it )
    {
    buffer.append( *it );
    buffer.push_back( 0 );
    }
}

// ----------------------------------------------------------------------
static bool StringArrayToStringBuffer( vtkTable* contingencyTab,
                                       vtkStdString& xyPacked,
                                       std::vector<vtkIdType>& kcValues )
{
  // Downcast meta columns to string arrays for efficient data access
  vtkIdTypeArray* keys = vtkIdTypeArray::SafeDownCast( contingencyTab->GetColumnByName( "Key" ) );
  vtkStringArray* valx = vtkStringArray::SafeDownCast( contingencyTab->GetColumnByName( "x" ) );
  vtkStringArray* valy = vtkStringArray::SafeDownCast( contingencyTab->GetColumnByName( "y" ) );
  vtkIdTypeArray* card = vtkIdTypeArray::SafeDownCast( contingencyTab->GetColumnByName( "Cardinality" ) );
  if ( ! keys || ! valx || ! valy || ! card )
    {
    return true;
    }

  std::vector<vtkStdString> xyValues; // consecutive (x,y) pairs

  vtkIdType nRowCont = contingencyTab->GetNumberOfRows();
  for ( vtkIdType r = 1; r < nRowCont; ++ r ) // Skip first row which is reserved for data set cardinality
    {
    // Push back x and y to list of strings
    xyValues.push_back( valx->GetValue( r ) );
    xyValues.push_back( valy->GetValue( r ) );

    // Push back (X,Y) index and #(x,y) to list of strings
    kcValues.push_back( keys->GetValue( r ) );
    kcValues.push_back( card->GetValue( r ) );
    }

  // Concatenate vector of strings into single string
  StringVectorToStringBuffer( xyValues, xyPacked );

  return false;
}

//-----------------------------------------------------------------------------
static void StringBufferToStringVector( const vtkStdString& buffer,
                                        std::vector<vtkStdString>& strings )
{
  strings.clear();

  const char* const bufferEnd = &buffer[0] + buffer.size();

  for( const char* start = &buffer[0]; start != bufferEnd; ++ start )
    {
    for( const char* finish = start; finish != bufferEnd; ++ finish )
      {
      if( ! *finish )
        {
        strings.push_back( vtkStdString( start ) );
        start = finish;
        break;
        }
      }
    }
}

// ----------------------------------------------------------------------
void vtkPContingencyStatistics::Learn( vtkTable* inData,
                                       vtkTable* inParameters,
                                       vtkMultiBlockDataSet* outMeta )
{
#if DEBUG_PARALLEL_CONTINGENCY_STATISTICS
  vtkTimerLog *timer=vtkTimerLog::New();
  timer->StartTimer();
#endif //DEBUG_PARALLEL_CONTINGENCY_STATISTICS

  if ( ! outMeta )
    {
#if DEBUG_PARALLEL_CONTINGENCY_STATISTICS
  timer->Delete();
#endif //DEBUG_PARALLEL_CONTINGENCY_STATISTICS

    return;
    }

#if DEBUG_PARALLEL_CONTINGENCY_STATISTICS
  vtkTimerLog *timers=vtkTimerLog::New();
  timers->StartTimer();
#endif //DEBUG_PARALLEL_CONTINGENCY_STATISTICS
  // First calculate contingency statistics on local data set
  this->Superclass::Learn( inData, inParameters, outMeta );
#if DEBUG_PARALLEL_CONTINGENCY_STATISTICS
  timers->StopTimer();

  cout << "## Process "
       << this->Controller->GetCommunicator()->GetLocalProcessId()
       << " serial engine executed in "
       << timers->GetElapsedTime()
       << " seconds."
       << "\n";

  timers->Delete();
#endif //DEBUG_PARALLEL_CONTINGENCY_STATISTICS

  // Get a hold of the summary table
  vtkTable* summaryTab = vtkTable::SafeDownCast( outMeta->GetBlock( 0 ) );
  if ( ! summaryTab )
    {
#if DEBUG_PARALLEL_CONTINGENCY_STATISTICS
  timer->Delete();
#endif //DEBUG_PARALLEL_CONTINGENCY_STATISTICS

    return;
    }

  // Determine how many (X,Y) variable pairs are present
  vtkIdType nRowSumm = summaryTab->GetNumberOfRows();
  if ( nRowSumm <= 0 )
    {
    // No statistics were calculated in serial.
#if DEBUG_PARALLEL_CONTINGENCY_STATISTICS
  timer->Delete();
#endif //DEBUG_PARALLEL_CONTINGENCY_STATISTICS

    return;
    }

  // Get a hold of the contingency table
  vtkTable* contingencyTab = vtkTable::SafeDownCast( outMeta->GetBlock( 1 ) );
  if ( ! contingencyTab )
    {
#if DEBUG_PARALLEL_CONTINGENCY_STATISTICS
  timer->Delete();
#endif //DEBUG_PARALLEL_CONTINGENCY_STATISTICS

    return;
    }

  // Determine number of (x,y) realizations are present
  vtkIdType nRowCont = contingencyTab->GetNumberOfRows();
  if ( nRowCont <= 0 )
    {
    // No statistics were calculated in serial.
#if DEBUG_PARALLEL_CONTINGENCY_STATISTICS
  timer->Delete();
#endif //DEBUG_PARALLEL_CONTINGENCY_STATISTICS

    return;
    }

  // Make sure that parallel updates are needed, otherwise leave it at that.
  int np = this->Controller->GetNumberOfProcesses();
  if ( np < 2 )
    {
#if DEBUG_PARALLEL_CONTINGENCY_STATISTICS
  timer->Delete();
#endif //DEBUG_PARALLEL_CONTINGENCY_STATISTICS

    return;
    }

  // Get ready for parallel calculations
  vtkCommunicator* com = this->Controller->GetCommunicator();
  if ( ! com )
    {
    vtkErrorMacro("No parallel communicator.");
    }

  vtkIdType myRank = com->GetLocalProcessId();

  // Packing step: concatenate all (x,y) pairs in a single string and all (k,c) pairs in single vector
  vtkStdString xyPacked_l;
  std::vector<vtkIdType> kcValues_l;
  if ( StringArrayToStringBuffer( contingencyTab, xyPacked_l, kcValues_l ) )
    {
    vtkErrorMacro("Packing error on process "
                  << myRank
                  << ".");

    return;
    }

  // NB: Use process 0 as sole reducer for now
  vtkIdType rProc = 0;

  // (All) gather all xy and kc sizes
  vtkIdType xySize_l = xyPacked_l.size();
  vtkIdType* xySize_g = new vtkIdType[np];

  vtkIdType kcSize_l = kcValues_l.size();
  vtkIdType* kcSize_g = new vtkIdType[np];

  com->AllGather( &xySize_l,
                  xySize_g,
                  1 );

  com->AllGather( &kcSize_l,
                  kcSize_g,
                  1 );

  // Calculate total size and displacement arrays
  vtkIdType* xyOffset = new vtkIdType[np];
  vtkIdType* kcOffset = new vtkIdType[np];

  vtkIdType xySizeTotal = 0;
  vtkIdType kcSizeTotal = 0;

  for ( vtkIdType i = 0; i < np; ++ i )
    {
    xyOffset[i] = xySizeTotal;
    kcOffset[i] = kcSizeTotal;

    xySizeTotal += xySize_g[i];
    kcSizeTotal += kcSize_g[i];
    }

  // Allocate receive buffers on reducer process, based on the global sizes obtained above
  char* xyPacked_g = 0;
  vtkIdType* kcValues_g = 0;
  if ( myRank == rProc )
    {
    xyPacked_g = new char[xySizeTotal];
    kcValues_g = new vtkIdType[kcSizeTotal];
    }

  // Gather all xyPacked and kcValues on process rProc
  // NB: GatherV because the packets have variable lengths
  if ( ! com->GatherV( &(*xyPacked_l.begin()),
                       xyPacked_g,
                       xySize_l,
                       xySize_g,
                       xyOffset,
                       rProc ) )
    {
    vtkErrorMacro("Process "
                  << myRank
                  << "could not gather (x,y) values.");

    delete [] xyOffset;
    delete [] kcOffset;
    delete [] xyPacked_g;
    delete [] kcValues_g;
    return;
    }

  if ( ! com->GatherV( &(*kcValues_l.begin()),
                       kcValues_g,
                       kcSize_l,
                       kcSize_g,
                       kcOffset,
                       rProc ) )
    {
    vtkErrorMacro("Process "
                  << myRank
                  << "could not gather (k,c) values.");

    delete [] xyOffset;
    delete [] kcOffset;
    delete [] xyPacked_g;
    delete [] kcValues_g;
    return;
    }

  // Reduce to global contingency table on process rProc
  if ( myRank == rProc )
    {
    if ( this->Reduce( xySizeTotal,
                       xyPacked_g,
                       xyPacked_l,
                       kcSizeTotal,
                       kcValues_g,
                       kcValues_l ) )
      {
      delete [] xyOffset;
      delete [] kcOffset;
      delete [] xyPacked_g;
      delete [] kcValues_g;
      return;
      }
    } // if ( myRank == rProc )

#if DEBUG_PARALLEL_CONTINGENCY_STATISTICS
  vtkTimerLog *timerB=vtkTimerLog::New();
  timerB->StartTimer();
#endif //DEBUG_PARALLEL_CONTINGENCY_STATISTICS

  // Broadcasting step: broadcast reduced contingency table to all processes
  std::vector<vtkStdString> xyValues_l; // local consecutive xy pairs
  if ( this->Broadcast( xySizeTotal,
                        xyPacked_l,
                        xyValues_l,
                        kcSizeTotal,
                        kcValues_l,
                        rProc ) )
    {
    delete [] xyOffset;
    delete [] kcOffset;
    delete [] xyPacked_g;
    delete [] kcValues_g;
    return;
    }

#if DEBUG_PARALLEL_CONTINGENCY_STATISTICS
  timerB->StopTimer();

  cout << "## Process "
       << myRank
       << " broadcasted in "
       << timerB->GetElapsedTime()
       << " seconds."
       << "\n";

  timerB->Delete();
#endif //DEBUG_PARALLEL_CONTINGENCY_STATISTICS

  // Finally, fill the new, global contingency table (everyone does this so everyone ends up with the same model)
  vtkVariantArray* row4 = vtkVariantArray::New();
  row4->SetNumberOfValues( 4 );

  std::vector<vtkStdString>::iterator xyit = xyValues_l.begin();
  std::vector<vtkIdType>::iterator    kcit = kcValues_l.begin();

  // First replace existing rows
  // Start with row 1 and not 0 because of cardinality row (cf. superclass for a detailed explanation)
  for ( vtkIdType r = 1 ; r < nRowCont; ++ r, xyit += 2, kcit += 2 )
    {
    row4->SetValue( 0, *kcit );
    row4->SetValue( 1, *xyit );
    row4->SetValue( 2, *(xyit + 1) );
    row4->SetValue( 3, *(kcit + 1) );

    contingencyTab->SetRow( r, row4 );
    }

  // Then insert new rows
  for ( ; xyit != xyValues_l.end() ; xyit += 2, kcit += 2 )
    {
    row4->SetValue( 0, *kcit );
    row4->SetValue( 1, *xyit );
    row4->SetValue( 2, *(xyit + 1) );
    row4->SetValue( 3, *(kcit + 1) );

   contingencyTab->InsertNextRow( row4 );
   }

  // Clean up
  row4->Delete();

  delete [] xyPacked_g;
  delete [] kcValues_g;
  delete [] xySize_g;
  delete [] kcSize_g;
  delete [] xyOffset;
  delete [] kcOffset;

#if DEBUG_PARALLEL_CONTINGENCY_STATISTICS
  timer->StopTimer();

  cout << "## Process "
       << myRank
       << " parallel Learn took "
       << timer->GetElapsedTime()
       << " seconds."
       << "\n";

  timer->Delete();
#endif //DEBUG_PARALLEL_CONTINGENCY_STATISTICS
}

// ----------------------------------------------------------------------
bool vtkPContingencyStatistics::Reduce( vtkIdType& xySizeTotal,
                                        char* xyPacked_g,
                                        vtkStdString& xyPacked_l,
                                        vtkIdType& kcSizeTotal,
                                        vtkIdType*  kcValues_g,
                                        std::vector<vtkIdType>& kcValues_l )
{
  // First, unpack the packet of strings
  std::vector<vtkStdString> xyValues_g;
  StringBufferToStringVector( vtkStdString ( xyPacked_g, xySizeTotal ), xyValues_g );

  // Second, check consistency: we must have the same number of xy and kc entries
  if ( vtkIdType( xyValues_g.size() ) != kcSizeTotal )
    {
    vtkErrorMacro("Reduction error on process "
                  << this->Controller->GetCommunicator()->GetLocalProcessId()
                  << ": inconsistent number of (x,y) and (k,c) pairs: "
                  << xyValues_g.size()
                  << " <> "
                  << kcSizeTotal
                  << ".");

    return true;
    }

  // Third, reduce to the global contingency table
  typedef std::map<vtkStdString,vtkIdType> Distribution;
  typedef std::map<vtkStdString,Distribution> Bidistribution;
  std::map<vtkIdType,Bidistribution> contingencyTable;
  vtkIdType i = 0;
  for ( std::vector<vtkStdString>::iterator vit = xyValues_g.begin();
        vit != xyValues_g.end(); vit += 2, i += 2 )
    {
    contingencyTable
      [kcValues_g[i]]
      [*vit]
      [*(vit + 1)]
      += kcValues_g[i + 1];
    }

  // Fourth, prepare send buffers of (global) xy and kc values
  std::vector<vtkStdString> xyValues_l;
  kcValues_l.clear();
  for ( std::map<vtkIdType,Bidistribution>::iterator ait = contingencyTable.begin();
        ait != contingencyTable.end(); ++ ait )
    {
    Bidistribution bidi = ait->second;
    for ( Bidistribution::iterator bit = bidi.begin();
          bit != bidi.end(); ++ bit )
      {
      Distribution di = bit->second;
      for ( Distribution::iterator dit = di.begin();
            dit != di.end(); ++ dit )
        {
        // Push back x and y to list of strings
        xyValues_l.push_back( bit->first );  // x
        xyValues_l.push_back( dit->first );  // y

        // Push back (X,Y) index and #(x,y) to list of strings
        kcValues_l.push_back( ait->first );  // k
        kcValues_l.push_back( dit->second ); // c
        }
      }
    }
  StringVectorToStringBuffer( xyValues_l, xyPacked_l );

  // Last, update xy and kc buffer sizes (which have changed because of the reduction)
  xySizeTotal = xyPacked_l.size();
  kcSizeTotal = kcValues_l.size();

  return false;
}


// ----------------------------------------------------------------------
bool vtkPContingencyStatistics::Broadcast( vtkIdType xySizeTotal,
                                           vtkStdString& xyPacked,
                                           std::vector<vtkStdString>& xyValues,
                                           vtkIdType kcSizeTotal,
                                           std::vector<vtkIdType>& kcValues,
                                           vtkIdType rProc )
{
  vtkCommunicator* com = this->Controller->GetCommunicator();

  // Broadcast the xy and kc buffer sizes
  if ( ! com->Broadcast( &xySizeTotal,
                         1,
                         rProc ) )
    {
    vtkErrorMacro("Process "
                  << com->GetLocalProcessId()
                  << " could not broadcast (x,y) buffer size.");

    return true;
    }

  if ( ! com->Broadcast( &kcSizeTotal,
                         1,
                         rProc ) )
    {
    vtkErrorMacro("Process "
                  << com->GetLocalProcessId()
                  << " could not broadcast (k,c) buffer size.");

    return true;
    }

  // Resize vectors so they can receive the broadcasted xy and kc values
  xyPacked.resize( xySizeTotal );
  kcValues.resize( kcSizeTotal );

  // Broadcast the contents of contingency table to everyone
  if ( ! com->Broadcast( &(*xyPacked.begin()),
                         xySizeTotal,
                         rProc ) )
    {
    vtkErrorMacro("Process "
                  << com->GetLocalProcessId()
                  << " could not broadcast (x,y) values.");

    return true;
    }

  if ( ! com->Broadcast( &(*kcValues.begin()),
                         kcSizeTotal,
                         rProc ) )
    {
    vtkErrorMacro("Process "
                  << com->GetLocalProcessId()
                  << " could not broadcast (k,c) values.");

    return true;
    }

  // Unpack the packet of strings
  StringBufferToStringVector( xyPacked, xyValues );

  return false;
}
