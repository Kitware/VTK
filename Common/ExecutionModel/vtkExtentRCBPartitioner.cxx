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
#include "vtkPriorityQueue.h"
#include "vtkStructuredData.h"

#include <cmath>
#include <cassert>
#include <algorithm>

vtkStandardNewMacro( vtkExtentRCBPartitioner );

//------------------------------------------------------------------------------
vtkExtentRCBPartitioner::vtkExtentRCBPartitioner()
{
  this->NumberOfGhostLayers  = 0;
  this->NumExtents           = 0;
  this->NumberOfPartitions   = 2;
  this->DuplicateNodes       = 1;
  this->ExtentIsPartitioned  = false;
  this->DataDescription      = VTK_EMPTY;
  for( int i=0; i < 3; ++i )
    {
    this->GlobalExtent[ i*2   ] = 0;
    this->GlobalExtent[ i*2+1 ] = 0;
    }
}

//------------------------------------------------------------------------------
vtkExtentRCBPartitioner::~vtkExtentRCBPartitioner()
{
  this->PartitionExtents.clear();
}

//------------------------------------------------------------------------------
void vtkExtentRCBPartitioner::PrintSelf( std::ostream &oss, vtkIndent indent )
{
  this->Superclass::PrintSelf( oss, indent );
  oss << "Number of partitions: " << this->NumberOfPartitions << endl;
  oss << "Number of extents: " << this->NumExtents << endl;
  oss << "Number of ghost layers: " << this->NumberOfGhostLayers << endl;
  oss << "Global Extent: ";
  for( int i=0; i < 6; ++i )
    {
    oss << this->GlobalExtent[ i ] << " ";
    }
  oss << endl;
}

//------------------------------------------------------------------------------
void vtkExtentRCBPartitioner::Partition()
{
  // Short-circuit here since the given global extent has already been
  // partitioned
  if( this->ExtentIsPartitioned )
    {
    return;
    }


  // STEP 0: Get the data description according to the given global extent
  this->AcquireDataDescription();
  if( this->DataDescription == VTK_EMPTY ||
      this->DataDescription == VTK_SINGLE_POINT  )
    {
    return;
    }

  // STEP 1: Insert the global extent to the workQueue
  vtkPriorityQueue *wrkQueue = vtkPriorityQueue::New();
  assert( "pre: work queue is NULL" && (wrkQueue != NULL) );

  this->AddExtent( this->GlobalExtent );
  wrkQueue->Insert( this->GetNumberOfNodes( this->GlobalExtent), 0);

  int ex[6]; // temporary buffer to store the current extent
  int s1[6]; // temporary buffer to store the sub-extent s1
  int s2[6]; // temporary buffer to store the sub-extent s2

  // STEP 2: Loop until number of partitions is attained
  while( this->NumExtents < this->NumberOfPartitions )
    {
    vtkIdType extentIdx = wrkQueue->Pop(wrkQueue->GetNumberOfItems()-1);
    this->GetExtent( extentIdx, ex );
    int ldim = this->GetLongestDimension( ex );

    this->SplitExtent( ex, s1, s2, ldim );
    this->ReplaceExtent(extentIdx, s1);
    this->AddExtent(s2);

    wrkQueue->Insert( this->GetNumberOfNodes( s1 ),extentIdx );
    wrkQueue->Insert( this->GetNumberOfNodes( s2 ),this->NumExtents-1);
    }

  // STEP 3: Clear priority data-structures
  wrkQueue->Delete();

  // STEP 4: Loop through all the extents and add ghost layers
  if( this->NumberOfGhostLayers > 0 )
    {
    int ext[6];
    for( int i=0; i < this->NumExtents; ++i )
      {
      this->GetExtent( i, ext );
      this->ExtendGhostLayers( ext );
      this->ReplaceExtent( i, ext );
      } // END for all extents
    }

  // STEP 5: Set the flag to indicate that the extent has been partitioned to
  // the requested number of partitions. The only way this method will reexecute
  // is when the user calls SetGlobalExtent or SetNumberOfPartitions
  this->ExtentIsPartitioned = true;

  assert("post: number of extents must be equal to the number of partitions" &&
         (this->NumExtents == this->NumberOfPartitions) );
}

//------------------------------------------------------------------------------
void vtkExtentRCBPartitioner::GetExtent( const int idx, int ext[6] )
{
  // Sanity check
  assert( "pre: idx is out-of-bounds" &&
           ( (idx >=0) && (idx < this->NumExtents) ) );

  for( int i=0; i < 6; ++i )
    {
    ext[ i ] = this->PartitionExtents[ idx*6+i ];
    }
}

