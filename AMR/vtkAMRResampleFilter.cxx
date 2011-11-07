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
#include "vtkAMRBox.h"
#include "vtkObjectFactory.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkHierarchicalBoxDataSet.h"
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
#include "vtkExtentRCBPartitioner.h"
#include "vtkUniformGridPartitioner.h"

#include <cassert>
#include <cmath>
#include <algorithm>

vtkStandardNewMacro( vtkAMRResampleFilter );

//-----------------------------------------------------------------------------
vtkAMRResampleFilter::vtkAMRResampleFilter()
{
  this->TransferToNodes      = 1;
  this->DemandDrivenMode     = 1;
  this->NumberOfPartitions   = 2;
  this->LevelOfResolution    = 0;
  this->NumberOfSamples[0]   = this->NumberOfSamples[1] = this->NumberOfSamples[2] = 10;
  this->Controller           = vtkMultiProcessController::GetGlobalController();
  this->ROI                  = vtkMultiBlockDataSet::New();
  this->SetNumberOfInputPorts( 1 );
  this->SetNumberOfOutputPorts( 1 );
}

//-----------------------------------------------------------------------------
vtkAMRResampleFilter::~vtkAMRResampleFilter()
{
  this->BlocksToLoad.clear();

  if( this->ROI != NULL )
    this->ROI->Delete();
  this->ROI = NULL;

// Filters should never delete the controller.
//  if( this->Controller != NULL)
//    this->Controller->Delete();
//  this->Controller = NULL;
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
   vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHierarchicalBoxDataSet" );
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
        &this->BlocksToLoad[0], this->BlocksToLoad.size() );
    }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkAMRResampleFilter::RequestInformation(
    vtkInformation* vtkNotUsed(rqst),
    vtkInformationVector **inputVector,
    vtkInformationVector* vtkNotUsed(outputVector) )
{

  assert( "pre: inputVector is NULL" && (inputVector != NULL) );

  vtkInformation *input = inputVector[0]->GetInformationObject( 0 );
  assert( "pre: input is NULL" && (input != NULL)  );

  if( this->DemandDrivenMode == 1 &&
      input->Has(vtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA() ) )
    {
      vtkHierarchicalBoxDataSet *metadata =
        vtkHierarchicalBoxDataSet::SafeDownCast(
          input->Get( vtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA() ) );
      assert( "pre: medata is NULL" && (metadata != NULL)  );

      // Get desired spacing
      double h[3];
//      int level = this->LevelOfResolution;
//      if( level >= metadata->GetNumberOfLevels() )
//        level = metadata->GetNumberOfLevels()-1;
//      vtkUniformGrid *grd = metadata->GetDataSet( level, 0 );
//      assert( "pre: reference grid is NULL" && (grd != NULL) );
//      grd->GetSpacing( h );
      this->SnapBounds( metadata );
      this->GetSpacing( metadata, h );

      // GetRegion
      this->GetRegion( h );

      // Compute which blocks to load
      this->ComputeAMRBlocksToLoad( metadata );
    }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkAMRResampleFilter::RequestData(
    vtkInformation* vtkNotUsed(rqst), vtkInformationVector** inputVector,
    vtkInformationVector* outputVector )
{
  // STEP 0: Get input object
  vtkInformation *input = inputVector[0]->GetInformationObject( 0 );
  assert( "pre: Null information object!" && (input != NULL) );
  vtkHierarchicalBoxDataSet *amrds=
     vtkHierarchicalBoxDataSet::SafeDownCast(
      input->Get(vtkDataObject::DATA_OBJECT()));
  assert( "pre: input AMR dataset is NULL" && (amrds != NULL) );

  vtkHierarchicalBoxDataSet *metadata = NULL;
  // STEP 1: Get Metadata
  if( this->DemandDrivenMode == 1 &&
      input->Has(vtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA() ) )
    {
    metadata = vtkHierarchicalBoxDataSet::SafeDownCast(
      input->Get( vtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA() ) );
    assert( "pre: medata is NULL" && (metadata != NULL)  );
    }
  else
    {
    metadata = amrds;

    // Get desired spacing
    double h[3];
//    int level = this->LevelOfResolution;
//    if( level >= amrds->GetNumberOfLevels() )
//      level = amrds->GetNumberOfLevels()-1;
//    vtkUniformGrid *grd = amrds->GetDataSet( level, 0 );
//    assert( "pre: reference grid is NULL" && (grd != NULL) );
//    grd->GetSpacing( h );
    this->SnapBounds( amrds );
    this->GetSpacing( amrds, h );

    // GetRegion
    this->GetRegion( h );
    }


  // STEP 2: Get output object
  vtkInformation *output = outputVector->GetInformationObject( 0 );
  assert( "pre: Null output information object!" && (output != NULL) );
  vtkMultiBlockDataSet *mbds=
     vtkMultiBlockDataSet::SafeDownCast(
      output->Get(vtkDataObject::DATA_OBJECT()));
  assert( "pre: ouput multi-block dataset is NULL" && (mbds != NULL) );

  // STEP 4: Extract region
  this->ExtractRegion( amrds, mbds, metadata );

  return 1;
}

//-----------------------------------------------------------------------------
bool vtkAMRResampleFilter::FoundDonor(
    double q[3],vtkUniformGrid *donorGrid,int &cellIdx)
{
  assert( "pre: donor grid is NULL" && (donorGrid != NULL) );

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
        vtkUniformGrid *g, vtkHierarchicalBoxDataSet *amrds )
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
   return;

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
void vtkAMRResampleFilter::TransferToGridNodes(
    vtkUniformGrid *g, vtkHierarchicalBoxDataSet *amrds )
{
  assert( "pre: uniform grid is NULL" && (g != NULL) );
  assert( "pre: AMR data-structure is NULL" && (amrds != NULL) );

  vtkUniformGrid *refGrid = this->GetReferenceGrid( amrds );

  vtkCellData *CD = refGrid->GetCellData();
  assert( "pre: Donor CellData is NULL!" && (CD != NULL)  );

  vtkPointData *PD = g->GetPointData();
  assert( "pre: Target PointData is NULL!" && (PD != NULL) );

  // STEP 0: Initialize the fields on the grid
  this->InitializeFields( PD, g->GetNumberOfPoints(), CD );

  if(PD->GetNumberOfArrays() == 0)
    return;

  // STEP 1: Loop through all the points and find the donors.
  vtkIdType pIdx = 0;
  for( ; pIdx < g->GetNumberOfPoints(); ++pIdx )
    {
      double qPoint[3];
      g->GetPoint( pIdx, qPoint );

      unsigned int level=0;
      for( ; level < amrds->GetNumberOfLevels(); ++level )
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
                   this->CopyData( PD, pIdx, CD, donorCellIdx );
                 } // END if

            } // END for all datasets at the current level
        } // END for all levels

    } // END for all grid nodes
}

