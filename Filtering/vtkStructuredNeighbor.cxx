/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkStructuredNeighbor.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkStructuredNeighbor.h"

vtkStructuredNeighbor::vtkStructuredNeighbor()
{
  this->NeighborID  = 0;
  this->OverlapExtent[ 0 ]  = this->OverlapExtent[ 1 ]  =
  this->OverlapExtent[ 2 ]  = this->OverlapExtent[ 3 ]  =
  this->OverlapExtent[ 4 ]  = this->OverlapExtent[ 5 ]  = 0;
}

//------------------------------------------------------------------------------
vtkStructuredNeighbor::vtkStructuredNeighbor(
    const int neiId, int overlap[6] )
{
  this->NeighborID = neiId;
  for( int i=0; i < 6; ++i )
    this->OverlapExtent[ i ] = overlap[ i ];
}


//------------------------------------------------------------------------------
vtkStructuredNeighbor::~vtkStructuredNeighbor()
{
  // TODO Auto-generated destructor stub
}
