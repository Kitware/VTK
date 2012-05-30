/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRUtilities.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkAMRBox.h"
#include "vtkAMRUtilities.h"
#include "vtkCellData.h"
#include "vtkCommunicator.h"
#include "vtkDataArray.h"
#include "vtkFieldData.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkOverlappingAMR.h"
#include "vtkPointData.h"
#include "vtkStructuredData.h"
#include "vtkUniformGrid.h"

#include <cmath>
#include <limits>
#include <cassert>

#define IMIN(ext) ext[0]
#define IMAX(ext) ext[1]
#define JMIN(ext) ext[2]
#define JMAX(ext) ext[3]
#define KMIN(ext) ext[4]
#define KMAX(ext) ext[5]

//------------------------------------------------------------------------------
void vtkAMRUtilities::PrintSelf( std::ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

//------------------------------------------------------------------------------
void vtkAMRUtilities::GenerateMetaData(
    vtkOverlappingAMR *amrData,
    vtkMultiProcessController *controller )
{
  // Sanity check
  assert( "Input AMR Data is NULL" && (amrData != NULL) );

  CollectAMRMetaData( amrData, controller );
  ComputeLevelRefinementRatio( amrData );
//  amrData->GenerateParentChildInformation();

  if( controller != NULL )
    {
    controller->Barrier();
    }
}

//------------------------------------------------------------------------------
void vtkAMRUtilities::ComputeDataSetOrigin(
       double origin[3], vtkOverlappingAMR *amrData,
       vtkMultiProcessController *controller )
{
  // Sanity check
  assert( "Input AMR Data is NULL" && (amrData != NULL) );

  double min[3];
  min[0] = min[1] = min[2] = std::numeric_limits<double>::max();

  // Note, we only need to check at level 0 since, the grids at
  // level 0 are guaranteed to cover the entire domain. Most datasets
  // will have a single grid at level 0.
  for( unsigned int idx=0; idx < amrData->GetNumberOfDataSets(0); ++idx )
    {
    vtkUniformGrid *gridPtr = amrData->GetDataSet( 0, idx );
    if( gridPtr != NULL )
      {
      double *gridBounds = gridPtr->GetBounds();
      assert( "Failed when accessing grid bounds!" && (gridBounds!=NULL) );
      if( gridBounds[0] < min[0] )
        {
        min[0] = gridBounds[0];
        }
      if( gridBounds[2] < min[1] )
        {
        min[1] = gridBounds[2];
        }
      if( gridBounds[4] < min[2] )
        {
        min[2] = gridBounds[4];
        }
      } // END if gridPtr is not NULL
    } // END for all data-sets at level 0

  // If data is distributed, get the global min
  if( controller != NULL )
    {
    if( controller->GetNumberOfProcesses() > 1 )
      {
      // TODO: Define a custom operator s.t. only one all-reduce operation
      // is called.
      controller->AllReduce(&min[0],&origin[0],1,vtkCommunicator::MIN_OP);
      controller->AllReduce(&min[1],&origin[1],1,vtkCommunicator::MIN_OP);
      controller->AllReduce(&min[2],&origin[2],1,vtkCommunicator::MIN_OP);
      return;
      }
    }

   // Else this is a single process
   origin[0] = min[0];
   origin[1] = min[1];
   origin[2] = min[2];
}

//------------------------------------------------------------------------------
void vtkAMRUtilities::ComputeGlobalBounds(
    double bounds[6], vtkOverlappingAMR *amrData,
    vtkMultiProcessController *myController )
{
  // Sanity check
  assert( "Input AMR Data is NULL" && (amrData != NULL) );

  double min[3];
  double max[3];
  min[0] = min[1] = min[2] = std::numeric_limits<double>::max();
  max[0] = max[1] = max[2] = std::numeric_limits<double>::min();

  // Note, we only need to check at level 0 since, the grids at
  // level 0 are guaranteed to cover the entire domain. Most datasets
  // will have a single grid at level 0.
  for( unsigned int idx=0; idx < amrData->GetNumberOfDataSets(0); ++idx )
    {
    vtkUniformGrid *gridPtr = amrData->GetDataSet( 0, idx );
    if( gridPtr != NULL )
      {
      // Get the bounnds of the grid: {xmin,xmax,ymin,ymax,zmin,zmax}
      double *gridBounds = gridPtr->GetBounds();
      assert( "Failed when accessing grid bounds!" && (gridBounds!=NULL) );

      // Check min
      if( gridBounds[0] < min[0] )
        {
        min[0] = gridBounds[0];
        }
      if( gridBounds[2] < min[1] )
        {
        min[1] = gridBounds[2];
        }
      if( gridBounds[4] < min[2] )
        {
        min[2] = gridBounds[4];
        }

      // Check max
      if( gridBounds[1] > max[0])
        {
        max[0] = gridBounds[1];
        }
      if( gridBounds[3] > max[1])
        {
        max[1] = gridBounds[3];
        }
      if( gridBounds[5] > max[2] )
        {
        max[2] = gridBounds[5];
        }
      } // END if grid is not NULL
    } // END for all data-sets at level 0

  if( myController != NULL )
    {
    if( myController->GetNumberOfProcesses() > 1 )
      {
      // All Reduce min
      myController->AllReduce(&min[0],&bounds[0],1,vtkCommunicator::MIN_OP);
      myController->AllReduce(&min[1],&bounds[1],1,vtkCommunicator::MIN_OP);
      myController->AllReduce(&min[2],&bounds[2],1,vtkCommunicator::MIN_OP);

      // All Reduce max
      myController->AllReduce(&max[0],&bounds[3],1,vtkCommunicator::MAX_OP);
      myController->AllReduce(&max[1],&bounds[4],1,vtkCommunicator::MAX_OP);
      myController->AllReduce(&max[2],&bounds[5],1,vtkCommunicator::MAX_OP);
      return;
      }
    }

  bounds[0] = min[0];
  bounds[1] = min[1];
  bounds[2] = min[2];
  bounds[3] = max[0];
  bounds[4] = max[1];
  bounds[5] = max[2];
}

//------------------------------------------------------------------------------
void vtkAMRUtilities::CollectAMRMetaData(
    vtkOverlappingAMR *amrData,
    vtkMultiProcessController *myController )
{
  // Sanity check
  assert( "Input AMR Data is NULL" && (amrData != NULL));

  // STEP 0: Compute the global dataset origin
  double origin[3];
  ComputeDataSetOrigin( origin, amrData, myController );
  amrData->SetOrigin( origin );

  // STEP 1: Compute the metadata of each process locally
  int process = (myController == NULL)? 0 : myController->GetLocalProcessId();
  ComputeLocalMetaData( origin, amrData, process );

  // STEP 2: Distribute meta-data to all processes
  if( myController != NULL )
    {
    DistributeMetaData( amrData, myController );
    }
}

//------------------------------------------------------------------------------
void vtkAMRUtilities::SerializeMetaData(
    vtkOverlappingAMR *amrData,
    unsigned char *&buffer,
    vtkIdType &numBytes )
{
  // Sanity check
  assert( "Input AMR Data is NULL" && (amrData != NULL) );

  // STEP 0: Collect all the AMR boxes in a vector
  std::vector< vtkAMRBox > boxList;
  for( unsigned int level=0; level < amrData->GetNumberOfLevels(); ++level )
    {
    for(unsigned int idx=0;idx < amrData->GetNumberOfDataSets( level );++idx )
      {
      if( amrData->GetDataSet(level,idx) != NULL )
        {
        vtkAMRBox myBox;
        amrData->GetMetaData(level,idx,myBox);
        boxList.push_back( myBox );
        }
      } // END for all data at the current level
    } // END for all levels

  // STEP 1: Compute & Allocate buffer size
  int N    = static_cast<int>( boxList.size( ) );
  numBytes = sizeof( int ) + vtkAMRBox::GetBytesize()*N;
  buffer   = new unsigned char[ numBytes ];

  // STEP 2: Serialize the number of boxes in the buffer
  unsigned char *ptr = buffer;
  memcpy( ptr, &N, sizeof(int) );
  ptr += sizeof(int);

  // STEP 3: Serialize each box
  for( unsigned int i=0; i < boxList.size( ); ++i )
    {
    assert( "ptr is NULL" && (ptr != NULL) );

    unsigned char *tmp = NULL;
    vtkIdType nbytes      = 0;
    boxList[ i ].Serialize( tmp, nbytes );
    memcpy( ptr, tmp, vtkAMRBox::GetBytesize() );
    ptr += vtkAMRBox::GetBytesize();
    }

}

//------------------------------------------------------------------------------
void vtkAMRUtilities::DeserializeMetaData(
    unsigned char *buffer,
    const vtkIdType vtkNotUsed(numBytes),
    std::vector< vtkAMRBox > &boxList )
{
  // Sanity check
  assert( "Buffer to deserialize is NULL" && (buffer != NULL) );

  unsigned char *ptr = buffer;
  int N              = 0;

  // STEP 0: Deserialize the number of boxes in the buffer
  memcpy( &N, ptr, sizeof(int) );
  ptr += sizeof(int);

  boxList.resize( N );
  for( int i=0; i < N; ++i )
    {
    assert( "ptr is NULL" && (ptr != NULL) );

    vtkIdType nbytes = vtkAMRBox::GetBytesize();
    boxList[ i ].Deserialize( ptr, nbytes );
    ptr += nbytes;
    }

}

//------------------------------------------------------------------------------
void vtkAMRUtilities::DistributeMetaData(
    vtkOverlappingAMR *amrData,
    vtkMultiProcessController *myController )
{
  // Sanity check
  assert( "Input AMR Data is NULL" && (amrData != NULL) );
  assert( "Multi-Process controller is NULL" && (myController != NULL) );

  // STEP 0: Serialize the meta-data owned by this process into a bytestream
  unsigned char *buffer = NULL;
  vtkIdType numBytes       = 0;
  SerializeMetaData( amrData, buffer, numBytes );
  assert( "Serialized buffer should not be NULL!" && (buffer != NULL) );
  assert( "Expected NumBytes > 0" && (numBytes > 0) );

  // STEP 1: Get the buffer sizes at each rank with an allGather
  int numRanks   = myController->GetNumberOfProcesses();
  vtkIdType *rcvcounts = new vtkIdType[ numRanks ];
  myController->AllGather( &numBytes, rcvcounts, 1);

  // STEP 2: Compute the receive buffer & Allocate
  vtkIdType rbufferSize = rcvcounts[0];
  for( int i=1; i < numRanks; ++i)
    {
    rbufferSize+=rcvcounts[i];
    }
  unsigned char *rcvBuffer = new unsigned char[ rbufferSize ];
  assert( "Receive buffer is NULL" && (rcvBuffer != NULL) );

  // STEP 3: Compute off-sets
  vtkIdType *offSet = new vtkIdType[ numRanks];
  offSet[0] = 0;
  for( int i=1; i < numRanks; ++i )
    {
    offSet[ i ] = offSet[ i-1 ]+rcvcounts[ i-1 ];
    }

  // STEP 4: All-gatherv boxes
  myController->AllGatherV( buffer, rcvBuffer, numBytes, rcvcounts, offSet );

  // STEP 5: Unpack receive buffer
  std::vector< std::vector< vtkAMRBox > > amrBoxes;
  amrBoxes.resize( numRanks );
  for( int i=0; i < numRanks; ++i )
    {
    if( i != myController->GetLocalProcessId() )
      {
      DeserializeMetaData( rcvBuffer+offSet[i],rcvcounts[i],amrBoxes[i] );
      for( unsigned int j=0; j < amrBoxes[i].size(); ++j )
        {
        int level = amrBoxes[i][j].GetLevel();
        int index = amrBoxes[i][j].GetBlockId();
        amrData->SetMetaData( level,index,amrBoxes[i][j] );
        }
      } // END if not a local process
    } // END for all ranks

  // STEP 7: Clean up all dynamically allocated memory
  delete [] buffer;
  delete [] rcvcounts;
  delete [] offSet;
  delete [] rcvBuffer;
}

//------------------------------------------------------------------------------
void vtkAMRUtilities::CreateAMRBoxForGrid(
    double origin[3], vtkUniformGrid *myGrid, vtkAMRBox &myBox )
{
  // Sanity check
  assert( "Input AMR Grid is not NULL" && (myGrid != NULL) );

  double *gridOrigin = myGrid->GetOrigin();
  assert( "Null Grid Origin" && (gridOrigin != NULL)  );

  int ndim[3];
  int lo[3];
  int hi[3];

  // Get pointer to the grid's spacing array
  double *h = myGrid->GetSpacing();
  assert( "Grid Spacing array is NULL!" && (h!=NULL) );

  // Get the grid's cell dimensions,i.e., number of cells along each dimension.
  myGrid->GetDimensions( ndim );
  ndim[0]--; ndim[1]--; ndim[2]--;
  ndim[0] = (ndim[0] < 1)? 1 : ndim[0];
  ndim[1] = (ndim[1] < 1)? 1 : ndim[1];
  ndim[2] = (ndim[2] < 1)? 1 : ndim[2];

  // Compute lo,hi box dimensions
  int i=0;
  switch( myGrid->GetDataDimension() )
    {
    case 1:
      lo[0] = vtkMath::Round( (gridOrigin[0]-origin[0])/h[0] );
      hi[0] = vtkMath::Round( static_cast<double>(lo[0] + ( ndim[0]-1 )) );
      for(i=1; i < 3; ++i )
        {
        lo[i] = hi[i] = 0;
        }
      break;
    case 2:
      if( myGrid->GetGridDescription() == VTK_YZ_PLANE )
        {
        // YZ plane
        for( i=1; i < 3; ++i )
          {
          lo[i] = vtkMath::Round( (gridOrigin[i]-origin[i])/h[i] );
          hi[i] = vtkMath::Round( static_cast<double>(lo[i] + ( ndim[i]-1 )) );
          }
        lo[0] = hi[0] = vtkMath::Round( (gridOrigin[0]-origin[0])/h[0] );
        }
      else if( myGrid->GetGridDescription() == VTK_XZ_PLANE )
        {
        // XZ plane
        lo[1] = hi[1] = vtkMath::Round( (gridOrigin[1]-origin[1])/h[1] );
        lo[0] = vtkMath::Round( (gridOrigin[0]-origin[0])/h[0] );
        hi[0] = vtkMath::Round( static_cast<double>(lo[0] + ( ndim[0]-1 )) );
        lo[2] = vtkMath::Round( (gridOrigin[2]-origin[2])/h[2] );
        hi[2] = vtkMath::Round( static_cast<double>(lo[2] + ( ndim[2]-1 )) );
        }
      else if( myGrid->GetGridDescription() == VTK_XY_PLANE )
        {
        // XY plane
        for( i=0; i < 2; ++i )
          {
          lo[i] = vtkMath::Round( (gridOrigin[i]-origin[i])/h[i] );
          hi[i] = vtkMath::Round( static_cast<double>(lo[i] + ( ndim[i]-1 )) );
          }
        lo[2] = hi[2] = vtkMath::Round( (gridOrigin[2]-origin[2])/h[2] );
        }
      break;
    case 3:
      for( i=0; i < 3; ++i )
        {
        lo[i] = vtkMath::Round( (gridOrigin[i]-origin[i])/h[i] );
        hi[i] = vtkMath::Round( static_cast<double>(lo[i] + ( ndim[i]-1 )) );
        }
      break;
    default:
      assert("Invalid grid dimension! Code should not reach here!" && false );
    } // END switch

  myBox.SetGridDescription( myGrid->GetGridDescription( ) );
  myBox.SetDimensionality( myGrid->GetDataDimension() );
  myBox.SetDataSetOrigin( origin );
  myBox.SetGridSpacing( h );
  myBox.SetDimensions( lo, hi );

  assert("post: Invalid AMR box Dimension, only 2,3 are supported" &&
         ( myBox.GetDimensionality()==2 || myBox.GetDimensionality()==3 ) );
  assert( "post: AMR box is invalid!" && !myBox.IsInvalid() );
}

//------------------------------------------------------------------------------
void vtkAMRUtilities::ComputeLocalMetaData(
    double origin[3], vtkOverlappingAMR* myAMRData, const int process )
{
  // Sanity check
  assert( "Input AMR data is NULL" && (myAMRData != NULL) );

  for( unsigned int level=0; level < myAMRData->GetNumberOfLevels(); ++level )
    {
    for(unsigned int idx=0;idx < myAMRData->GetNumberOfDataSets(level);++idx )
      {
      vtkUniformGrid *myGrid = myAMRData->GetDataSet( level, idx );
      if( myGrid != NULL )
        {
        vtkAMRBox myBox;
        CreateAMRBoxForGrid( origin, myGrid, myBox );
        myBox.SetBlockId( idx );
        myBox.SetLevel( level );
        myBox.SetProcessId( process );
        myAMRData->SetMetaData( level, idx, myBox );
        }
      } // END for all data at current level
    } // END for all levels
}

//------------------------------------------------------------------------------
void vtkAMRUtilities::ComputeLevelRefinementRatio(
    vtkOverlappingAMR *amr )
{
  // sanity check
  assert( "Input AMR Data is NULL" && (amr != NULL)  );

  int numLevels = amr->GetNumberOfLevels();

  if( numLevels < 1 )
    {
    // Dataset is empty!
    return;
    }

  if( numLevels == 1)
    {
    // No refinement, data-set has only a single level.
    // The refinement ratio is set to 2 to satisfy the
    // vtkOverlappingAMR requirement.
    amr->SetRefinementRatio(0,2);
    return;
    }

   for( int level=0; level < numLevels-1; ++level )
     {
     int childLevel = level+1;
     assert("No data at parent!" && amr->GetNumberOfDataSets(childLevel)>=1);
     assert("No data in this level" && amr->GetNumberOfDataSets(level)>=1 );

     vtkAMRBox childBox;
     amr->GetMetaData(childLevel,0,childBox);

     vtkAMRBox myBox;
     amr->GetMetaData(level,0,myBox);

     double childSpacing[3];
     childBox.GetGridSpacing(childSpacing);

     double currentSpacing[3];
     myBox.GetGridSpacing( currentSpacing );

     // Note current implementation assumes uniform spacing. The
     // refinement ratio is the same in each dimension i,j,k.
     int ratio = vtkMath::Round(currentSpacing[0]/childSpacing[0]);

     // Set the ratio at the last level, i.e., level numLevels-1, to be the
     // same as the ratio at the previous level,since the highest level
     // doesn't really have a refinement ratio.
     if( level==numLevels-2 )
       {
       amr->SetRefinementRatio(level+1,ratio);
       }
     amr->SetRefinementRatio(level,ratio);
     } // END for all hi-res levels
}

//------------------------------------------------------------------------------
bool vtkAMRUtilities::HasPartiallyOverlappingGhostCells(
    vtkOverlappingAMR *amr)
{
  assert("pre: input AMR data is NULL" && (amr != NULL) );
  unsigned int numLevels = amr->GetNumberOfLevels();
  unsigned int levelIdx = numLevels-1;
  for(; levelIdx > 0; --levelIdx )
    {
    unsigned int lowResLevel = levelIdx-1;
    assert("pre: lowResLevel >= 0" && (static_cast<int>(lowResLevel) >= 0) );

    int r = amr->GetRefinementRatio( levelIdx );
    unsigned int numDataSets = amr->GetNumberOfDataSets( levelIdx );
    for( unsigned int dataIdx=0; dataIdx < numDataSets; ++dataIdx )
      {
      assert( "pre: AMR dataset has no metadata for requested grid!" &&
               amr->HasMetaData(levelIdx,dataIdx) );

      vtkAMRBox myBox;
      amr->GetMetaData(levelIdx,dataIdx,myBox);

      vtkAMRBox coarsenedBox = myBox;
      coarsenedBox.Coarsen( r );

      // Detecting partially overlapping boxes is based on the following:
      // Cell location k at level L-1 holds the range [k*r,k*r+(r-1)] of
      // level L, where r is the refinement ratio. Consequently, if the
      // min extent of the box is greater than k*r or if the max extent
      // of the box is less than k*r+(r-1), then the grid partially overlaps.
      for( int i=0; i < myBox.GetDimensionality(); ++i )
        {
        int minRange[2];
        minRange[0] = coarsenedBox.LoCorner[i]*r;
        minRange[1] = coarsenedBox.LoCorner[i]*r + (r-1);
        if( myBox.LoCorner[i] > minRange[0] )
          {
          return true;
          }

        int maxRange[2];
        maxRange[0] = coarsenedBox.HiCorner[i]*r;
        maxRange[1] = coarsenedBox.HiCorner[i]*r + (r-1);
        if( myBox.HiCorner[i] < maxRange[1] )
          {
          return true;
          }
        } // END for all dimensions

      } // END for all data at the current level
    } // END for all levels
  return false;
}

//------------------------------------------------------------------------------
void vtkAMRUtilities::GetGhostVector(
    vtkAMRBox &box, int r, int nghost[6])
{
  // STEP 0: initialize nghost
  for( int i=0; i < 3; ++i )
    {
    nghost[i*2] = nghost[i*2+1] = 0;
    }

  // STEP 1: compute number of ghost layers along each dimension's min and max.
  // Detecting partially overlapping boxes is based on the following:
  // Cell location k at level L-1 holds the range [k*r,k*r+(r-1)] of
  // level L, where r is the refinement ratio. Consequently, if the
  // min extent of the box is greater than k*r or if the max extent
  // of the box is less than k*r+(r-1), then the grid partially overlaps.

  vtkAMRBox coarsenedBox = box;
  coarsenedBox.Coarsen( r );
  for( int i=0; i < box.GetDimensionality(); ++i )
    {
    int minRange[2];
    minRange[0] = coarsenedBox.LoCorner[i]*r;
    minRange[1] = coarsenedBox.LoCorner[i]*r + (r-1);
    if( box.LoCorner[i] > minRange[0] )
      {
      nghost[i*2]=(minRange[1]+1)-box.LoCorner[i];
      }

    int maxRange[2];
    maxRange[0] = coarsenedBox.HiCorner[i]*r;
    maxRange[1] = coarsenedBox.HiCorner[i]*r + (r-1);
    if( box.HiCorner[i] < maxRange[1] )
      {
      nghost[i*2+1] = box.HiCorner[i]-(maxRange[0]-1);
      }
    } // END for all dimensions

}

//------------------------------------------------------------------------------
void vtkAMRUtilities::CopyFieldData(
    vtkFieldData *target, vtkIdType targetIdx,
    vtkFieldData *source, vtkIdType srcIdx )
{
  assert("pre: target should not be NULL" && (target != NULL) );
  assert("pre: source should not be NULL" && (source != NULL) );
  assert("pre: number of arrays between source and target does not match!" &&
         (source->GetNumberOfArrays()==target->GetNumberOfArrays() ) );

  for( int arrayIdx=0; arrayIdx < source->GetNumberOfArrays(); ++arrayIdx )
    {
    vtkDataArray *targetArray = target->GetArray( arrayIdx );
    vtkDataArray *srcArray    = source->GetArray( arrayIdx );
    assert( "pre: target array is NULL!" && (targetArray != NULL) );
    assert( "pre: source array is NULL!" && (srcArray != NULL) );
    assert( "pre: targer/source array number of components mismatch!" &&
            (targetArray->GetNumberOfComponents()==
             srcArray->GetNumberOfComponents() ) );
    assert( "pre: target/source array names mismatch!" &&
            (strcmp(targetArray->GetName(),srcArray->GetName()) == 0) );
    assert( "pre: source index is out-of-bounds" &&
            (srcIdx >=0) &&
            (srcIdx < srcArray->GetNumberOfTuples() ) );
    assert( "pre: target index is out-of-bounds" &&
            (targetIdx >= 0) &&
            (targetIdx < targetArray->GetNumberOfTuples() ) );

    // copy the tuple from the source array
    targetArray->SetTuple( targetIdx, srcIdx, srcArray );
    } // END for all arrays

}

//------------------------------------------------------------------------------
void vtkAMRUtilities::CopyFieldsWithinRealExtent(
    int realExtent[6],
    vtkUniformGrid *ghostedGrid,
    vtkUniformGrid *strippedGrid)
{
  assert("pre: input ghost grid is NULL" && (ghostedGrid != NULL) );
  assert("pre: input stripped grid is NULL" && (strippedGrid != NULL) );

  // STEP 0: Initialize the unghosted grid fields (point/cell data)
  strippedGrid->GetPointData()->CopyAllOn();
  strippedGrid->GetPointData()->CopyAllocate(
      ghostedGrid->GetPointData(),strippedGrid->GetNumberOfPoints() );
  strippedGrid->GetCellData()->CopyAllOn();
  strippedGrid->GetCellData()->CopyAllocate(
      ghostedGrid->GetCellData(),strippedGrid->GetNumberOfCells() );

  // STEP 1: Ensure each array has the right number of tuples, for some reason
  // CopyAllocate does not allocate the arrays with the prescribed size.
  int arrayIdx = 0;
  for(;arrayIdx < strippedGrid->GetPointData()->GetNumberOfArrays();++arrayIdx)
    {
    strippedGrid->GetPointData()->GetArray(arrayIdx)->
        SetNumberOfTuples(strippedGrid->GetNumberOfPoints() );
    } // END for all node arrays

  for(;arrayIdx < strippedGrid->GetCellData()->GetNumberOfArrays();++arrayIdx)
    {
    strippedGrid->GetCellData()->GetArray(arrayIdx)->
        SetNumberOfTuples( strippedGrid->GetNumberOfCells() );
    } // END for all cell arrays

  // STEP 2: Get the data-description
  int dataDescription =
      vtkStructuredData::GetDataDescriptionFromExtent(realExtent);
  // NOTE: a mismatch in the description here is possible but, very unlikely.
  // For example, consider a grid on the XY-PLANE that is padded with ghost
  // nodes along the z-dimension. Consequently, the ghosted grid will have
  // a 3-D data-description and the unghosted grid will be 2-D. Again, although
  // possible, this is not a realistic use-case. We will just catch this error
  // here and fix if we ever come across such use-case.
  assert("pre: description of ghosted and non-ghosted grid mismatch!" &&
         (dataDescription == vtkStructuredData::GetDataDescription(
                 ghostedGrid->GetDimensions())));

  // STEP 3: Get the corresponding cell-extent for accessing cell fields
  int realCellExtent[6];
  vtkStructuredData::GetCellExtentFromNodeExtent(
      realExtent,realCellExtent,dataDescription);

  // STEP 4: Loop through all real nodes/cells and copy the fields onto the
  // stripped grid.
  int ijk[3];
  int lijk[3];
  for(int i=IMIN(realExtent); i <= IMAX(realExtent); ++i)
    {
    for(int j=JMIN(realExtent); j <= JMAX(realExtent); ++j)
      {
      for( int k=KMIN(realExtent); k <= KMAX(realExtent); ++k)
        {
        // Vectorize i,j,k
        ijk[0]=i; ijk[1]=j; ijk[2]=k;

        // Compute the local i,j,k on the un-ghosted grid
        vtkStructuredData::GetLocalStructuredCoordinates(
            ijk,realExtent,lijk,dataDescription);

        // Compute the source index w.r.t. the ghosted grid dimensions
        vtkIdType sourceIdx =
            vtkStructuredData::ComputePointId(
                ghostedGrid->GetDimensions(), ijk, dataDescription );

        // Compute the target index w.r.t the real extent
        vtkIdType targetIdx =
            vtkStructuredData::ComputePointIdForExtent(
                realExtent,ijk,dataDescription);

        // Copy node-centered data
        vtkAMRUtilities::CopyFieldData(
            strippedGrid->GetPointData(),targetIdx,
            ghostedGrid->GetPointData(),sourceIdx);

        // If within the cell-extent, copy cell-centered data
        if( (i >= IMIN(realCellExtent)) && (i <= IMAX(realCellExtent))&&
            (j >= JMIN(realCellExtent)) && (j <= JMAX(realCellExtent))&&
            (k >= KMIN(realCellExtent)) && (k <= KMAX(realCellExtent)) )
          {
          // Compute the source cell index w.r.t. the ghosted grid
          vtkIdType sourceCellIdx =
              vtkStructuredData::ComputeCellId(
                  ghostedGrid->GetDimensions(),ijk,dataDescription);

          // Compute the target cell index w.r.t. the un-ghosted grid
          vtkIdType targetCellIdx =
              vtkStructuredData::ComputeCellId(
                  strippedGrid->GetDimensions(),lijk,dataDescription);

          // Copy cell-centered data
          vtkAMRUtilities::CopyFieldData(
              strippedGrid->GetCellData(),targetCellIdx,
              ghostedGrid->GetCellData(),sourceCellIdx);
          } // END if within the cell extent

        } // END for all k
      } // END for all j
    } // END for all i
}

//------------------------------------------------------------------------------
vtkUniformGrid* vtkAMRUtilities::StripGhostLayersFromGrid(
      vtkUniformGrid* grid, int ghost[6] )
{
  assert("pre: input grid is NULL" && (grid != NULL) );

  // STEP 0: Get the grid properties, i.e., origin, dims, extent, etc.
  double origin[3];
  double spacing[3];
  int dims[3];
  int ghostedGridDims[3];

  grid->GetOrigin( origin );
  grid->GetSpacing( spacing );
  grid->GetDimensions( ghostedGridDims );
  grid->GetDimensions( dims );

  int copyExtent[6];
  grid->GetExtent( copyExtent );

  // STEP 1: Adjust origin, copyExtent, dims according to the supplied ghost
  // vector.
  for( int i=0; i < 3; ++i )
    {
    if( ghost[i*2] > 0)
      {
      copyExtent[i*2] += ghost[i*2];
      dims[i]         -= ghost[i*2];
      origin[i]       += ghost[i*2]*spacing[i];
      }
    if( ghost[i*2+1] > 0 )
      {
      dims[i]           -= ghost[i*2+1];
      copyExtent[i*2+1] -= ghost[i*2+1];
      }
    } // END for all dimensions

  // STEP 2: Initialize the unghosted grid
  vtkUniformGrid *myGrid = vtkUniformGrid::New();
  myGrid->Initialize();
  myGrid->SetOrigin(origin);
  myGrid->SetSpacing(spacing);
  myGrid->SetDimensions( dims );

  // STEP 3: Copy the field data within the real extent
  vtkAMRUtilities::CopyFieldsWithinRealExtent(copyExtent,grid,myGrid);
  return( myGrid );
}

//------------------------------------------------------------------------------
void vtkAMRUtilities::StripGhostLayers(
        vtkOverlappingAMR *ghostedAMRData,
        vtkOverlappingAMR *strippedAMRData,
        vtkMultiProcessController *controller)
{
  assert("pre: input AMR data is NULL" && (ghostedAMRData != NULL) );
  assert("pre: outputAMR data is NULL" && (strippedAMRData != NULL) );

  if( !vtkAMRUtilities::HasPartiallyOverlappingGhostCells( ghostedAMRData ) )
    {
    strippedAMRData->ShallowCopy(ghostedAMRData);
    return;
    }

  // TODO: At some point we should check for overlapping cells within the
  // same level, e.g., consider a level 0 with 2 abutting blocks that is
  // ghosted by N !!!!
  strippedAMRData->SetNumberOfLevels( ghostedAMRData->GetNumberOfLevels() );
  unsigned int dataIdx=0;
  for( ;dataIdx < ghostedAMRData->GetNumberOfDataSets(0); ++dataIdx)
    {
    strippedAMRData->SetDataSet(
        0,dataIdx,ghostedAMRData->GetDataSet(0,dataIdx));
    } // END for all data at level 0

  int ghost[6];
  unsigned int levelIdx = 1;
  for( ;levelIdx < ghostedAMRData->GetNumberOfLevels(); ++levelIdx )
    {
    unsigned int dataIdx = 0;
    for( ;dataIdx < ghostedAMRData->GetNumberOfDataSets(levelIdx); ++dataIdx)
      {
      assert( "pre: AMR dataset has no metadata for requested grid!" &&
               ghostedAMRData->HasMetaData(levelIdx,dataIdx) );

      vtkUniformGrid *grid = ghostedAMRData->GetDataSet( levelIdx, dataIdx );
      if( grid == NULL )
        {
        strippedAMRData->SetDataSet( levelIdx, dataIdx, NULL );
        }
      else
        {
        int r = ghostedAMRData->GetRefinementRatio( levelIdx );

        vtkAMRBox myBox;
        ghostedAMRData->GetMetaData(levelIdx,dataIdx,myBox);
        vtkAMRUtilities::GetGhostVector(myBox,r,ghost);

        vtkUniformGrid *strippedGrid=
            vtkAMRUtilities::StripGhostLayersFromGrid(grid,ghost);
        strippedAMRData->SetDataSet(levelIdx,dataIdx,strippedGrid);
        strippedGrid->Delete();
        }
      } // END for all data at the given level
    } // END for all levels

  vtkAMRUtilities::GenerateMetaData(strippedAMRData,controller);
  if( controller != NULL )
    {
    controller->Barrier();
    }
}