//-----------------------------------------------------------------------------
void vtkAMRResampleFilter::TransferSolution(
    vtkUniformGrid *g, vtkHierarchicalBoxDataSet *amrds)
{
  assert( "pre: uniform grid is NULL" && (g != NULL) );
  assert( "pre: AMR data-strucutre is NULL" && (amrds != NULL) );

  if( this->TransferToNodes == 1 )
    this->TransferToGridNodes( g, amrds );
  else
    this->TransferToCellCenters( g, amrds );
}

//-----------------------------------------------------------------------------
void vtkAMRResampleFilter::ExtractRegion(
    vtkHierarchicalBoxDataSet *amrds, vtkMultiBlockDataSet *mbds,
    vtkHierarchicalBoxDataSet *metadata )
{
  assert( "pre: input AMR data-structure is NULL" && (amrds != NULL) );
  assert( "pre: output data-structure is NULL" && (mbds != NULL) );
  assert( "pre: metatadata is NULL" && (metadata != NULL) );

  mbds->SetNumberOfBlocks( this->ROI->GetNumberOfBlocks() );
  for( int block=0; block < this->ROI->GetNumberOfBlocks(); ++block )
    {
      if( this->IsRegionMine( block ) )
        {
          vtkUniformGrid *myGrid = vtkUniformGrid::New();
          myGrid->DeepCopy( this->ROI->GetBlock( block ) );
          this->TransferSolution( myGrid, amrds );
          mbds->SetBlock( block, myGrid );
          myGrid->Delete();
        }
      else
        {
          mbds->SetBlock( block, NULL );
        }
    } // END for all blocks

}

