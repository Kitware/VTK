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
#include "vtkXMLImageDataWriter.h"
#include "vtkExtentRCBPartitioner.h"
#include "vtkUniformGridPartitioner.h"

#include <cassert>
#include <sstream>
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
  this->ROIChanged           = false;
  this->InitialCall          = true;

  for( int i=0; i < 3; ++i )
    {
    this->Min[i] = 0.;
    this->Max[i] = 1.;
    }
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

      // Get Region
      double h[3];
      this->ComputeAndAdjustRegionParameters( metadata, h );
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

    // GetRegion
    double h[3];
    this->ComputeAndAdjustRegionParameters( amrds, h );
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
  for( unsigned int block=0; block < this->ROI->GetNumberOfBlocks(); ++block )
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
  if( this->LevelOfResolution < static_cast<int>(metadata->GetNumberOfLevels()))
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
void vtkAMRResampleFilter::GetDomainParameters(
    vtkHierarchicalBoxDataSet *amr,
    double domainMin[3], double domainMax[3], double h[3],
    int dims[3], double &rf )
{
  assert( "pre: AMR dataset is NULL!" && (amr != NULL) );

  vtkAMRBox amrBox;
  amr->GetRootAMRBox( amrBox );

  rf = amr->GetRefinementRatio(1);

  amrBox.GetNumberOfNodes( dims );
  amrBox.GetMinBounds( domainMin );
  amrBox.GetMaxBounds( domainMax );
  amrBox.GetGridSpacing( h );
}

//-----------------------------------------------------------------------------
void vtkAMRResampleFilter::SnapBounds(
    const double h0[3], const double domainMin[3], const double domainMax[3],
    const int dims[3], double min[3], double max[3],
    int ijkmin[3], int ijkmax[3],
    bool outside[6] )
{
  double dx = 0.0;
  for( int i=0; i < 3; ++i )
    {
    dx = this->Min[i]-domainMin[i];
    ijkmin[i]=static_cast<int>( vtkMath::Round( dx/h0[i] ) );
    --ijkmin[i];
    if( ijkmin[i] < 0 )
      {
      ijkmin[ i ] = 0;
      outside[ i*2 ] = true;
      }
    else
      {
      outside[ i*2 ] = false;
      }

    dx = this->Max[i]-domainMin[i];
    ijkmax[i]=static_cast<int>( vtkMath::Round( dx/h0[i] ) );
    ++ijkmax[i];
    if( ijkmax[i] > dims[i]-1 )
      {
      ijkmax[i]        = dims[i]-1;
      outside[ i*2+1 ] = true;
      }
    else
      {
      outside[ i*2+1 ] = false;
      }

    if( ijkmin[i] == 0)
      min[ i ] = domainMin[i];
    else
      min[ i ] = domainMin[i] + ijkmin[i]*h0[i];
    assert( "post: min is out-of-bounds!" && (min[i] >= domainMin[i]) );

    if( ijkmax[i] == 0)
      max[ i ] = domainMin[i]+h0[i];
    else
      max[ i ] = domainMin[i]+ijkmax[i]*h0[i];
    assert( "post: max is out-of-bounds!" && (max[i] <= domainMax[i]) );
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
      this->LevelOfResolution = currentLevel;
    } // END for all i

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
    return true;
  return false;
}

//-----------------------------------------------------------------------------
void vtkAMRResampleFilter::AdjustNumberOfSamplesInRegion(
    const double min[3], const double max[3], const double Rh[3],
    const bool outside[6], int N[3] )
{
  int ijkRegionMin[3];
  int ijkRegionMax[3];
  double dx = 0.0;
  for( int i=0; i < 3; ++i )
    {
    N[i] = this->NumberOfSamples[i];

    // Get ijk of the snapped bounding wrt the requested virtual grid.
    if( outside[i*2] || outside[i*2+1] )
      {
      dx              = min[i]-this->Min[i];
      ijkRegionMin[i] = static_cast<int>( dx/Rh[i]+1 );
      dx              = max[i]-this->Min[i];
      ijkRegionMax[i] = static_cast<int>( dx/Rh[i]+1 );

      int reduceTo    = ijkRegionMax[i]-ijkRegionMin[i]+1;
      N[i]            = reduceTo;
      }
    }
}

