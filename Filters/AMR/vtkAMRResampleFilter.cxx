/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRToUniformGrid.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

#include "vtkAMRResampleFilter.h"
#include "vtkAMRInformation.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkOverlappingAMR.h"
#include "vtkMultiProcessController.h"
#include "vtkUniformGrid.h"
#include "vtkIndent.h"
#include "vtkAMRUtilities.h"
#include "vtkBoundingBox.h"
#include "vtkMath.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkFieldData.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkCell.h"
//#include "vtkXMLImageDataWriter.h"
#include "vtkExtentRCBPartitioner.h"
#include "vtkUniformGridPartitioner.h"
#include "vtkDataArray.h"

#include "vtkTimerLog.h"

#include <cassert>
#include <cmath>
#include <sstream>
#include <cmath>
#include <algorithm>


vtkStandardNewMacro( vtkAMRResampleFilter );

//-----------------------------------------------------------------------------
vtkAMRResampleFilter::vtkAMRResampleFilter()
{
  this->TransferToNodes      = 1;
  this->DemandDrivenMode     = 0;
  this->NumberOfPartitions   = 1;
  this->LevelOfResolution    = 0;
  this->AMRMetaData          = NULL;
  this->NumberOfSamples[0]   = this->NumberOfSamples[1] = this->NumberOfSamples[2] = 10;
  this->Controller           = vtkMultiProcessController::GetGlobalController();
  this->ROI                  = vtkMultiBlockDataSet::New();

  for( int i=0; i < 3; ++i )
    {
    this->Min[i] = 0.;
    this->Max[i] = 1.;
    }
  this->SetNumberOfInputPorts( 1 );
  this->SetNumberOfOutputPorts( 1 );
  this->UseBiasVector = false;
  this->BiasVector[0] = this->BiasVector[1] = this->BiasVector[2] = 0.0;
}

//-----------------------------------------------------------------------------
vtkAMRResampleFilter::~vtkAMRResampleFilter()
{
  this->BlocksToLoad.clear();

  if( this->ROI != NULL )
    {
    this->ROI->Delete();
    }
  this->ROI = NULL;

//  if( this->AMRMetaData != NULL )
//    {
//    this->AMRMetaData->Delete();
//    }
//  this->AMRMetaData = NULL;
}

//-----------------------------------------------------------------------------
void vtkAMRResampleFilter::PrintSelf( std::ostream &oss, vtkIndent indent )
{
  this->Superclass::PrintSelf( oss, indent );
}

