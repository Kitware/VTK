/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkStructuredAMRNeighbor.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkStructuredAMRNeighbor.h"

vtkStructuredAMRNeighbor::vtkStructuredAMRNeighbor()
{
  this->GridLevel     = -1;
  this->NeighborLevel = -1;
  this->NeighborID  = 0;
  this->OverlapExtent[ 0 ]  = this->OverlapExtent[ 1 ]  =
  this->OverlapExtent[ 2 ]  = this->OverlapExtent[ 3 ]  =
  this->OverlapExtent[ 4 ]  = this->OverlapExtent[ 5 ]  = 0;

  this->Orientation[ 0 ]    = this->Orientation[ 1 ] =
  this->Orientation[ 2 ]    = vtkStructuredNeighbor::UNDEFINED;

  for( int i=0; i < 6; ++i )
    {
    this->SendExtent[i] = this->RcvExtent[i] = -1;
    }
}

//-----------------------------------------------------------------------------
vtkStructuredAMRNeighbor::vtkStructuredAMRNeighbor(
    const int GridLevel, const int NeiID, const int NeighborLevel,
    int overlap[6], int orient[3], int r[3])
{
  this->GridLevel     = GridLevel;
  this->NeighborID    = NeiID;
  this->NeighborLevel = NeighborLevel;
  for( int i=0; i < 3; ++i )
    {
    this->RcvExtent[i*2]         =
    this->SendExtent[i*2]        =
    this->OverlapExtent[ i*2 ]   = overlap[ i*2 ];

    this->RcvExtent[i*2+1]       =
    this->SendExtent[i*2+1]      =
    this->OverlapExtent[ i*2+1 ] = overlap[ i*2+1 ];

    this->Orientation[ i ]       = orient[ i ];
    this->RefinementRatio[ i ]   = r[i];
    }
}


//-----------------------------------------------------------------------------
vtkStructuredAMRNeighbor::~vtkStructuredAMRNeighbor()
{
  // TODO Auto-generated destructor stub
}

//-----------------------------------------------------------------------------
vtkStructuredAMRNeighbor& vtkStructuredAMRNeighbor::operator=(
      const vtkStructuredAMRNeighbor &N)
{
  if( this != &N )
    {
    this->Orientation[ 0 ] = N.Orientation[ 0 ];
    this->Orientation[ 1 ] = N.Orientation[ 1 ];
    this->Orientation[ 2 ] = N.Orientation[ 2 ];
    this->NeighborID       = N.NeighborID;
    this->NeighborLevel    = N.NeighborLevel;
    for( int i=0; i < 6; ++i )
      {
      this->SendExtent[ i ]    = N.SendExtent[ i ];
      this->RcvExtent[ i ]     = N.RcvExtent[ i ];
      this->OverlapExtent[ i ] = N.OverlapExtent[ i ];
      } // END for
    } // END if
  return *this;
}

//-----------------------------------------------------------------------------
void vtkStructuredAMRNeighbor::ComputeSendAndReceiveExtent(
    int gridRealExtent[6], int gridGhostedExtent[6], int neiRealExtent[6],
    int WholeExtent[6], const int N)
{
  // TODO: implement this
}