//------------------------------------------------------------------------------
void vtkExtentRCBPartitioner::AddExtent( int ext[6] )
{
  for( int i=0; i < 6; ++i )
    {
    this->PartitionExtents.push_back( ext[ i ] );
    }
  ++this->NumExtents;
}

//------------------------------------------------------------------------------
void vtkExtentRCBPartitioner::ReplaceExtent(const int idx, int ext[6] )
{
  // Sanity check
  assert( "pre: idx is out-of-bounds" &&
           ( (idx >=0) && (idx < this->NumExtents) ) );

  for( int i=0; i < 6; ++i )
    {
    this->PartitionExtents[ idx*6+i ] = ext[ i ];
    }
}

//------------------------------------------------------------------------------
void vtkExtentRCBPartitioner::GetPartitionExtent( const int idx, int ext[6] )
{
  // Sanity check
  assert( "pre: idx is out-of-bounds" &&
           ( (idx >=0) && (idx < this->NumExtents) ) );

  for( int i=0; i < 6; ++i )
    {
    ext[i] = this->PartitionExtents[idx*6+i];
    }
}

//------------------------------------------------------------------------------
int vtkExtentRCBPartitioner::GetNumberOfTotalExtents()
{
  return( this->NumExtents );
}

//------------------------------------------------------------------------------
void vtkExtentRCBPartitioner::SplitExtent(
    int parent[6], int s1[6], int s2[6], int splitDimension )
{
  int numNodes = 0;
  int mid      = -1;
  int minIdx   = -1;
  int maxIdx   = -1;

  for( int i=0; i < 6; ++i )
    {
    s1[ i ] = s2[ i ] = parent[ i ];
    }

  switch( splitDimension )
    {
    case 1:
      minIdx = 0;
      maxIdx = 1;
      break;
    case 2:
      minIdx = 2;
      maxIdx = 3;
      break;
    case 3:
      minIdx = 4;
      maxIdx = 5;
      break;
    default:
      vtkErrorMacro( "Cannot split extent: Undefined split dimension!");
    }

  numNodes      = (parent[maxIdx]-parent[minIdx]) + 1;
  mid           = (int)vtkMath::Floor(0.5*numNodes);

  if( this->DuplicateNodes == 1 )
    {
    s1[ maxIdx ]  = (mid < s1[minIdx])? (s1[minIdx]+mid) : mid;
    s2[ minIdx ]  = (mid < s1[minIdx])? (s1[minIdx]+mid) : mid;
    }
  else
    {
    s1[ maxIdx ]  = (mid < s1[minIdx])? (s1[minIdx]+mid) : mid;
    s2[ minIdx ]  = (mid < s1[minIdx])? (s1[minIdx]+mid)+1 : mid+1;
    }

//  this->PrintExtent( "Parent", parent );
//  this->PrintExtent( "s1", s1 );
//  this->PrintExtent( "s2", s2 );

}

//------------------------------------------------------------------------------
void vtkExtentRCBPartitioner::ExtendGhostLayers( int ext[6] )
{
  if( this->NumberOfGhostLayers == 0 )
    {
    return;
    }

  switch( this->DataDescription )
    {
    case VTK_X_LINE:
      this->GetGhostedExtent( ext, 0, 1 );
      break;
    case VTK_Y_LINE:
      this->GetGhostedExtent( ext, 2, 3 );
      break;
    case VTK_Z_LINE:
      this->GetGhostedExtent( ext, 4, 5 );
      break;
    case VTK_XY_PLANE:
      this->GetGhostedExtent( ext, 0, 1 );
      this->GetGhostedExtent( ext, 2, 3 );
      break;
    case VTK_YZ_PLANE:
      this->GetGhostedExtent( ext, 2, 3 );
      this->GetGhostedExtent( ext, 4, 5 );
      break;
    case VTK_XZ_PLANE:
      this->GetGhostedExtent( ext, 0, 1 );
      this->GetGhostedExtent( ext, 4, 5 );
      break;
    case VTK_XYZ_GRID:
      this->GetGhostedExtent( ext, 0, 1 );
      this->GetGhostedExtent( ext, 2, 3 );
      this->GetGhostedExtent( ext, 4, 5 );
      break;
    default:
      assert("pre: unsupported data-description, code should not reach here!"&&
             false );
    }
}

