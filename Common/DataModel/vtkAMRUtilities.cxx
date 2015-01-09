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
#include "vtkAMRInformation.h"
#include "vtkAMRUtilities.h"
#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataArray.h"
#include "vtkFieldData.h"
#include "vtkOverlappingAMR.h"
#include "vtkPointData.h"
#include "vtkStructuredData.h"
#include "vtkUniformGrid.h"
#include "vtkUnsignedCharArray.h"
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
  vtkStructuredData::GetCellExtentFromPointExtent(
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
        vtkOverlappingAMR *strippedAMRData)
{
  assert("pre: input AMR data is NULL" && (ghostedAMRData != NULL) );
  assert("pre: outputAMR data is NULL" && (strippedAMRData != NULL) );
  double spacing[3];

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
  strippedAMRData->Initialize(static_cast<int>(blocksPerLevel.size()),&blocksPerLevel[0]);
  strippedAMRData->SetOrigin(ghostedAMRData->GetOrigin());
  strippedAMRData->SetGridDescription(ghostedAMRData->GetGridDescription());

  ghostedAMRData->GetSpacing(0,spacing);
  strippedAMRData->SetSpacing(0, spacing);
  unsigned int dataIdx=0;
  for( ;dataIdx < ghostedAMRData->GetNumberOfDataSets(0); ++dataIdx)
    {
    vtkUniformGrid* grid = ghostedAMRData->GetDataSet(0,dataIdx);
    const vtkAMRBox& box = ghostedAMRData->GetAMRBox(0,dataIdx);
    strippedAMRData->SetAMRBox(0,dataIdx,box);
    strippedAMRData->SetDataSet(0,dataIdx,grid);
    } // END for all data at level 0

  int ghost[6];
  unsigned int levelIdx = 1;
  for( ;levelIdx < ghostedAMRData->GetNumberOfLevels(); ++levelIdx )
    {
    dataIdx=0;
    ghostedAMRData->GetSpacing(levelIdx,spacing);
    strippedAMRData->SetSpacing(levelIdx, spacing);
    for(;dataIdx < ghostedAMRData->GetNumberOfDataSets(levelIdx); ++dataIdx)
      {
      vtkUniformGrid *grid = ghostedAMRData->GetDataSet( levelIdx, dataIdx );
      int r = ghostedAMRData->GetRefinementRatio( levelIdx );
      vtkAMRBox myBox=ghostedAMRData->GetAMRBox(levelIdx,dataIdx);
      vtkAMRBox strippedBox = myBox;
      strippedBox.RemoveGhosts(r);

      strippedAMRData->SetAMRBox(levelIdx,dataIdx, strippedBox);
      if( grid !=NULL )
        {
        myBox.GetGhostVector(r, ghost);

        vtkUniformGrid *strippedGrid=
          vtkAMRUtilities::StripGhostLayersFromGrid(grid,ghost);

        assert(strippedBox == vtkAMRBox(strippedGrid->GetOrigin(), strippedGrid->GetDimensions(), strippedGrid->GetSpacing(),strippedAMRData->GetOrigin(),strippedGrid->GetGridDescription()));
        strippedAMRData->SetAMRBox(levelIdx,dataIdx,strippedBox);
        strippedAMRData->SetDataSet(levelIdx,dataIdx,strippedGrid);
        strippedGrid->Delete();
        }
      } // END for all data at the given level
    } // END for all levels
}

//------------------------------------------------------------------------------
void vtkAMRUtilities::BlankCells(vtkOverlappingAMR* amr)
{
  vtkAMRInformation* info = amr->GetAMRInfo();
  if(!info->HasRefinementRatio())
    {
    info->GenerateRefinementRatio();
    }
  if(!info->HasChildrenInformation())
    {
    info->GenerateParentChildInformation();
    }

  std::vector<int> processorMap;
  processorMap.resize(amr->GetTotalNumberOfBlocks(),-1);
  vtkSmartPointer<vtkCompositeDataIterator> iter;
  iter.TakeReference(amr->NewIterator());
  iter->SkipEmptyNodesOn();

  for(iter->GoToFirstItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    unsigned int index = iter->GetCurrentFlatIndex();
    processorMap[index] = 0;
    }

  unsigned int numLevels =info->GetNumberOfLevels();
  for(unsigned int i=0; i<numLevels; i++)
    {
    BlankGridsAtLevel(amr, i, info->GetChildrenAtLevel(i),processorMap);
    }
}

//------------------------------------------------------------------------------
void vtkAMRUtilities::BlankGridsAtLevel(vtkOverlappingAMR* amr, int levelIdx,
                        std::vector<std::vector<unsigned int> >& children,
                        const std::vector<int>& processMap)
{
  unsigned int numDataSets = amr->GetNumberOfDataSets(levelIdx);
  int N;

  for( unsigned int dataSetIdx=0; dataSetIdx<numDataSets; dataSetIdx++)
    {
    const vtkAMRBox& box = amr->GetAMRBox(levelIdx, dataSetIdx);
    vtkUniformGrid* grid = amr->GetDataSet(levelIdx, dataSetIdx);
    if (grid == NULL )
      {
      continue;
      }
    N = grid->GetNumberOfCells();

    vtkUnsignedCharArray* vis = vtkUnsignedCharArray::New();
    vis->SetName("visibility");
    vis->SetNumberOfTuples( N );
    vis->FillComponent(0,static_cast<char>(1));
    grid->SetCellVisibilityArray(vis);
    vis->Delete();

    if (children.size() <= dataSetIdx)
      continue;

    std::vector<unsigned int>& dsChildren = children[dataSetIdx];
    std::vector<unsigned int>::iterator iter;

    // For each higher res box fill in the cells that
    // it covers
    for (iter=dsChildren.begin(); iter!=dsChildren.end(); iter++)
      {
      vtkAMRBox ibox;;
      int childGridIndex  = amr->GetCompositeIndex(levelIdx+1, *iter);
      if(processMap[childGridIndex]<0)
        {
        continue;
        }
      if (amr->GetAMRInfo()->GetCoarsenedAMRBox(levelIdx+1, *iter, ibox))
        {
        ibox.Intersect(box);
        const int *loCorner=ibox.GetLoCorner();
        int hi[3];
        ibox.GetValidHiCorner(hi);
        for( int iz=loCorner[2]; iz<=hi[2]; iz++ )
          {
          for( int iy=loCorner[1]; iy<=hi[1]; iy++ )
            {
            for( int ix=loCorner[0]; ix<=hi[0]; ix++ )
              {
              vtkIdType id =  vtkAMRBox::GetCellLinearIndex(box,ix, iy, iz, grid->GetDimensions());
              vis->SetValue(id, 0);
              } // END for x
            } // END for y
          } // END for z
        }
      } // Processing all higher boxes for a specific coarse grid
    }
}
