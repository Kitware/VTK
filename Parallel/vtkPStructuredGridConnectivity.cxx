/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkPStructuredGridConnectivity.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkPStructuredGridConnectivity.h"
#include "vtkObjectFactory.h"
#include "vtkMultiProcessController.h"

#include <cassert>

vtkStandardNewMacro(vtkPStructuredGridConnectivity);

//------------------------------------------------------------------------------
vtkPStructuredGridConnectivity::vtkPStructuredGridConnectivity()
{
  this->Controller       = vtkMultiProcessController::GetGlobalController();
  this->Rank             = this->Controller->GetLocalProcessId();
  this->GridConnectivity = vtkStructuredGridConnectivity::New();
}

//------------------------------------------------------------------------------
vtkPStructuredGridConnectivity::~vtkPStructuredGridConnectivity()
{
  if( this->GridConnectivity != NULL )
    this->GridConnectivity->Delete();
}

//------------------------------------------------------------------------------
void vtkPStructuredGridConnectivity::PrintSelf(
    std::ostream& os,vtkIndent indent)
{
  this->Superclass::PrintSelf( os, indent );
}

//------------------------------------------------------------------------------
void vtkPStructuredGridConnectivity::SetNumberOfGrids( const int N )
{
  this->Superclass::SetNumberOfGrids( N );
  this->GridRanks.resize( N,-1 );
}

//------------------------------------------------------------------------------
void vtkPStructuredGridConnectivity::RegisterGrid(
    const int gridID, int extents[6] )
{
  assert( "pre: gridID out-of-bounds!" &&
          (gridID >= 0  && gridID < this->NumberOfGrids) );

  this->Superclass::RegisterGrid( gridID, extents );
  this->GridIds.push_back( gridID );
  this->GridRanks[ gridID ] = this->Rank;
}

//------------------------------------------------------------------------------
int vtkPStructuredGridConnectivity::GetGridRank( const int gridID )
{
  assert( "pre: gridID out-of-bounds!" &&
          (gridID >= 0  && gridID < this->NumberOfGrids) );
  return( this->GridRanks[ gridID ] );
}

//------------------------------------------------------------------------------
void vtkPStructuredGridConnectivity::ComputeNeighbors()
{
  assert( "pre: Null multi-process controller" && (this->Controller != NULL) );

  this->ExchangeGridExtents();
  this->Controller->Barrier();

  this->Superclass::ComputeNeighbors();
  this->Controller->Barrier();
}

//------------------------------------------------------------------------------
void vtkPStructuredGridConnectivity::ExchangeGridExtents()
{
  // TODO: implement this
}

//------------------------------------------------------------------------------
void vtkPStructuredGridConnectivity::SerializeData( int *&sndbuffer, int &N )
{
  assert( "pre: send buffer is expected ot be NULL" && sndbuffer == NULL );
  // TODO: implement this
}

//------------------------------------------------------------------------------
void vtkPStructuredGridConnectivity::DeserializeData( int *rcvbuffer, int &N )
{
  assert( "pre: rcvbuffer should not be NULL" && (rcvbuffer != NULL) );
  // TODO: implement this
}