//------------------------------------------------------------------------------
int vtkExtentRCBPartitioner::GetNumberOfNodes( int ext[6] )
{
  int ilength,jlength,klength;
  int numNodes = 0;
  switch( this->DataDescription )
    {
    case VTK_X_LINE:
      numNodes = ilength = (ext[1]-ext[0])+1;
      break;
    case VTK_Y_LINE:
      numNodes = jlength = (ext[3]-ext[2])+1;
      break;
    case VTK_Z_LINE:
      numNodes = klength = (ext[5]-ext[4])+1;
      break;
    case VTK_XY_PLANE:
      ilength  = (ext[1]-ext[0])+1;
      jlength  = (ext[3]-ext[2])+1;
      numNodes = ilength*jlength;
      break;
    case VTK_YZ_PLANE:
      jlength  = (ext[3]-ext[2])+1;
      klength  = (ext[5]-ext[4])+1;
      numNodes = jlength*klength;
      break;
    case VTK_XZ_PLANE:
      ilength  = (ext[1]-ext[0])+1;
      klength  = (ext[5]-ext[4])+1;
      numNodes = ilength*klength;
      break;
    case VTK_XYZ_GRID:
      ilength  = (ext[1]-ext[0])+1;
      jlength  = (ext[3]-ext[2])+1;
      klength  = (ext[5]-ext[4])+1;
      numNodes = ilength*jlength*klength;
      break;
    default:
      assert("pre: unsupported data-description, code should not reach here!"&&
             false );
    }
  return( numNodes );
}
//------------------------------------------------------------------------------
int vtkExtentRCBPartitioner::GetNumberOfCells( int ext[6] )
{
  int ilength,jlength,klength;
  int numNodes = 0;
  switch( this->DataDescription )
    {
    case VTK_X_LINE:
      numNodes = ilength = (ext[1]-ext[0]);
      break;
    case VTK_Y_LINE:
      numNodes = jlength = (ext[3]-ext[2]);
      break;
    case VTK_Z_LINE:
      numNodes = klength = (ext[5]-ext[4]);
      break;
    case VTK_XY_PLANE:
      ilength  = (ext[1]-ext[0]);
      jlength  = (ext[3]-ext[2]);
      numNodes = ilength*jlength;
      break;
    case VTK_YZ_PLANE:
      jlength  = (ext[3]-ext[2]);
      klength  = (ext[5]-ext[4]);
      numNodes = jlength*klength;
      break;
    case VTK_XZ_PLANE:
      ilength  = (ext[1]-ext[0]);
      klength  = (ext[5]-ext[4]);
      numNodes = ilength*klength;
      break;
    case VTK_XYZ_GRID:
      ilength  = (ext[1]-ext[0]);
      jlength  = (ext[3]-ext[2]);
      klength  = (ext[5]-ext[4]);
      numNodes = ilength*jlength*klength;
      break;
    default:
      assert("pre: unsupported data-description, code should not reach here!"&&
             false );
    }
  return( numNodes );
}

//------------------------------------------------------------------------------
int vtkExtentRCBPartitioner::GetLongestDimensionLength( int ext[6] )
{
  int ilength = (ext[1]-ext[0])+1;
  int jlength = (ext[3]-ext[2])+1;
  int klength = (ext[5]-ext[4])+1;

  if ((ilength >= jlength) && (ilength >= klength))
    {
    return ilength;
    }
  else if ((jlength >= ilength) && (jlength >= klength))
    {
    return jlength;
    }
  else if ((klength >= ilength) && (klength >= jlength))
    {
    return klength;
    }

  assert( "pre: could not find longest dimension" && false );
  return 0;
}

//------------------------------------------------------------------------------
int vtkExtentRCBPartitioner::GetLongestDimension( int ext[6] )
{
  int ilength = (ext[1]-ext[0])+1;
  int jlength = (ext[3]-ext[2])+1;
  int klength = (ext[5]-ext[4])+1;

  if ((ilength >= jlength) && (ilength >= klength))
    {
    return 1;
    }
  else if ((jlength >= ilength) && (jlength >= klength))
    {
    return 2;
    }
  else if ((klength >= ilength) && (klength >= jlength))
    {
    return 3;
    }

  assert( "pre: could not find longest dimension" && false );
  return 0;
}

//------------------------------------------------------------------------------
void vtkExtentRCBPartitioner::AcquireDataDescription()
{
  this->DataDescription =
      vtkStructuredData::GetDataDescriptionFromExtent( this->GlobalExtent );
}

//------------------------------------------------------------------------------
void vtkExtentRCBPartitioner::PrintExtent( std::string name, int ext[6] )
{
  cout << name << ": [";
  for( int i=0; i < 6; ++i  )
    {
    cout << ext[i] << " ";
    }
  cout << "]\n";
  cout.flush();
}
