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
#include "vtkAMRLink.h"
#include "vtkAssertUtils.hpp"
#include "vtkUnsignedIntArray.h"

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
  vtkstd::map<unsigned int,vtkstd::vector< vtkAMRLink > >::iterator iter;
  for(iter=this->connectivity.begin();iter != this->connectivity.end(); ++iter)
      iter->second.clear( );
  this->connectivity.clear( );
}

//------------------------------------------------------------------------------
void vtkAMRInterBlockConnectivity::PrintSelf(
    std::ostream &oss, vtkIndent indent )
{
  vtkstd::map<unsigned int,vtkstd::vector< vtkAMRLink > >::iterator iter;
  for(iter=this->connectivity.begin();iter != this->connectivity.end(); ++iter)
    {
      unsigned int idx = iter->first;
      int level        = -1;
      int block        = -1;
      vtkAMRGridIndexEncoder::Decode( idx, level, block );
      vtkAssertUtils::assertTrue( (level>=0),__FILE__,__LINE__ );
      vtkAssertUtils::assertTrue( (block>=0),__FILE__,__LINE__ );

      oss << "Block: " << block << " Level: " << level << std::endl;
      oss << "=====================================================\n";

      for( int i=0; i < this->GetNumberOfConnections( idx ); ++i )
        {
          vtkAMRLink tuple = this->GetConnection( idx, i );
          oss <<  "( BlockID:" << tuple.GetBlockID() << ", Level:";
          oss << tuple.GetLevel( ) << ", Rank:";
          oss << tuple.GetProcessRank( );
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
  unsigned int idx = vtkAMRGridIndexEncoder::Encode( myLevelId, myBlockId );
  if( this->HasBlockConnections( idx ) )
    {
      this->connectivity[ idx ].push_back(
          this->GetTuple( connectingBlockIdx, connectingBlockLevel,
                          connectingBlockProcess )
                          );
    }
  else
    {
      vtkstd::vector< vtkAMRLink > myVector;
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
  unsigned int idx = vtkAMRGridIndexEncoder::Encode( myLevelId, myBlockId );
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
  unsigned int idx = vtkAMRGridIndexEncoder::Encode( myLevelId, myBlockId );
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
bool vtkAMRInterBlockConnectivity::HasConnections()
{
  if( this->connectivity.size() > 0 )
    return true;
  return false;
}

//------------------------------------------------------------------------------
vtkUnsignedIntArray* vtkAMRInterBlockConnectivity::GetEncodedGridKeys()
{
  vtkUnsignedIntArray *myArray = vtkUnsignedIntArray::New();
  vtkstd::map<unsigned int, vtkstd::vector< vtkAMRLink > >::iterator iter;
  int idx = 0;
  for(iter=this->connectivity.begin();iter != this->connectivity.end();++iter)
    {
      myArray->InsertValue( idx, iter->first );
      ++idx;
    } // END for all connections
  return( myArray );
}

//------------------------------------------------------------------------------
vtkAMRLink vtkAMRInterBlockConnectivity::GetConnection(
    const int myBlockId, const int myLevelId, const int idx )
{
  unsigned int blockidx=vtkAMRGridIndexEncoder::Encode( myLevelId, myBlockId );
  return( this->GetConnection(blockidx,idx));
}

//------------------------------------------------------------------------------
vtkAMRLink vtkAMRInterBlockConnectivity::GetConnection(
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
vtkAMRLink vtkAMRInterBlockConnectivity::GetTuple(
                const int block, const int level, const int rank)
{
  vtkAMRLink link;
  link.SetProcessRank( rank );
  link.SetBlockID( block );
  link.SetLevel( level );
  return( link );
}