//-----------------------------------------------------------------------------
void vtkAMRResampleFilter::ComputeAMRBlocksToLoad( vtkHierarchicalBoxDataSet *metadata )
{
  assert( "pre: metadata is NULL" && (metadata != NULL) );

  this->BlocksToLoad.clear();

  unsigned int maxLevelToLoad = 0;
  if( this->LevelOfResolution < metadata->GetNumberOfLevels() )
    maxLevelToLoad = this->LevelOfResolution+1;
  else
    maxLevelToLoad = metadata->GetNumberOfLevels();

  unsigned int level=0;
  for( ;level < maxLevelToLoad; ++level )
    {
      unsigned int dataIdx = 0;
      for( ; dataIdx < metadata->GetNumberOfDataSets( level ); ++dataIdx )
        {
           vtkUniformGrid *grd = metadata->GetDataSet( level, dataIdx );
           assert( "pre: Metadata grid is NULL" && (grd != NULL) );

           if( this->IsBlockWithinBounds( grd ) )
             {
               this->BlocksToLoad.push_back(
                 metadata->GetCompositeIndex(level,dataIdx) );
             } // END check if the block is within the bounds of the ROI

        } // END for all data
    } // END for all levels

   std::sort( this->BlocksToLoad.begin(), this->BlocksToLoad.end() );

}

//-----------------------------------------------------------------------------
void vtkAMRResampleFilter::GetSpacing(
    vtkHierarchicalBoxDataSet *amr, double h[3] )
{
  assert( "pre: AMR dataset/metadata is NULL" && (amr != NULL) );

  int targetLevel = 0;

  vtkAMRBox amrBox;
  amr->GetMetaData( 0, 0, amrBox );

  // STEP 0: Get initial spacing in each dimension
  double h0[3];
  amrBox.GetGridSpacing( h0 );
  std::cout << "h0: " << h0[0] << " " << h0[1] << " " << h0[2] << std::endl;
  std::cout.flush();

  // STEP 1: Get the refinement ratio -- assumes constant refinement ratio
  double rf = amr->GetRefinementRatio(1);
  std::cout << "Refinement ratio: " << rf << std::endl;
  std::cout.flush();

  // STEP 2: Get length of the box in each dimension
  double L[3];
  L[0] = this->Max[0]-this->Min[0];
  L[1] = this->Max[1]-this->Min[1];
  L[2] = this->Max[2]-this->Min[2];

  // STEP 3: Compute the target AMR level from where the spacing will be comuted
  for( int i=0; i < 3; ++i )
    {
    double c1        = ( ( this->NumberOfSamples[i]*h0[i] )/L[i] );
    int currentLevel = vtkMath::Floor( 0.5+(log(c1)/log(rf)) );
    if( currentLevel > targetLevel )
      targetLevel = currentLevel;
    } // END for all dimensions

  if( targetLevel > amr->GetNumberOfLevels() )
    targetLevel = amr->GetNumberOfLevels()-1;

  // STEP 4: Acquire the spacing at the target level.
  amr->GetMetaData( targetLevel, 0, amrBox );
  amrBox.GetGridSpacing( h );
  this->LevelOfResolution = targetLevel;

  std::cout << "Level of resolution is: " << targetLevel << std::endl;
  std::cout.flush();
}

//-----------------------------------------------------------------------------
void vtkAMRResampleFilter::SnapBounds( vtkHierarchicalBoxDataSet *amrds )
{
  // STEP 0: Ensure AMR dataset is NULL
  assert( "pre: Ensure AMR dataset is NULL" && (amrds != NULL) );

  // STEP 1: Get root AMR box
  vtkAMRBox amrBox;
  amrds->GetRootAMRBox( amrBox );
  amrBox.WriteBox();

  // STEP 2: Snap bounds
  double bbmin[3];
  double bbmax[3];
  double h0[3];
  int dims[3];
  amrBox.GetNumberOfNodes(dims);
  std::cout << "Root AMR box dimensions: " << dims[0] << " " << dims[1] << " " << dims[2];
  std::cout << std::endl;
  amrBox.GetMinBounds( bbmin );
  amrBox.GetMaxBounds( bbmax );
  amrBox.GetGridSpacing( h0 );

  // STEP 3: Process min & max
  int ijkmin[3];
  int ijkmax[3];
  double dx = 0.0;

  for( int i=0; i < 3; ++i )
    {
    dx = this->Min[i]-bbmin[i];
    ijkmin[i]=static_cast<int>( vtkMath::Floor( dx/h0[i] ) );
    --ijkmin[i];
    if( ijkmin[i] < 0 )
      ijkmin[ i ] = 0;

    dx = this->Max[i]-bbmin[i];
    ijkmax[i]=static_cast<int>( vtkMath::Floor( dx/h0[i] ) );
    ++ijkmax[i];
    if( ijkmax[i] > dims[i] )
      ijkmax[i] = dims[i];

    if( ijkmin[i] == 0)
      this->Min[ i ] = bbmin[i];
    else
      this->Min[ i ] = bbmin[i] + ijkmin[i]*h0[i];

    if( ijkmax[i] == 0)
      this->Max[ i ] = bbmin[i]+h0[i];
    else
      this->Max[ i ] = bbmin[i]+ijkmax[i]*h0[i];
    }

//  std::cout << "ijk-min: " << ijkmin[0] << " " << ijkmin[1] << " " << ijkmin[2];
//  std::cout << std::endl;
//  std::cout.flush();
//  assert( "post: ijk-min inside domain" && amrBox.Contains(ijkmin) );
//
//  std::cout << "ijk-max: " << ijkmax[0] << " " << ijkmax[1] << " " << ijkmax[2];
//  std::cout << std::endl;
//  std::cout.flush();
//  assert( "post: ijk-max inside domain" && amrBox.Contains(ijkmax) );
//
//  amrBox.GetPoint( ijkmin, this->Min );
//  amrBox.GetPoint( ijkmax, this->Max );
}

