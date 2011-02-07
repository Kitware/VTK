/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAMRInterBlockConnectivity.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAMRInterBlockConnectivity.h"
#include "vtkAMRGridIndexEncoder.h"
#include "vtkObjectFactory.h"
#include "vtkVector.h"
#include "vtkAssertUtils.hpp"

//
// Standard functions
//
vtkStandardNewMacro(vtkAMRInterBlockConnectivity);

//------------------------------------------------------------------------------
vtkAMRInterBlockConnectivity::vtkAMRInterBlockConnectivity()
{
  // TODO Auto-generated constructor stub
}

//------------------------------------------------------------------------------
vtkAMRInterBlockConnectivity::~vtkAMRInterBlockConnectivity()
{
  vtkstd::map<unsigned int,vtkstd::vector< vtkVector<int,3> > >::iterator iter;
  for(iter=this->connectivity.begin();iter != this->connectivity.end(); ++iter)
      iter->second.clear( );
  this->connectivity.clear( );
}

//------------------------------------------------------------------------------
void vtkAMRInterBlockConnectivity::PrintSelf(
    std::ostream &oss, vtkIndent indent )
{
  vtkstd::map<unsigned int,vtkstd::vector< vtkVector<int,3> > >::iterator iter;
  for(iter=this->connectivity.begin();iter != this->connectivity.end(); ++iter)
    {
      unsigned int idx = iter->first;
      int level        = -1;
      int block        = -1;
      vtkAMRGridIndexEncoder::decode( idx, level, block );
      vtkAssertUtils::assertTrue( (level>=0),__FILE__,__LINE__ );
      vtkAssertUtils::assertTrue( (block>=0),__FILE__,__LINE__ );

      oss << "Block: " << block << " Level: " << level << std::endl;
      oss << "=====================================================\n";

      for( int i=0; i < this->GetNumberOfConnections( idx ); ++i )
        {
          vtkVector<int,3> tuple = this->GetConnection( idx, i );
          oss <<  "(" << tuple[0] << ", " << tuple[1] << ", " << tuple[2];
          oss << ")\n";
        } // END for all connections of the block corresponding to (block,level)

    } // END for all connecting blocks (owned by this process)

}

//------------------------------------------------------------------------------
void vtkAMRInterBlockConnectivity::InsertConnection(
       const int myBlockId, const int myLevelId,
       const int connectingBlockIdx, const int connectingBlockLevel,
       const int connectingBlockProcess )
{
  unsigned int idx = vtkAMRGridIndexEncoder::encode( myLevelId, myBlockId );
  if( this->HasBlockConnections( idx ) )
    {
      this->connectivity[ idx ].push_back(
          this->GetTuple( connectingBlockIdx, connectingBlockLevel,
                          connectingBlockProcess )
                          );
    }
  else
    {
      vtkstd::vector< vtkVector<int,3> > myVector;
      myVector.push_back(
          this->GetTuple( connectingBlockIdx, connectingBlockLevel,
                          connectingBlockProcess )
          );
      this->connectivity[ idx ] = myVector;
    }

}

//------------------------------------------------------------------------------
int vtkAMRInterBlockConnectivity::GetNumberOfConnections(
                      const int myBlockId, const int myLevelId )
{
  unsigned int idx = vtkAMRGridIndexEncoder::encode( myLevelId, myBlockId );
  return( this->GetNumberOfConnections( idx ) );
}

//------------------------------------------------------------------------------
int vtkAMRInterBlockConnectivity::GetNumberOfConnections(const unsigned int idx)
{
  if( this->HasBlockConnections( idx ) )
    return this->connectivity[ idx ].size( );
  return 0;
}

//------------------------------------------------------------------------------
bool vtkAMRInterBlockConnectivity::HasBlockConnections(
    const int myBlockId, const int myLevelId )
{
  unsigned int idx = vtkAMRGridIndexEncoder::encode( myLevelId, myBlockId );
  if( this->HasBlockConnections( idx ) )
    return true;
  return false;
}

//------------------------------------------------------------------------------
bool vtkAMRInterBlockConnectivity::HasBlockConnections( const unsigned int idx )
{
  if( this->connectivity.find( idx ) != this->connectivity.end() )
    return true;
  return false;
}

//------------------------------------------------------------------------------
vtkVector<int,3> vtkAMRInterBlockConnectivity::GetConnection(
    const int myBlockId, const int myLevelId, const int idx )
{
  unsigned int blockidx=vtkAMRGridIndexEncoder::encode( myLevelId, myBlockId );
  return( this->GetConnection(blockidx,idx));
}

//------------------------------------------------------------------------------
vtkVector<int,3> vtkAMRInterBlockConnectivity::GetConnection(
                                    const unsigned int blockidx, const int idx )
{
  if( !this->HasBlockConnections( blockidx ) )
    {
      vtkErrorMacro(
        "Attempting to query inter-block connectivity for a block" <<
        " that does not have any connections!");
    }
  if( idx >= this->GetNumberOfConnections( blockidx ) )
    {
      vtkErrorMacro( "Connection index out-of-bounds" );
    }

  return( this->connectivity[blockidx][idx] );

}

//------------------------------------------------------------------------------
vtkVector<int,3> vtkAMRInterBlockConnectivity::GetTuple(
                const int block, const int level, const int rank)
{
  vtkVector<int,3> tuple;
  tuple[0] = block;
  tuple[1] = level;
  tuple[2] = rank;
  return( tuple );
}



