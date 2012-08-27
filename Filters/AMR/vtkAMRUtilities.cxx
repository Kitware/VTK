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
#include "vtkAMRInformation.h"
#include <cmath>
#include <limits>
#include <cassert>

#define IMIN(ext) ext[0]
#define IMAX(ext) ext[1]
#define JMIN(ext) ext[2]
#define JMAX(ext) ext[3]
#define KMIN(ext) ext[4]
#define KMAX(ext) ext[5]
#define PRINT(x) cout<<"("<<myRank<<")"<<x<<endl;

//------------------------------------------------------------------------------
void vtkAMRUtilities::PrintSelf( std::ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

//------------------------------------------------------------------------------
void vtkAMRUtilities::DistributeProcessInformation(vtkOverlappingAMR* amr,
                                                   vtkMultiProcessController *controller,
                                                   std::vector<int>& processMap)
{
  processMap.resize(amr->GetTotalNumberOfBlocks(),0);
  if(!controller || controller->GetNumberOfProcesses()==1)
    {
    return;
    }
  vtkAMRInformation* amrInfo = amr->GetAMRInfo();
  int myRank = controller->GetLocalProcessId();
  int numProcs   = controller->GetNumberOfProcesses();

  //get the active process ids
  std::vector<int> myBlocks;
  for( unsigned int level=0; level < amr->GetNumberOfLevels(); ++level )
    {
    for(unsigned int idx=0;idx < amr->GetNumberOfDataSets(level);++idx )
      {
      vtkUniformGrid *myGrid = amr->GetDataSet( level, idx );
      if( myGrid != NULL )
        {
        int index = amrInfo->GetIndex(level,idx);
        myBlocks.push_back(index);
        }
      } // END for all data at current level
    } // END for all levels

  vtkIdType myNumBlocks = myBlocks.size();
  std::vector<vtkIdType> numBlocks(numProcs,0);
  numBlocks[myRank] = myNumBlocks;

  //gather the active process counts
  controller->AllGather( &myNumBlocks, &numBlocks[0], 1);

  //gather the blocks each process owns into one array
  std::vector<vtkIdType> offsets(numProcs,0);
  vtkIdType currentOffset(0);
  for(int i=0; i<numProcs;i++)
    {
    offsets[i] = currentOffset;
    currentOffset+=numBlocks[i];
    }
  PRINT("total # of active blocks: "<<currentOffset<<" out of total "<<amrInfo->GetTotalNumberOfBlocks());
  std::vector<int> allBlocks(currentOffset,-1);
  controller->AllGatherV(&myBlocks[0], &allBlocks[0], (vtkIdType)myBlocks.size(), &numBlocks[0], &offsets[0] );

#ifdef DEBUG
  if(myRank==0)
    {
    for(int i=0; i<numProcs; i++)
      {
      vtkIdType offset= offsets[i];
      int n = numBlocks[i];
      cout<<"Rank "<<i<<" has: ";
      for(vtkIdType j=offset; j<offset+n; j++)
        {
        cout<<allBlocks[j]<<" ";
        }
      cout<<endl;
      }
    }
#endif
  for(int rank=0; rank<numProcs; rank++)
    {
    int offset= offsets[rank];
    int n = numBlocks[rank];
    for(int j=offset; j<offset+n; j++)
      {
      int index = allBlocks[j];
      assert(index>=0);
      processMap[index] =rank;
      }
    }
}

//------------------------------------------------------------------------------
bool vtkAMRUtilities::HasPartiallyOverlappingGhostCells(
    vtkOverlappingAMR *amr)
{
  assert("pre: input AMR data is NULL" && (amr != NULL) );
  int numLevels = static_cast<int>( amr->GetNumberOfLevels() );
  int levelIdx  = numLevels-1;
  for(; levelIdx > 0; --levelIdx )
    {
    int r = amr->GetRefinementRatio( levelIdx );
    unsigned int numDataSets = amr->GetNumberOfDataSets( levelIdx );
    for( unsigned int dataIdx=0; dataIdx < numDataSets; ++dataIdx )
      {
      const vtkAMRBox& myBox = amr->GetAMRInfo()->GetAMRBox(levelIdx,dataIdx);
      const int* lo = myBox.GetLoCorner();
      int hi[3];
      myBox.GetValidHiCorner(hi);
      vtkAMRBox coarsenedBox = myBox;
      coarsenedBox.Coarsen(r);

      // Detecting partially overlapping boxes is based on the following:
      // Cell location k at level L-1 holds the range [k*r,k*r+(r-1)] of
      // level L, where r is the refinement ratio. Consequently, if the
      // min extent of the box is greater than k*r or if the max extent
      // of the box is less than k*r+(r-1), then the grid partially overlaps.
      for( int i=0; i < 3; ++i )
        {
        if(myBox.EmptyDimension(i))
          {
          continue;
          }
        int minRange[2];
        minRange[0] = coarsenedBox.GetLoCorner()[i]*r;
        minRange[1] = coarsenedBox.GetLoCorner()[i]*r + (r-1);
        if( lo[i] > minRange[0] )
          {
          return true;
          }

        int coarseHi[3];
        coarsenedBox.GetValidHiCorner(coarseHi);
        int maxRange[2];
        maxRange[0] = coarseHi[i]*r;
        maxRange[1] = coarseHi[i]*r + (r-1);
        if( hi[i] < maxRange[1] )
          {
          return true;
          }
        } // END for all dimensions

      } // END for all data at the current level
    } // END for all levels
  return false;
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
  std::vector<int> blocksPerLevel(ghostedAMRData->GetNumberOfLevels());
  for(unsigned int i=0; i<blocksPerLevel.size();i++)
    {
    blocksPerLevel[i] = ghostedAMRData->GetNumberOfDataSets(i);
    }
  strippedAMRData->Initialize(blocksPerLevel.size(),&blocksPerLevel[0],
                              ghostedAMRData->GetOrigin(), ghostedAMRData->GetGridDescription());
  unsigned int dataIdx=0;
  for( ;dataIdx < ghostedAMRData->GetNumberOfDataSets(0); ++dataIdx)
    {
    vtkUniformGrid* grid = ghostedAMRData->GetDataSet(0,dataIdx);
    strippedAMRData->SetAMRBox(0,dataIdx,grid->GetOrigin(), grid->GetDimensions(), grid->GetSpacing());
    strippedAMRData->SetDataSet(0,dataIdx,grid);
    } // END for all data at level 0

  int ghost[6];
  unsigned int levelIdx = 1;
  for( ;levelIdx < ghostedAMRData->GetNumberOfLevels(); ++levelIdx )
    {
    dataIdx=0;
    for(;dataIdx < ghostedAMRData->GetNumberOfDataSets(levelIdx); ++dataIdx)
      {
      vtkUniformGrid *grid = ghostedAMRData->GetDataSet( levelIdx, dataIdx );
      if( grid == NULL )
        {
        strippedAMRData->SetDataSet( levelIdx, dataIdx, NULL );
        }
      else
        {
        int r = ghostedAMRData->GetRefinementRatio( levelIdx );

        vtkAMRBox myBox=ghostedAMRData->GetAMRInfo()->GetAMRBox(levelIdx,dataIdx);
        myBox.GetGhostVector(r, ghost);

        vtkUniformGrid *strippedGrid=
          vtkAMRUtilities::StripGhostLayersFromGrid(grid,ghost);
        strippedAMRData->SetAMRBox(levelIdx,dataIdx,strippedGrid->GetOrigin(), strippedGrid->GetDimensions(), strippedGrid->GetSpacing());
        strippedAMRData->SetDataSet(levelIdx,dataIdx,strippedGrid);
        strippedGrid->Delete();
        }
      } // END for all data at the given level
    } // END for all levels

  if( controller != NULL )
    {
    controller->Barrier();
    }
}