//-----------------------------------------------------------------------------
void vtkAMRResampleFilter::GetRegion( double h[3] )
{
  vtkUniformGrid *grd = vtkUniformGrid::New();

  int dims[3];
  for( int i=0; i < 3; ++i )
      dims[ i ] = ( (this->Max[ i ] - this->Min[ i ] )) / h[ i ];

  grd->SetOrigin( this->Min );
  grd->SetSpacing( h );
  grd->SetDimensions( dims );

  vtkUniformGridPartitioner *gridPartitioner = vtkUniformGridPartitioner::New();
  gridPartitioner->SetInput( grd );
  grd->Delete();

  if( grd ->GetNumberOfPoints() == 0 )
    return;

  gridPartitioner->SetNumberOfPartitions( this->NumberOfPartitions );
  gridPartitioner->Update();

  this->ROI->DeepCopy( gridPartitioner->GetOutput() );

  gridPartitioner->Delete();
}

//-----------------------------------------------------------------------------
bool vtkAMRResampleFilter::GridsIntersect( vtkUniformGrid *g1, vtkUniformGrid *g2 )
{
  assert( "pre: g1 is NULL" && (g1 != NULL) );
  assert( "pre: g2 is NULL" && (g2 != NULL) );

  if( g1->GetNumberOfPoints() == 0 || g2->GetNumberOfPoints() == 0 )
    return false;

  vtkBoundingBox b1;
  b1.SetBounds( g1->GetBounds() );

  vtkBoundingBox b2;
  b2.SetBounds( g2->GetBounds() );

  if( b1.IntersectBox( b2 ) )
    return true;

  return false;
}

//-----------------------------------------------------------------------------
bool vtkAMRResampleFilter::IsBlockWithinBounds( vtkUniformGrid *grd )
{
  assert( "pre: Input AMR grid is NULL" && (grd != NULL) );

  for( int block=0; block < this->ROI->GetNumberOfBlocks(); ++block )
    {
      if( this->IsRegionMine( block ) )
        {
          vtkUniformGrid *blk =
              vtkUniformGrid::SafeDownCast( this->ROI->GetBlock( block ) );
          assert( "pre: block is NULL" && (blk != NULL) );

          if( this->GridsIntersect( grd, blk ) )
            return true;
        } // END if region is mine
    } // END for all blocks

  return false;
}

//-----------------------------------------------------------------------------
int vtkAMRResampleFilter::GetRegionProcessId( const int regionIdx )
{
  if( !this->IsParallel() )
    return 0;

  int N = this->Controller->GetNumberOfProcesses();
  return( regionIdx%N );
}

//-----------------------------------------------------------------------------
bool vtkAMRResampleFilter::IsRegionMine( const int regionIdx )
{
  if( !this->IsParallel() )
    return true;

  int myRank = this->Controller->GetLocalProcessId();
  if( myRank == this->GetRegionProcessId( regionIdx ) )
    return true;
  return false;
}

//-----------------------------------------------------------------------------
bool vtkAMRResampleFilter::IsParallel()
{
  if( this->Controller == NULL )
    return false;

  if( this->Controller->GetNumberOfProcesses() > 1 )
    return true;

  return false;
}

//-----------------------------------------------------------------------------
vtkUniformGrid* vtkAMRResampleFilter::GetReferenceGrid(
    vtkHierarchicalBoxDataSet *amrds)
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
            return( refGrid );
        } // END for all datasets
    } // END for all number of levels

  // This process has no grids
  return NULL;
}
