/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkExtentRCBPartitioner.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

#include "vtkExtentRCBPartitioner.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"

#include <cmath>
#include <cassert>
#include <algorithm>
#include <vtkstd/queue>

vtkStandardNewMacro( vtkExtentRCBPartitioner );

//------------------------------------------------------------------------------
vtkExtentRCBPartitioner::vtkExtentRCBPartitioner()
{
  this->NumExtents           = 0;
  this->NumberOfSubdivisions = 0;
  for( int i=0; i < 3; ++i )
    {
      this->GlobalExtent[ i ]   = 0;
      this->GlobalExtent[ i+3 ] = 1;
    }
}

//------------------------------------------------------------------------------
vtkExtentRCBPartitioner::~vtkExtentRCBPartitioner()
{
  this->pextents.clear();
}

//------------------------------------------------------------------------------
void vtkExtentRCBPartitioner::PrintSelf( std::ostream &oss, vtkIndent indent )
{
  this->Superclass::PrintSelf( oss, indent );
}

//------------------------------------------------------------------------------
void vtkExtentRCBPartitioner::Partition()
{
  this->pextents.clear();
  this->pextents.resize( this->GetNumberOfTotalExtents()*6 );

  this->AddExtent( 0, this->GlobalExtent );
  assert( "pre: Unexpected number of extents:" && (this->NumExtents==1) );

  int ex[6]; // temporary buffer to store the current extent
  int s1[6]; // temporary buffer to store the sub-extent s1
  int s2[6]; // temporary buffer to store the sub-extent s2

  for( int i=0; i < this->GetNumberOfSubdivisions(); ++i )
    {
      int N = this->NumExtents;
      for( int e=0; e < N; ++e )
        {
          this->GetExtent( e, ex );
          int ldim = this->GetLongestDimension( ex );

          this->SplitExtent( ex, s1, s2, ldim );
          this->ReplaceExtent( e, s1 );
          this->AddExtent( this->NumExtents, s2 );
        } // END for all extents
    } // END for all subdivisions

}

//------------------------------------------------------------------------------
void vtkExtentRCBPartitioner::GetExtent( const int idx, int ext[6] )
{
  int N = this->GetNumberOfTotalExtents();
  assert( "pre: invalide pextents size" && ( (this->pextents.size()/6)==N) );
  assert( "pre: idx is out-of-bounds" && ( (idx >= 0) && (idx < N) ) );

  for( int i=0; i < 6; ++i )
    ext[ i ] = this->pextents[ idx*6+i ];
}

//------------------------------------------------------------------------------
void vtkExtentRCBPartitioner::AddExtent( const int idx, int ext[6] )
{
  int N = this->GetNumberOfTotalExtents();
  assert( "pre: invalide pextents size" && ( (this->pextents.size()/6)==N) );
  assert( "pre: idx is out-of-bounds" && ( (idx >= 0) && (idx < N) ) );

  for( int i=0; i < 6; ++i )
    this->pextents[ idx*6+i ] = ext[ i ];
  ++this->NumExtents;
}

//------------------------------------------------------------------------------
void vtkExtentRCBPartitioner::ReplaceExtent(const int idx, int ext[6] )
{
  int N = this->GetNumberOfTotalExtents();
  assert( "pre: invalide pextents size" && ( (this->pextents.size()/6)==N) );
  assert( "pre: idx is out-of-bounds" && ( (idx >= 0) && (idx < N) ) );

  for( int i=0; i < 6; ++i )
    this->pextents[ idx*6+i ] = ext[ i ];
}

//------------------------------------------------------------------------------
void vtkExtentRCBPartitioner::GetPartitionExtent( const int idx, int ext[6] )
{
  int N = this->GetNumberOfTotalExtents();
  assert( "pre: invalide pextents size" && ( (this->pextents.size()/6)==N) );
  assert( "pre: idx is out-of-bounds" && ( (idx >= 0) && (idx < N) ) );

  for( int i=0; i < 6; ++i )
    ext[i] = this->pextents[idx*6+i];
}

//------------------------------------------------------------------------------
int vtkExtentRCBPartitioner::GetNumberOfTotalExtents()
{
  int N = std::pow( 2,this->NumberOfSubdivisions );
  assert( "pre: N >= 1" && (N >= 1) );
  return N;
}

//------------------------------------------------------------------------------
void vtkExtentRCBPartitioner::SplitExtent(
    int parent[6], int s1[6], int s2[6], int dimension )
{
  int numNodes = 0;
  int mid      = -1;
  int minIdx   = -1;
  int maxIdx   = -1;

  for( int i=0; i < 6; ++i )
    s1[ i ] = s2[ i ] = parent[ i ];

  switch( dimension )
    {
      case 1:
        minIdx = 0;
        maxIdx = 3;
        break;
      case 2:
        minIdx = 1;
        maxIdx = 4;
        break;
      case 3:
        minIdx = 2;
        maxIdx = 5;
        break;
      default:
        vtkErrorMacro( "Cannot split extent: Undefined split dimension!");
    }

  numNodes      = (parent[maxIdx]-parent[minIdx]) + 1;
  mid           = (int)vtkMath::Floor(0.5*numNodes);
  s1[ maxIdx ]  = (mid < s1[minIdx])? (s1[minIdx]+mid) : mid;
  s2[ minIdx ]  = (mid < s1[minIdx])? (s1[minIdx]+mid) : mid;

//  this->PrintExtent( "Parent", parent );
//  this->PrintExtent( "s1", s1 );
//  this->PrintExtent( "s2", s2 );

}

//------------------------------------------------------------------------------
int vtkExtentRCBPartitioner::GetLongestDimension( int ext[6] )
{
  int ilength = (ext[3]-ext[0])+1;
  int jlength = (ext[4]-ext[1])+1;
  int klength = (ext[5]-ext[2])+1;

  if ((ilength >= jlength) && (ilength >= klength))
    return 1;
  else if ((jlength >= ilength) && (jlength >= klength))
     return 2;
  else if ((klength >= ilength) && (klength >= jlength))
     return 3;
  assert( "pre: could not find longest dimension" && false );
  return 0;
}

//------------------------------------------------------------------------------
void vtkExtentRCBPartitioner::PrintExtent( std::string name, int ext[6] )
{
  std::cout << name << ": [";
  for( int i=0; i < 6; std::cout << ext[i++] << " " );
  std::cout << "]\n";
  std::cout.flush();
}
