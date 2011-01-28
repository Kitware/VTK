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
#include "vtkAssertUtils.hpp"

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
vtkAMRGridIndexEncoder::encode( const int level, const int blockidx )
{
  vtkAssertUtils::assertInRange( level, 0, 65535, __FILE__, __LINE__ );
  vtkAssertUtils::assertInRange( blockidx, 0, 65535, __FILE__, __LINE__ );

  unsigned int encodeIdx;
  encodeIdx = ( level << 16 ) | blockidx;
  return( encodeIdx );
}

//-----------------------------------------------------------------------------

void
vtkAMRGridIndexEncoder::decode(
    const unsigned int gridIdx, int &level, int &blockIdx )
{
  level    = ( gridIdx >> 16 );
  blockIdx = ( ( gridIdx << 16 ) >> 16 );
  vtkAssertUtils::assertInRange( level, 0, 65535, __FILE__, __LINE__ );
  vtkAssertUtils::assertInRange( blockIdx, 0, 65535, __FILE__, __LINE__ );
}