//-----------------------------------------------------------------------------
int vtkAMRResampleFilter::FillInputPortInformation(
    int vtkNotUsed(port), vtkInformation *info )
{
  assert( "pre: information object is NULL" && (info != NULL) );
  info->Set(
   vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkOverlappingAMR" );
  return 1;
}

//-----------------------------------------------------------------------------
int vtkAMRResampleFilter::FillOutputPortInformation(
    int vtkNotUsed(port), vtkInformation *info )
{
  assert( "pre: information object is NULL" && (info != NULL) );
  info->Set(vtkDataObject::DATA_TYPE_NAME(),"vtkMultiBlockDataSet");
  return 1;
}

//-----------------------------------------------------------------------------
int vtkAMRResampleFilter::RequestUpdateExtent(
    vtkInformation*, vtkInformationVector **inputVector,
    vtkInformationVector* vtkNotUsed(outputVector) )
{
  assert( "pre: inputVector is NULL" && (inputVector != NULL) );
  vtkInformation *info = inputVector[0]->GetInformationObject(0);
  assert( "pre: info is NULL" && (info != NULL) );

  if( this->DemandDrivenMode == 1 )
    {
    // Tell reader to load all requested blocks.
    info->Set( vtkCompositeDataPipeline::LOAD_REQUESTED_BLOCKS(), 1 );

    // Tell reader which blocks this process requires
    info->Set(
        vtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES(),
        &this->BlocksToLoad[0], static_cast<int>(this->BlocksToLoad.size()));
    }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkAMRResampleFilter::RequestInformation(
    vtkInformation* vtkNotUsed(rqst),
    vtkInformationVector** inputVector,
    vtkInformationVector* vtkNotUsed(outputVector) )
{

  assert( "pre: inputVector is NULL" && (inputVector != NULL) );

  vtkInformation *input = inputVector[0]->GetInformationObject( 0 );
  assert( "pre: input is NULL" && (input != NULL)  );

  if( this->DemandDrivenMode == 1 &&
      input->Has(vtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA() ) )
    {
//    this->AMRMetaData = vtkOverlappingAMR::New();
    this->AMRMetaData =
    vtkOverlappingAMR::SafeDownCast(
      input->Get( vtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA() ) );


    // Get Region
    double h[3];
    this->ComputeAndAdjustRegionParameters( this->AMRMetaData, h );
    this->GetRegion( h );

    // Compute which blocks to load
    this->ComputeAMRBlocksToLoad( this->AMRMetaData );
    }

// Don't we need to call this->Modified() here?
// this->Modified();
 return 1;
}

//-----------------------------------------------------------------------------
int vtkAMRResampleFilter::RequestData(
    vtkInformation* vtkNotUsed(rqst), vtkInformationVector** inputVector,
    vtkInformationVector* outputVector )
{
  cerr << "Running Resampler\n";

  // STEP 0: Get input object
  vtkInformation *input = inputVector[0]->GetInformationObject( 0 );
  assert( "pre: Null information object!" && (input != NULL) );
  vtkOverlappingAMR *amrds=
     vtkOverlappingAMR::SafeDownCast(
      input->Get(vtkDataObject::DATA_OBJECT()));
  assert( "pre: input AMR dataset is NULL" && (amrds != NULL) );

  // STEP 1: Get output object
   vtkInformation *output = outputVector->GetInformationObject( 0 );
   assert( "pre: Null output information object!" && (output != NULL) );

   vtkMultiBlockDataSet *mbds =
      vtkMultiBlockDataSet::SafeDownCast(
          output->Get(vtkDataObject::DATA_OBJECT() ) );
   assert( "pre: ouput grid is NULL" && (mbds != NULL) );

  // STEP 2: Get Metadata
  if( this->DemandDrivenMode == 1 )
    {
    assert( "pre: Metadata must have been populated in RqstInfo" &&
            (this->AMRMetaData != NULL) );
    this->ExtractRegion( amrds, mbds, this->AMRMetaData );
    }
  else
    {
    // GetRegion
    double h[3];
    this->ComputeAndAdjustRegionParameters(amrds, h );
    this->GetRegion( h );
    this->ExtractRegion( amrds, mbds, amrds);
    }

  return 1;
}


//-----------------------------------------------------------------------------
bool vtkAMRResampleFilter::FoundDonor(
    double q[3],vtkUniformGrid *&donorGrid,int &cellIdx)
{
  assert( "pre: donor grid is NULL" && (donorGrid != NULL) );
  double gbounds[6];
  // Lets do a trival spatial check
  this->NumberOfBlocksTested++;
  donorGrid->GetBounds(gbounds);
  if ((q[0] < gbounds[0]) || (q[0] > gbounds[1]) ||
      (q[1] < gbounds[2]) || (q[1] > gbounds[3]) ||
      (q[2] < gbounds[4]) || (q[2] > gbounds[5]))
    {
    return false;
    }
  int ijk[3];
  double pcoords[3];
  int status = donorGrid->ComputeStructuredCoordinates( q, ijk, pcoords );
  if( status == 1 )
    {
    cellIdx=vtkStructuredData::ComputeCellId(donorGrid->GetDimensions(),ijk);
    return true;
    }
  return false;
}


//-----------------------------------------------------------------------------
void vtkAMRResampleFilter::InitializeFields(
      vtkFieldData *f, vtkIdType size, vtkCellData *src )
{
  assert( "pre: field data is NULL!" && (f != NULL) );
  assert( "pre: source cell data is NULL" && (src != NULL) );

  for( int arrayIdx=0; arrayIdx < src->GetNumberOfArrays(); ++arrayIdx )
    {
    int dataType        = src->GetArray( arrayIdx )->GetDataType();
    vtkDataArray *array = vtkDataArray::CreateDataArray( dataType );
    assert( "pre: failed to create array!" && (array != NULL) );

    array->SetName( src->GetArray(arrayIdx)->GetName() );
    array->SetNumberOfComponents(
             src->GetArray(arrayIdx)->GetNumberOfComponents() );
    array->SetNumberOfTuples( size );
    assert( "post: array size mismatch" &&
            (array->GetNumberOfTuples() == size) );

    f->AddArray( array );
    array->Delete();

//      int myIndex = -1;
//      vtkDataArray *arrayPtr = f->GetArray(
//          src->GetArray(arrayIdx)->GetName(), myIndex );
//      assert( "post: array index mismatch" && (myIndex == arrayIdx) );
//      assert( "post: array size mismatch" &&
//              (arrayPtr->GetNumberOfTuples()==size) );

    assert( "post: array size mismatch" &&
            (f->GetArray( arrayIdx)->GetNumberOfTuples() == size) );
    } // END for all arrays

}

//-----------------------------------------------------------------------------
void vtkAMRResampleFilter::CopyData(
    vtkFieldData *target, vtkIdType targetIdx,
    vtkCellData *src, vtkIdType srcIdx )
{
  assert( "pre: target field data is NULL" && (target != NULL) );
  assert( "pre: source field data is NULL" && (src != NULL) );
  assert( "pre: number of arrays does not match" &&
           (target->GetNumberOfArrays() == src->GetNumberOfArrays() ) );

  int arrayIdx = 0;
  for( ; arrayIdx < src->GetNumberOfArrays(); ++arrayIdx )
    {
    vtkDataArray *targetArray = target->GetArray( arrayIdx );
    vtkDataArray *srcArray    = src->GetArray( arrayIdx );
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

    int c=0;
    for( ; c < srcArray->GetNumberOfComponents(); ++c )
      {
      double f = srcArray->GetComponent( srcIdx, c );
      targetArray->SetComponent( targetIdx, c, f );
      } // END for all componenents

    } // END for all arrays
}

//-----------------------------------------------------------------------------
void vtkAMRResampleFilter::ComputeCellCentroid(
    vtkUniformGrid *g, const vtkIdType cellIdx, double c[3] )
{
  assert( "pre: uniform grid is NULL" && (g != NULL) );
  assert( "pre: centroid is NULL" && (c != NULL) );
  assert( "pre: cell index out-of-bounds" &&
           (cellIdx >= 0) && (cellIdx < g->GetNumberOfCells()) );


  vtkCell *myCell = g->GetCell( cellIdx );
  assert( "post: cell is NULL!" && (myCell != NULL) );

  double pc[3]; // the parametric center
  double *weights = new double[ myCell->GetNumberOfPoints() ];
  assert( "post: weights vector is NULL" && (weights != NULL) );

  int subId = myCell->GetParametricCenter( pc );
  myCell->EvaluateLocation( subId, pc, c, weights );
  delete [] weights;
}

//-----------------------------------------------------------------------------
void vtkAMRResampleFilter::TransferToCellCenters(
        vtkUniformGrid *g, vtkOverlappingAMR *amrds )
{
  assert( "pre: uniform grid is NULL" && (g != NULL) );
  assert( "pre: AMR data-strucutre is NULL" && (amrds != NULL) );

  // STEP 0: Get the first block so that we know the arrays
//  vtkUniformGrid *refGrid = amrds->GetDataSet(0,0);
//  assert( "pre: Block(0,0) is NULL!" && (refGrid != NULL) );
  vtkUniformGrid *refGrid = this->GetReferenceGrid( amrds );

  // STEP 1: Get the cell-data of the reference grid
  vtkCellData *CD = refGrid->GetCellData();
  assert( "pre: Donor CellData is NULL!" && (CD != NULL)  );

  // STEP 2: Get the cell data of the resampled grid
  vtkCellData *fieldData = g->GetCellData();
  assert( "pre: Target PointData is NULL!" && (fieldData != NULL) );

  // STEP 3: Initialize the fields on the resampled grid
  this->InitializeFields( fieldData, g->GetNumberOfCells(), CD );

  if(fieldData->GetNumberOfArrays() == 0)
    {
    return;
    }

  // TODO: this is a very naive implementation and should be optimized. However,
  // mostly this filter is used to transfer the solution to the grid nodes and
  // not on the cell nodes.
  vtkIdType cellIdx = 0;
  for( ; cellIdx < g->GetNumberOfCells(); ++cellIdx )
    {
    double qPoint[3];
    this->ComputeCellCentroid( g, cellIdx, qPoint );

    unsigned int level=0;
    for( ; level < amrds->GetNumberOfDataSets( level ); ++level )
      {
      unsigned int dataIdx = 0;
      for( ; dataIdx < amrds->GetNumberOfDataSets( level ); ++dataIdx )
        {
          int donorCellIdx = -1;
          vtkUniformGrid *donorGrid = amrds->GetDataSet(level,dataIdx);
          if( (donorGrid!=NULL) &&
              this->FoundDonor(qPoint,donorGrid,donorCellIdx) )
            {
            assert( "pre: donorCellIdx is invalid" &&
                    (donorCellIdx >= 0) &&
                    (donorCellIdx < donorGrid->GetNumberOfCells()) );
            CD = donorGrid->GetCellData();
            this->CopyData( fieldData, cellIdx, CD, donorCellIdx );
            } // END if
        } // END for all datasets
      } // END for all levels
    } // END for all cells
}

//-----------------------------------------------------------------------------
bool vtkAMRResampleFilter::SearchForDonorGridAtLevel(
    double q[3], vtkOverlappingAMR *amrds,
    unsigned int level, unsigned int& donorGridId,
    int &donorCellIdx )
{
  assert( "pre: AMR dataset is NULL" && (amrds != NULL) );
  this->NumberOfBlocksTestedForLevel = 0;
  std::ostringstream oss;
  oss << "SearchLevel-" << level;

  vtkTimerLog::MarkStartEvent( oss.str().c_str() );

  for(donorGridId = 0; donorGridId < amrds->GetNumberOfDataSets(level); ++donorGridId )
    {
    donorCellIdx = -1;
    this->NumberOfBlocksTestedForLevel++;
    if(amrds->GetAMRInfo()->FindCell(q, level,donorGridId,donorCellIdx))
     {
     assert( "pre: donorCellIdx is invalid" &&
             (donorCellIdx >= 0));//  &&
             // (donorCellIdx < donorGrid->GetNumberOfCells()) );
     vtkTimerLog::MarkEndEvent( oss.str().c_str() );
     return true;
     } // END if

    } // END for all data at level


  // No suitable grid is found at the requested level, set donorGrid to NULL
  // to indicate that to the caller.
  vtkTimerLog::MarkEndEvent( oss.str().c_str() );
  return false;
}

//-----------------------------------------------------------------------------
int vtkAMRResampleFilter::ProbeGridPointInAMR(
  double q[3], unsigned int &donorLevel, unsigned int& donorGridId,
    vtkOverlappingAMR *amrds, unsigned int maxLevel, bool hadDonorGrid)
{
  assert( "pre: AMR dataset is NULL" && amrds != NULL );

  vtkUniformGrid *currentGrid = NULL;
  int currentCellIdx          = -1;
  int donorCellIdx            = -1;
  unsigned int currentLevel = 0;
  unsigned int currentGridId = 0;
  vtkUniformGrid* donorGrid = hadDonorGrid? amrds->GetDataSet(donorLevel,donorGridId): NULL;

  // STEP 0: Check the previously cached donor-grid
  if( hadDonorGrid)
    {
    this->NumberOfBlocksTested++;
    bool res(true);
    if(!amrds->GetAMRInfo()->FindCell( q, donorLevel, donorGridId, donorCellIdx ) )
      {
      // Lets see if the point is contained by a grid at the same donar level
      res = this->SearchForDonorGridAtLevel(q,amrds,donorLevel,donorGridId,
                                                 donorCellIdx);
      donorGrid = res? amrds->GetDataSet(donorLevel,donorGridId) : NULL;
      this->NumberOfBlocksTested += this->NumberOfBlocksTestedForLevel;
      }

    // If donorGrid is still not NULL then we found the grid and potential starting
    // level
    if (res)
      {
      assert( "pre: donorCellIdx is invalid" &&
              (donorCellIdx >= 0) && (donorCellIdx < donorGrid->GetNumberOfCells()) );

      this->NumberOfTimesFoundOnDonorLevel++;

      // Initialize values for step 1 s.t. that the search will start from the
      // current donorLevel
      currentGrid    = donorGrid;
      currentGridId = donorGridId;
      currentCellIdx = donorCellIdx;
      currentLevel = donorLevel;
      assert(!donorGrid || amrds->GetDataSet(donorLevel,donorGridId)==donorGrid);
      }
    else if (donorLevel == 0)
      {
      //if we are here then the point is not contained in any of the level 0
      // blocks!
      this->NumberOfFailedPoints++;
      donorGrid    = NULL;
      donorLevel = 0;
      return -1;
      }
    else
      {
      // If we are here then we know the point is not on the donor level
      // and therefore not contained in any of the more refined levels -
      // Base on the assumption of overlapping AMR
      assert("pre:Donor Level is 0" && donorLevel != 0);
      // Initialize values for step 1 s.t. the search will start from level 0.
      donorGrid  = NULL;
      maxLevel = donorLevel;
      donorLevel = 0;
      currentLevel = 0;
      }
    }

  // If we didn't have an initial donor grid or if we still have one
  // we need to test higher res grids
  int startLevel, endLevel;
  int incLevel;
  if (!((donorGrid == NULL) && hadDonorGrid))
    {
    startLevel = donorGrid==NULL? currentLevel : currentLevel+1;
    endLevel = maxLevel;
    incLevel = 1;
    }
  else
    {
    startLevel = maxLevel-1;
    endLevel = -1;
    incLevel = -1;
    }
  // STEP 1: Search in the AMR hierarchy for the donor-grid
  for( int level=startLevel; level != endLevel; level += incLevel )
    {
    if (incLevel == 1)
      {
      this->NumberOfTimesLevelUp++;
      }
    else
      {
      this->NumberOfTimesLevelDown++;
      }
    bool res = this->SearchForDonorGridAtLevel(q,amrds,level,donorGridId,donorCellIdx);
    donorGrid = res? amrds->GetDataSet(level,donorGridId) : NULL;

    this->NumberOfBlocksTested += this->NumberOfBlocksTestedForLevel;
    if( res )
      {
      donorLevel = level;
      // if we are going from fine to coarse then we can stop the search
      if (incLevel == -1)
        {
        assert(amrds->GetDataSet(donorLevel,donorGridId)==donorGrid);
        return donorCellIdx;
        }

      // Lets see if this is the highest resolution grid that contains the
      // point
      if (donorGrid->IsCellVisible(donorCellIdx))
        {
        //return donorCellIdx;
        }
      // we found a grid that contains the point at level l, let's store it
      // here temporatily in case there is a grid at a higher resolution that
      // we need to use.
      currentGrid    = donorGrid;
      currentCellIdx = donorCellIdx;
      currentLevel = level;
      currentGridId = donorGridId;
      }
    else if( currentGrid != NULL )
      {
      // we did not find the point at a higher res, but, we did find at a lower
      // resolution, so we will use the solution we found previously
      // THIS SHOULD NOW NOT HAPPEN!!
      //vtkErrorMacro("Could not find point in an unblanked cell.");
      this->NumberOfBlocksVisSkipped +=  this->NumberOfBlocksTestedForLevel;
      donorGrid    = currentGrid;
      donorCellIdx = currentCellIdx;
      donorLevel = currentLevel;
      donorGridId = currentGridId;
      assert(!donorGrid || amrds->GetDataSet(donorLevel,donorGridId)==donorGrid);
      break;
      }
    else
      {
      // we are not able to find a grid/cell that contains the query point, in
      // this case we will just return.
      this->NumberOfFailedPoints++;
      donorCellIdx = -1;
      donorGrid    = NULL;
      donorLevel = 0;
      break;
      }
    } // END for all levels
  assert(!donorGrid || amrds->GetDataSet(donorLevel,donorGridId)==donorGrid);
  return( donorCellIdx );
}

//-----------------------------------------------------------------------------
bool vtkAMRResampleFilter::SearchGridAncestors(double q[3],
                                               vtkOverlappingAMR *amrds,
                                               unsigned int &level,
                                               unsigned int &gridId,
                                               int &cellId )
{
  assert( "pre: AMR dataset is NULL" && (amrds != NULL) );
  unsigned int *parents, plevel;
  for (; level > 0; --level)
    {
    ++this->NumberOfTimesLevelUp;
    // Get the parents of the grid

    unsigned int numParents;
    parents = amrds->GetParents(level, gridId,numParents);
    plevel = level - 1;
    // There should be at least 1 parent
    assert( "Found non-level 0 grid with no parents" && (parents != NULL) && (numParents > 0) );
    if (numParents > 1)
      {
      vtkDebugMacro( "Number of parents: " << numParents << " - Only processing 1 route");
      }
    gridId = parents[0];
    if (amrds->GetAMRInfo()->FindCell( q, plevel,gridId, cellId ))
      {
      level = plevel;
      return true;
      }
    }
  // If we are here then we could not find an ancestor
  cellId = -1;
  return false;
}

//-----------------------------------------------------------------------------
void vtkAMRResampleFilter::SearchGridDecendants(double q[3],
                                                vtkOverlappingAMR *amrds,
                                                unsigned int maxLevel,
                                                unsigned int &level,
                                                unsigned int &gridId,
                                                int &cellId)
{
  assert( "pre: AMR dataset is NULL" && (amrds != NULL) );
  unsigned int *children, clevel, n, i;
  for (; level < maxLevel-1; ++level)
    {
    // Get the children of the grid
    children = amrds->GetChildren(level, gridId,n);
    clevel = level + 1;
    // If there are no children then we found the grid!
    if (children == NULL)
      {
      return;
      }
//    assert(n == children[0]);
    for (i = 0; i < n; ++i)
      {
      if (amrds->GetAMRInfo()->FindCell( q, clevel,children[i], cellId ))
        {
        // We found a decendant so stop searching the
        // children and can instead search that grid's
        // children
        gridId = children[i];
        ++this->NumberOfTimesLevelDown;
        break;
        }
      }
    if (i >= n)
      {
      // We tested some children that we didn't need to if
      // we had visibility info
      this->NumberOfBlocksVisSkipped += n;
      // If we are here then no child contains the point
      // so don't search any further
      return;
      }
    }
}



//-----------------------------------------------------------------------------
int vtkAMRResampleFilter::
ProbeGridPointInAMRGraph(double q[3],
                         unsigned int &donorLevel,  unsigned int &donorGridId,
                         vtkOverlappingAMR *amrds, unsigned int maxLevel, bool useCached)
{
  assert( "pre: AMR dataset is NULL" && amrds != NULL );

  int donorCellIdx = -1;

  vtkUniformGrid* donorGrid = NULL;
  // STEP 0: Check the previously cached donor-grid
  if( useCached)
    {
    if(!amrds->GetAMRInfo()->FindCell( q, donorLevel,donorGridId, donorCellIdx ) )
      {
      // Lets find the grid's ancestor that contains the point
      bool res = this->SearchGridAncestors(q,amrds,donorLevel,donorGridId,donorCellIdx);
      donorGrid  = res? amrds->GetDataSet(donorLevel,donorGridId) : NULL;
      }
    else
      {
      donorGrid  = amrds->GetDataSet(donorLevel,donorGridId);
      ++this->NumberOfTimesFoundOnDonorLevel;
      }
    // if the point is not contained in an ancestor then lets just assume its on level
    // 0 which is the default
    }

  // If there is no initial donor grid then search level 0
  if (donorGrid == NULL)
    {
    bool res = SearchForDonorGridAtLevel(q,amrds,0,donorGridId,donorCellIdx);
    // If we still can't find a grid then the point is not contained in the
    // AMR Data
    if (!res)
      {
      this->NumberOfFailedPoints++;
      donorLevel = 0;
      return -1;
      }
    }

  // Now search the decendants of the donor grid
  this->SearchGridDecendants(q,amrds,maxLevel,donorLevel,donorGridId,donorCellIdx);
  return( donorCellIdx );
}


//-----------------------------------------------------------------------------
void vtkAMRResampleFilter::TransferToGridNodes(
    vtkUniformGrid *g, vtkOverlappingAMR *amrds )
{
  this->NumberOfBlocksTested = 0;
  this->NumberOfBlocksVisSkipped = 0;
  this->NumberOfTimesFoundOnDonorLevel = 0;
  this->NumberOfTimesLevelUp = 0;
  this->NumberOfTimesLevelDown = 0;
  this->NumberOfFailedPoints = 0;
  this->AverageLevel = 0.0;
  assert( "pre: uniform grid is NULL" && (g != NULL) );
  assert( "pre: AMR data-structure is NULL" && (amrds != NULL) );

  // STEP 0: Initialize the fields on the grid
  vtkUniformGrid *refGrid = this->GetReferenceGrid( amrds );

  vtkCellData *CD = refGrid->GetCellData();
  assert( "pre: Donor CellData is NULL!" && (CD != NULL)  );

  vtkPointData *PD = g->GetPointData();
  assert( "pre: Target PointData is NULL!" && (PD != NULL) );

  // STEP 0: Initialize the fields on the grid
  this->InitializeFields( PD, g->GetNumberOfPoints(), CD );

  // STEP 1: If no arrays are selected, there is no need to interpolate
  // anything on the grid, just return
  if(PD->GetNumberOfArrays() == 0)
    {
    return;
    }

  // STEP 2: Fix the maximum level at which the search algorithm will operate
  unsigned int maxLevelToLoad = 0;
  if( this->LevelOfResolution < static_cast<int>(amrds->GetNumberOfLevels()) &&
    this->DemandDrivenMode == 1)
    {
    maxLevelToLoad = this->LevelOfResolution+1;
    }
  else
    {
    maxLevelToLoad = amrds->GetNumberOfLevels();
    }

  // STEP 3: Loop through all the points and find the donors.
  int numPoints           = 0;
  unsigned int donorLevel = 0;
  unsigned int donorGridId = 0;
  double qPoint[3];
  vtkIdType pIdx;
  int donorCellIdx;
  bool useCached(false);
  // Do we have parent/child meta information (yes, we always do)
  if (this->AMRMetaData)
    {
    for(pIdx = 0; pIdx < g->GetNumberOfPoints(); ++pIdx )
      {
      g->GetPoint( pIdx, qPoint );
      donorCellIdx =
        this->ProbeGridPointInAMRGraph(qPoint,
                                       donorLevel, donorGridId,
                                       amrds,maxLevelToLoad, useCached);
      if( donorCellIdx != -1 )
        {
        useCached = true;
        vtkUniformGrid *amrGrid = amrds->GetDataSet(donorLevel,donorGridId);
        this->AverageLevel += donorLevel;
        CD = amrGrid->GetCellData();
        this->CopyData( PD, pIdx, CD, donorCellIdx );
        }
      else
        {
        useCached = false;
        // Point is outside the domain, blank it
        ++ numPoints;
        g->BlankPoint( pIdx );
        }
      } // END for all grid nodes
    }
  else
    {
    for(pIdx = 0; pIdx < g->GetNumberOfPoints(); ++pIdx )
      {
      g->GetPoint( pIdx, qPoint );

      donorCellIdx =
        this->ProbeGridPointInAMR(qPoint,donorLevel,donorGridId,
                                  amrds,maxLevelToLoad, useCached);

      if( donorCellIdx != -1 )
        {
        useCached = true;
        this->AverageLevel += donorLevel;
        vtkUniformGrid*  donorGrid = amrds->GetDataSet(donorLevel,donorGridId);
        assert(donorGrid != NULL);
        CD = donorGrid->GetCellData();
        this->CopyData( PD, pIdx, CD, donorCellIdx );
        }
      else
        {
        useCached = false;
        // Point is outside the domain, blank it
        ++ numPoints;
        g->BlankPoint( pIdx );
        }
      } // END for all grid nodes
    }
  std::cerr << "********* Resample Stats *************\n";
  double c = this->NumberOfSamples[0] * this->NumberOfSamples[1] * this->NumberOfSamples[2];
  double b = g->GetNumberOfPoints();
  std::cerr << "Number of Requested Points: " << c << " Number of Actual Points: " << b << "\n";
  std::cerr << " Percentage of Requested Points in Grid: " << 100.0 * b / c << "\n";
  std::cerr << "Total Number of Blocks Tested: " << this->NumberOfBlocksTested << "\n";
  std::cerr << " Number of Blocks that could be skipped by Visibility: " << this->NumberOfBlocksVisSkipped << "\n";
  double a = 100.0 * (double)(this->NumberOfBlocksVisSkipped) / (double) this->NumberOfBlocksTested;
  std::cerr << "Percentage of Blocks skipped via Visibility: " << a << "\n";
  a = (double) this->NumberOfBlocksTested / b;
  std::cerr << "Ave Number of Blocks Tested per Point: " << a << "\n";
  a = 100.0 * (double) this->NumberOfTimesFoundOnDonorLevel / b;
  std::cerr << "Percentage of Times we found point on Previous Level: " << a << "\n";
  a = 100.0 * (double) this->NumberOfTimesLevelUp / b;
  std::cerr << "Percentage of Times went to finer level: " << a << "\n";
  a = 100.0 * (double) this->NumberOfTimesLevelDown / b;
  std::cerr << "Percentage of Times went to coarser level: " << a << "\n";
  a = this->AverageLevel / b;
  std::cerr << "Average Level: " << a << "\n";
  std::cerr << "Number Of Failed Points: " << this->NumberOfFailedPoints << "\n";

}

//-----------------------------------------------------------------------------
void vtkAMRResampleFilter::TransferSolution(
    vtkUniformGrid *g, vtkOverlappingAMR *amrds)
{
  assert( "pre: uniform grid is NULL" && (g != NULL) );
  assert( "pre: AMR data-strucutre is NULL" && (amrds != NULL) );

  if( this->TransferToNodes == 1 )
    {
    this->TransferToGridNodes( g, amrds );
    }
  else
    {
    this->TransferToCellCenters( g, amrds );
    }
}

//-----------------------------------------------------------------------------
void vtkAMRResampleFilter::ExtractRegion(
    vtkOverlappingAMR *amrds, vtkMultiBlockDataSet *mbds,
    vtkOverlappingAMR * vtkNotUsed(metadata) )
{

  assert( "pre: input AMR data-structure is NULL" && (amrds != NULL) );
  assert( "pre: resampled grid should not be NULL" && (mbds != NULL) );

//  std::cout << "NumBlocks: " << this->ROI->GetNumberOfBlocks() << std::endl;
//  std::cout << "NumProcs: "  << this->Controller->GetNumberOfProcesses() << std::endl;
//  std::cout.flush();

  assert( !this->Controller || ( "pre: NumProcs must be less than or equal to NumBlocks" &&
                                 ( static_cast<int>(this->ROI->GetNumberOfBlocks()) <= this->Controller->GetNumberOfProcesses())));

  mbds->SetNumberOfBlocks( this->ROI->GetNumberOfBlocks( ) );
  for( unsigned int block=0; block < this->ROI->GetNumberOfBlocks(); ++block )
    {
    if( this->IsRegionMine( block ) )
      {
      vtkUniformGrid *grid = vtkUniformGrid::New();
      grid->ShallowCopy( this->ROI->GetBlock( block ) );
      this->TransferSolution( grid, amrds );
      mbds->SetBlock( block, grid );
      grid->Delete();
      }
    else
      {
      mbds->SetBlock( block, NULL );
      }
    } // END for all blocks

}

//-----------------------------------------------------------------------------
void vtkAMRResampleFilter::ComputeAMRBlocksToLoad(
    vtkOverlappingAMR *metadata )
{
  assert( "pre: metadata is NULL" && (metadata != NULL) );

  this->BlocksToLoad.clear();

  unsigned int maxLevelToLoad = 0;
  if( this->LevelOfResolution < static_cast<int>(metadata->GetNumberOfLevels()))
    {
    maxLevelToLoad = this->LevelOfResolution+1;
    }
  else
    {
    maxLevelToLoad = metadata->GetNumberOfLevels();
    }

  unsigned int level=0;
  for( ;level < maxLevelToLoad; ++level )
    {
    unsigned int dataIdx = 0;
    for( ; dataIdx < metadata->GetNumberOfDataSets( level ); ++dataIdx )
      {
      double grd[6];
      metadata->GetBounds(level,dataIdx,grd);
      if( this->IsBlockWithinBounds( grd ) )
        {
        this->BlocksToLoad.push_back(
        metadata->GetCompositeIndex(level,dataIdx) );
        } // END check if the block is within the bounds of the ROI
      } // END for all data
    } // END for all levels

   std::sort( this->BlocksToLoad.begin(), this->BlocksToLoad.end() );
   cerr << "Number Levels Loaded = " << maxLevelToLoad << " Number of Blocks = " << BlocksToLoad.size() << "\n";
}

//-----------------------------------------------------------------------------
void vtkAMRResampleFilter::GetDomainParameters(
    vtkOverlappingAMR *amr,
    double domainMin[3], double domainMax[3], double h[3],
    int dims[3], double &rf )
{
  assert( "pre: AMR dataset is NULL!" && (amr != NULL) );

  rf = amr->GetRefinementRatio(1);
  amr->GetAMRInfo()->GetAMRBox(0,0).GetNumberOfNodes(dims);
  amr->GetMin(domainMin);
  amr->GetMax(domainMax);

  amr->GetSpacing(0,h);
}

//-----------------------------------------------------------------------------
void vtkAMRResampleFilter::SnapBounds(
    const double* vtkNotUsed(h0[3]),
    const double domainMin[3],
    const double domainMax[3],
    const int* vtkNotUsed(dims[3]), bool outside[6] )
{
  int i, j;
  for(i=0, j=0; i < 3; ++i )
    {
    // Snap the parts of the bounds that lie outside of the AMR data
    if (this->Min[i] < domainMin[i])
      {
      outside[j++] = true;
      this->GridMin[i] = domainMin[i];
      }
    else
      {
      outside[j++] = false;
      this->GridMin[i] = this->Min[i];
      }

    if (this->Max[i] > domainMax[i])
      {
      outside[j++] = true;
      this->GridMax[i] = domainMax[i];
      }
    else
      {
      outside[j++] = false;
      this->GridMax[i] = this->Max[i];
      }
    }
}

//-----------------------------------------------------------------------------
void vtkAMRResampleFilter::ComputeLevelOfResolution(
    const int N[3], const double h0[3], const double L[3], const double rf )
{
  this->LevelOfResolution = 0;
  for( int i=0; i < 3; ++i )
    {
    double c1        = ( ( N[i]*h0[i] )/L[i] );
    int currentLevel = vtkMath::Floor( 0.5+(log(c1)/log(rf)) );
    if( currentLevel > this->LevelOfResolution )
      {
      this->LevelOfResolution = currentLevel;
      }
    } // END for all i
  std::cerr << "Requested Max Level = " << this->LevelOfResolution << "\n";
}

//-----------------------------------------------------------------------------
bool vtkAMRResampleFilter::RegionIntersectsWithAMR(
    double domainMin[3], double domainMax[3],
    double regionMin[3], double regionMax[3])
{
  vtkBoundingBox domain;
  domain.SetMinPoint( domainMin );
  domain.SetMaxPoint( domainMax );

  vtkBoundingBox region;
  region.SetMinPoint( regionMin );
  region.SetMaxPoint( regionMax );

  if( domain.Intersects(region) )
    {
    return true;
    }

  return false;
}

//-----------------------------------------------------------------------------
void vtkAMRResampleFilter::AdjustNumberOfSamplesInRegion(
    const double Rh[3],
    const bool outside[6], int N[3] )
{
  int startIndex;
  int endIndex;
  double dx = 0.0;
  for( int i=0; i < 3; ++i )
    {
    N[i] = this->NumberOfSamples[i];

    // Get ijk of the snapped bounding wrt the requested virtual grid.
    if( outside[i*2] || outside[i*2+1] )
      {
      dx = this->GridMin[i]-this->Min[i];
      if (dx > 0.0)
        {
        startIndex = static_cast<int>( dx/Rh[i]+1 );
        }
      else
        {
        startIndex = 0;
        }

      dx  = this->GridMax[i]-this->Min[i];
      endIndex = static_cast<int>( dx/Rh[i]+1 );

      if (endIndex > N[i])
        {
        endIndex = N[i];
        }
      int newN = endIndex - startIndex +1;
      if (newN <= N[i])
        {
        N[i] = newN;
        }
      else
        {
        assert("ERROR: code should not reach here!" && false );
        }
      }
    }
  std::cerr << "Request Grid Dim : " << this->NumberOfSamples[0] << ", "  << this->NumberOfSamples[1] << ", "  << this->NumberOfSamples[2] << "\n";
  std::cerr << "Computed Grid Dim: " << N[0] << ", "  << N[1] << ", "  << N[2] << "\n";
  if (this->UseBiasVector)
    {
    double a[3];
    a[0] = fabs(this->BiasVector[0]);
    a[1] = fabs(this->BiasVector[1]);
    a[2] = fabs(this->BiasVector[2]);

    // Find the max component
    int bdir =
      (a[0] > a[1]) ? ((a[0] > a[2]) ? 0 : 2) : ((a[1] > a[2]) ? 1 : 2);

    if (bdir == 0)
      {
      N[0] = std::min(N[0], std::max(N[1], N[2]));
      }
    else if (bdir == 1)
      {
      N[1] = std::min(N[1], std::max(N[0], N[2]));
      }
    else
      {
      N[2] = std::min(N[2], std::max(N[0], N[1]));
      }
    std::cerr << "Adjusted Grid Dim: " << N[0] << ", "  << N[1] << ", "  << N[2] << "\n";
    }
}

//-----------------------------------------------------------------------------
void vtkAMRResampleFilter::ComputeAndAdjustRegionParameters(
    vtkOverlappingAMR *amrds, double h[3] )
{
  assert( "pre: AMR dataset is NULL" && (amrds != NULL) );

  // STEP 0: Get domain parameters from root level metadata
  int dims[3];
  double h0[3];
  double domainMin[3];
  double domainMax[3];
  double rf;
  this->GetDomainParameters( amrds, domainMin, domainMax, h0, dims, rf );

  // STEP 1: Check to see if the requested region intersects the AMR domain
  if( !this->RegionIntersectsWithAMR(domainMin, domainMax, this->Min, this->Max))
    {
    h[0]=h[1]=h[2]=0.0;
    return;
    }

  // STEP 3: Get requested region parameters
  double L0[3]; // initial length of each box side
  double Rh[3]; // initial spacing based on the number of samples requested.
  for( int i=0; i < 3; ++i )
    {
    L0[i] = this->Max[i]-this->Min[i];
    Rh[i] = L0[i]/(this->NumberOfSamples[i]-1);
    }

// DEBUG
//  this->WriteUniformGrid(this->Min,this->NumberOfSamples,Rh,"RequestedGrid" );
// END DEBUG
// DEBUG
//  this->WriteUniformGrid(domainMin,dims,h0,"RootGrid");
// END DEBUG

  // STEP 4: Snap region to domain bounds
  bool outside[6];
  // Determine the Min/Max of the computed grid
  this->SnapBounds(h0, domainMin, domainMax,dims,outside);

  // STEP 5: Compute grid parameters on the snapped region
  double L[3];
  for( int i=0; i < 3; ++i )
    {
    L[i] = this->GridMax[i]-this->GridMin[i];
    h[i] = L[i]/(this->NumberOfSamples[i]-1);
    }

// DEBUG
//  this->WriteUniformGrid(min,this->NumberOfSamples,h,"SnappedGrid");
// END DEBUG

  // STEP 6: Adjust N according to how much of the requested region is cropped
  int N[3];
  this->AdjustNumberOfSamplesInRegion(Rh, outside, N );


  // STEP 7: Adjust region parameters
  for( int i=0; i < 3; ++i )
    {
    this->GridNumberOfSamples[i] = (N[i] > 1)? N[i] : 2;
    h[i] = L[i]/(this->GridNumberOfSamples[i]-1);
    }

  this->ComputeLevelOfResolution(this->GridNumberOfSamples,h0,L,rf);
}

//-----------------------------------------------------------------------------
void vtkAMRResampleFilter::GetRegion( double h[3] )
{
  assert( "pre: Region of interest is NULL!" && (this->ROI != NULL) );

  unsigned int block = 0;
  for( ; block < this->ROI->GetNumberOfBlocks(); ++block  )
    {
    this->ROI->RemoveBlock( block );
    }

  if( h[0]==0.0 && h[1]==0.0 && h[2]==0.0 )
    {
    return;
    }

  vtkUniformGrid *grd = vtkUniformGrid::New();
  grd->SetOrigin( this->GridMin );
  grd->SetSpacing( h );
  grd->SetDimensions( this->GridNumberOfSamples );
  if( grd ->GetNumberOfPoints() == 0 )
    {
    vtkErrorMacro( "Empty Grid!" );
    return;
    }

  vtkUniformGridPartitioner *gridPartitioner = vtkUniformGridPartitioner::New();
  gridPartitioner->SetInputData( grd );
  grd->Delete();

  gridPartitioner->SetNumberOfPartitions( this->NumberOfPartitions );
  gridPartitioner->Update();

  this->ROI->DeepCopy( gridPartitioner->GetOutput() );

  gridPartitioner->Delete();
}

//-----------------------------------------------------------------------------
bool vtkAMRResampleFilter::GridsIntersect( double *g1, double *g2 )
{
  assert( "pre: g1 is NULL" && (g1 != NULL) );
  assert( "pre: g2 is NULL" && (g2 != NULL) );

  vtkBoundingBox b1;
  b1.SetBounds( g1);

  vtkBoundingBox b2;
  b2.SetBounds( g2 );

  if( b1.IntersectBox( b2 ) )
    {
    return true;
    }

  return false;
}

//-----------------------------------------------------------------------------
bool vtkAMRResampleFilter::IsBlockWithinBounds( double *grd )
{
  assert( "pre: Input AMR grid is NULL" && (grd != NULL) );

  for( unsigned int block=0; block < this->ROI->GetNumberOfBlocks(); ++block )
    {
    if( this->IsRegionMine( block ) )
      {
      vtkUniformGrid *blk =
          vtkUniformGrid::SafeDownCast( this->ROI->GetBlock( block ) );
      assert( "pre: block is NULL" && (blk != NULL) );

      if( this->GridsIntersect( grd, blk->GetBounds() ) )
        {
        return true;
        }
      } // END if region is mine
    } // END for all blocks

  return false;
}

//-----------------------------------------------------------------------------
int vtkAMRResampleFilter::GetRegionProcessId( const int regionIdx )
{
  if( !this->IsParallel() )
    {
    return 0;
    }

  int N = this->Controller->GetNumberOfProcesses();
  return( regionIdx%N );
}

//-----------------------------------------------------------------------------
bool vtkAMRResampleFilter::IsRegionMine( const int regionIdx )
{
  if( !this->IsParallel() )
    {
    return true;
    }

  int myRank = this->Controller->GetLocalProcessId();
  if( myRank == this->GetRegionProcessId( regionIdx ) )
    {
    return true;
    }

  return false;
}

//-----------------------------------------------------------------------------
bool vtkAMRResampleFilter::IsParallel()
{
  if( this->Controller == NULL )
    {
    return false;
    }

  if( this->Controller->GetNumberOfProcesses() > 1 )
    {
    return true;
    }

  return false;
}

//-----------------------------------------------------------------------------
vtkUniformGrid* vtkAMRResampleFilter::GetReferenceGrid(
    vtkOverlappingAMR *amrds)
{
  assert( "pre:AMR dataset is  NULL" && (amrds != NULL) );

  unsigned int numLevels = amrds->GetNumberOfLevels();
  for(unsigned int l=0; l < numLevels; ++l )
    {
    unsigned int numDatasets = amrds->GetNumberOfDataSets( l );
    for( unsigned int dataIdx=0; dataIdx < numDatasets; ++dataIdx )
      {
      vtkUniformGrid *refGrid = amrds->GetDataSet( l, dataIdx );
      if( refGrid != NULL )
        {
        return( refGrid );
        }
      } // END for all datasets
    } // END for all number of levels

  // This process has no grids
  return NULL;
}

// //-----------------------------------------------------------------------------
// void vtkAMRResampleFilter::WriteUniformGrid(
//     double origin[3], int dims[3], double h[3],
//     std::string prefix )
// {
//   vtkUniformGrid *grd = vtkUniformGrid::New();
//   grd->SetOrigin( origin );
//   grd->SetSpacing( h );
//   grd->SetDimensions( dims );

//   this->WriteUniformGrid( grd, prefix );
//   grd->Delete();
// }

// //-----------------------------------------------------------------------------
// void vtkAMRResampleFilter::WriteUniformGrid(
//     vtkUniformGrid *g, std::string prefix )
// {
//   assert( "pre: Uniform grid (g) is NULL!" && (g != NULL) );

//   vtkXMLImageDataWriter *imgWriter = vtkXMLImageDataWriter::New();

//   std::ostringstream oss;
//   oss << prefix << "." << imgWriter->GetDefaultFileExtension();
//   imgWriter->SetFileName( oss.str().c_str() );
//   imgWriter->SetInputData( g );
//   imgWriter->Write();

//   imgWriter->Delete();
// }
