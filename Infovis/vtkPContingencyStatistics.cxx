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
#include "vtkToolkits.h"

#include "vtkPContingencyStatistics.h"
#include "vtkBivariateStatisticsAlgorithmPrivate.h"

#include "vtkCommunicator.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkMultiProcessController.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkVariant.h"

#include <vtkstd/map>
#include <vtkstd/set>
#include <vtkstd/vector>

vtkStandardNewMacro(vtkPContingencyStatistics);
vtkCxxRevisionMacro(vtkPContingencyStatistics, "1.4");
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
void PackValues( const vtkstd::vector<vtkStdString>& values, 
                 vtkStdString& buffer )
{
  for( vtkstd::vector<vtkStdString>::const_iterator it = values.begin();
       it != values.end(); ++ it )
    {
    buffer.append( *it );
    buffer.push_back( 0 );
    }
}

//-----------------------------------------------------------------------------
void UnpackValues( const vtkStdString& buffer,
                   vtkstd::vector<vtkStdString>& values )
{
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
void vtkPContingencyStatistics::ExecuteLearn( vtkTable* inData,
                                              vtkDataObject* outMetaDO )
{
  vtkMultiBlockDataSet* outMeta = vtkMultiBlockDataSet::SafeDownCast( outMetaDO );
  if ( ! outMeta )
    {
    return;
    }

  // First calculate contingency statistics on local data set
  this->Superclass::ExecuteLearn( inData, outMeta );

  // Get a hold of the contingency table
  vtkTable* contingencyTab = vtkTable::SafeDownCast( outMeta->GetBlock( 1 ) );
  if ( ! contingencyTab )
    {
    return;
    }

  vtkIdType nRowCont = contingencyTab->GetNumberOfRows();
  if ( nRowCont <= 0 )
    {
    // No statistics were calculated in serial.
    return;
    }

  // Make sure that parallel updates are needed, otherwise leave it at that.
  int np = this->Controller->GetNumberOfProcesses();
  if ( np < 2 )
    {
    return;
    }

  // Downcast meta columns to string arrays for efficient data access
  vtkIdTypeArray* keys = vtkIdTypeArray::SafeDownCast( contingencyTab->GetColumnByName( "Key" ) );
  vtkStringArray* valx = vtkStringArray::SafeDownCast( contingencyTab->GetColumnByName( "x" ) );
  vtkStringArray* valy = vtkStringArray::SafeDownCast( contingencyTab->GetColumnByName( "y" ) );
  vtkIdTypeArray* card = vtkIdTypeArray::SafeDownCast( contingencyTab->GetColumnByName( "Cardinality" ) );
  if ( ! keys || ! valx || ! valy || ! card )
    {
    return;
    }

  vtkstd::vector<vtkStdString> xyValues_l; // consecutive values of ( x, y ) pairs
  vtkstd::vector<int>          kcValues_l; // consecutive values of ( (X,Y) indices, #(x,y) ) pairs

  for ( int r = 1; r < nRowCont; ++ r ) // Skip first row which contains data set cardinality
    {
    // Push back x and y to list of strings
    xyValues_l.push_back( valx->GetValue( r ) );
    xyValues_l.push_back( valy->GetValue( r ) );

    // Push back (X,Y) index and #(x,y) to list of strings
    kcValues_l.push_back( keys->GetValue( r ) );
    kcValues_l.push_back( card->GetValue( r ) );
    }

  // Now concatenate vector of strings into single string
  vtkStdString xyPacked_l;
  PackValues( xyValues_l, xyPacked_l );

  // Now get ready for parallel calculations
  vtkCommunicator* com = this->Controller->GetCommunicator();

  // Use process 0 as sole reducer for now
  int reduceProc = 0; 

  // (All) gather all xy and kc sizes on process reduceProc
  int xySize_l = xyPacked_l.size();
  int* xySize_g = new int[np];
  com->AllGather( &xySize_l,
                  xySize_g,
                  1 );

  int kcSize_l = kcValues_l.size();
  int* kcSize_g = new int[np];
  com->AllGather( &kcSize_l,
                  kcSize_g,
                  1 );

  // Calculate total size, receive counts, and displacement array
  int xySizeTotal = 0;
  int kcSizeTotal = 0;
  for ( int i = 0; i < np; ++ i )
    {
    xySizeTotal += xySize_g[i];
    kcSizeTotal += kcSize_g[i];
    }

  int myRank = com->GetLocalProcessId();
  // Allocate receive buffers on reducer process, based on the global sizes obtained above
  char* xyPacked_g;
  int*  kcValues_g;
  if ( myRank == reduceProc )
    {
    xyPacked_g = new char[xySizeTotal];
    kcValues_g = new  int[kcSizeTotal];
    }
  
  // Now gather all xyPacked and kcValues on process reduceProc
//   if ( ! com->Gather( &(xyPacked_l[0]),
//                       xyPacked_g,
//                       xySize_l,
//                       reduceProc ) )
//     {
//     vtkErrorMacro("Process "<<myRank<< "could not gather (x,y) values.");
//     return;
//     }
  
//   if ( ! com->Gather( &(kcValues_l[0]),
//                       kcValues_g,
//                       kcSize_l,
//                       reduceProc ) )
//     {
//     vtkErrorMacro("Process "<<myRank<< "could not gather (x,y) indices and cardinalities.");
//     return;
//     }
  
  // Clean up
  if ( myRank == reduceProc )
    {
    delete [] xyPacked_g;
    delete [] kcValues_g;
    }

  delete [] xySize_g;
  delete [] kcSize_g;
}