//-----------------------------------------------------------------------------
void vtkAMRResampleFilter::ComputeAndAdjustRegionParameters(
    vtkHierarchicalBoxDataSet *amrds, double h[3] )
{
  assert( "pre: AMR dataset is NULL" && (amrds != NULL) );

  if( this->InitialCall )
    {
    h[0]=h[1]=h[2]=0.0;
    this->InitialCall = false;
    return;
    }

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

  // STEP 2: Check if the ROI has changed from a previous request
  if( !this->ROIChanged )
    {
    // Just update h, e.g., if the number of samples has changed and return.
    for( int i=0; i < 3; ++i )
      {
      double l = this->Max[i] - this->Min[i];
      h[i]     = l/(this->NumberOfSamples[i]-1);
      }
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
  double min[3];
  double max[3];
  int ijkmin[3];
  int ijkmax[3];
  bool outside[6];
  this->SnapBounds(h0, domainMin,
      domainMax,dims, min, max, ijkmin, ijkmax,outside);

  // STEP 5: Compute grid parameters on the snapped region
  double L[3];
  for( int i=0; i < 3; ++i )
    {
    L[i] = max[i]-min[i];
    h[i] = L[i]/(this->NumberOfSamples[i]-1);
    }

// DEBUG
//  this->WriteUniformGrid(min,this->NumberOfSamples,h,"SnappedGrid");
// END DEBUG

  // STEP 6: Adjust N according to how much of the requested region is cropped
  int N[3];
  this->AdjustNumberOfSamplesInRegion( min, max, Rh, outside, N );


  // STEP 7: Adjust region parameters
  for( int i=0; i < 3; ++i )
    {
    this->Min[i]             = min[i];
    this->Max[i]             = max[i];
    this->NumberOfSamples[i] = (N[i] > 1)? N[i] : 2;
    L[i]                     = this->Max[i]-this->Min[i];
    h[i]                     = L[i]/(this->NumberOfSamples[i]-1);
    }

  this->ComputeLevelOfResolution(this->NumberOfSamples,h0,L,rf);
  this->ROIChanged = false;
}

//-----------------------------------------------------------------------------
void vtkAMRResampleFilter::GetRegion( double h[3] )
{
  assert( "pre: Region of interest is NULL!" && (this->ROI != NULL) );

  unsigned int block = 0;
  for( ; block < this->ROI->GetNumberOfBlocks(); ++block  )
    this->ROI->RemoveBlock( block );

  if( h[0]==0.0 && h[1]==0.0 && h[2]==0.0 )
    return;

  vtkUniformGrid *grd = vtkUniformGrid::New();
  grd->SetOrigin( this->Min );
  grd->SetSpacing( h );
  grd->SetDimensions( this->NumberOfSamples );

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

  for( unsigned int block=0; block < this->ROI->GetNumberOfBlocks(); ++block )
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

//-----------------------------------------------------------------------------
void vtkAMRResampleFilter::WriteUniformGrid(
    double origin[3], int dims[3], double h[3],
    std::string prefix )
{
  vtkUniformGrid *grd = vtkUniformGrid::New();
  grd->SetOrigin( origin );
  grd->SetSpacing( h );
  grd->SetDimensions( dims );

  this->WriteUniformGrid( grd, prefix );
  grd->Delete();
}

//-----------------------------------------------------------------------------
void vtkAMRResampleFilter::WriteUniformGrid(
    vtkUniformGrid *g, std::string prefix )
{
  assert( "pre: Uniform grid (g) is NULL!" && (g != NULL) );

  vtkXMLImageDataWriter *imgWriter = vtkXMLImageDataWriter::New();

  std::ostringstream oss;
  oss << prefix << "." << imgWriter->GetDefaultFileExtension();
  imgWriter->SetFileName( oss.str().c_str() );
  imgWriter->SetInput( g );
  imgWriter->Write();

  imgWriter->Delete();
}
