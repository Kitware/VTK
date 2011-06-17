/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAMRGridIndexEncoder.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAMRGridIndexEncoder.h"

#include <cassert>

vtkAMRGridIndexEncoder::vtkAMRGridIndexEncoder()
{
  // TODO Auto-generated constructor stub
}

//-----------------------------------------------------------------------------

vtkAMRGridIndexEncoder::~vtkAMRGridIndexEncoder()
{
  // TODO Auto-generated destructor stub
}

//-----------------------------------------------------------------------------
unsigned int
vtkAMRGridIndexEncoder::Encode( const int level, const int blockidx )
{
  assert( "pre: 0 <= level <= 65535" && ( 0 <= level && level <= 65535) );
  assert( "pre: 0 <= blockIdx <= 65535" &&
      ( 0 <= blockidx && blockidx <= 65535) );

  unsigned int encodeIdx;
  encodeIdx = ( level << 16 ) | blockidx;
  return( encodeIdx );
}

//-----------------------------------------------------------------------------

void
vtkAMRGridIndexEncoder::Decode(
    const unsigned int gridIdx, int &level, int &blockIdx )
{
  level    = ( gridIdx >> 16 );
  blockIdx = ( ( gridIdx << 16 ) >> 16 );
  assert( "pre: 0 <= level <= 65535" && ( 0 <= level && level <= 65535) );
  assert( "pre: 0 <= blockIdx <= 65535" &&
      ( 0 <= blockIdx && blockIdx <= 65535) );
}
