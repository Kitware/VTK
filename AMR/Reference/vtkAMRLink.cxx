/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRLink.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

#include "vtkAMRLink.h"

//------------------------------------------------------------------------------
vtkAMRLink::vtkAMRLink()
{
  this->ProcessRank = -1;
  this->Level       = -1;
  this->BlockID     = -1;
}

//------------------------------------------------------------------------------
vtkAMRLink::vtkAMRLink(
    const int block, const int level, const int rank )
{
  this->ProcessRank = rank;
  this->BlockID     = block;
  this->Level       = level;
}

//------------------------------------------------------------------------------
vtkAMRLink::vtkAMRLink( const vtkAMRLink &lnk )
{
  *this = lnk;
}

//------------------------------------------------------------------------------
vtkAMRLink::~vtkAMRLink()
{

}

//------------------------------------------------------------------------------
vtkAMRLink& vtkAMRLink::operator=( const vtkAMRLink &rhs )
{
  if( this != &rhs )
    {
      this->ProcessRank = rhs.ProcessRank;
      this->BlockID     = rhs.BlockID;
      this->Level       = rhs.Level;
    }
  return( *this );
}

