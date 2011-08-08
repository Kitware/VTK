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

#include "vtkAMRToGrid.h"
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

#include <cassert>
#include <algorithm>

vtkStandardNewMacro( vtkAMRToGrid );

//-----------------------------------------------------------------------------
vtkAMRToGrid::vtkAMRToGrid()
{
  this->TransferToNodes      = 1;
  this->LevelOfResolution    = 1;
  this->NumberOfSubdivisions = 0;
//  this->initializedRegion    = false;
//  this->subdividedRegion     = false;
  this->Controller           = vtkMultiProcessController::GetGlobalController();
  this->SetNumberOfInputPorts( 1 );
  this->SetNumberOfOutputPorts( 1 );
}

//-----------------------------------------------------------------------------
vtkAMRToGrid::~vtkAMRToGrid()
{
  this->blocksToLoad.clear();
  this->boxes.clear();

  if( this->Controller != NULL)
    this->Controller->Delete();
  this->Controller = NULL;
}

//-----------------------------------------------------------------------------
void vtkAMRToGrid::PrintSelf( std::ostream &oss, vtkIndent indent )
{
  this->Superclass::PrintSelf( oss, indent );
}

//-----------------------------------------------------------------------------
int vtkAMRToGrid::FillInputPortInformation(
    int vtkNotUsed(port), vtkInformation *info )
{
  assert( "pre: information object is NULL" && (info != NULL) );
  info->Set(
   vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHierarchicalBoxDataSet" );
  return 1;
}

//-----------------------------------------------------------------------------
int vtkAMRToGrid::FillOutputPortInformation(
    int vtkNotUsed(port), vtkInformation *info )
{
  assert( "pre: information object is NULL" && (info != NULL) );
  info->Set(vtkDataObject::DATA_TYPE_NAME(),"vtkMultiBlockDataSet");
  return 1;
}

//-----------------------------------------------------------------------------
int vtkAMRToGrid::RequestUpdateExtent(
    vtkInformation*, vtkInformationVector **inputVector,
    vtkInformationVector* vtkNotUsed(outputVector) )
{
  assert( "pre: inputVector is NULL" && (inputVector != NULL) );
  vtkInformation *info = inputVector[0]->GetInformationObject(0);
  assert( "pre: info is NULL" && (info != NULL) );

  info->Set(
      vtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES(),
      &this->blocksToLoad[0], this->blocksToLoad.size() );

  return 1;
}

//-----------------------------------------------------------------------------
int vtkAMRToGrid::RequestInformation(
    vtkInformation* vtkNotUsed(rqst),
    vtkInformationVector **inputVector,
    vtkInformationVector* vtkNotUsed(outputVector) )
{
  assert( "pre: inputVector is NULL" && (inputVector != NULL) );

  vtkInformation *input = inputVector[0]->GetInformationObject( 0 );
  assert( "pre: input is NULL" && (input != NULL)  );

  if( input->Has(vtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA() ) )
    {
      vtkHierarchicalBoxDataSet *metadata =
        vtkHierarchicalBoxDataSet::SafeDownCast(
          input->Get( vtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA() ) );
      assert( "pre: medata is NULL" && (metadata != NULL)  );

      this->SubdivideExtractionRegion();

      this->ComputeAMRBlocksToLoad( metadata );
    }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkAMRToGrid::RequestData(
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

  // STEP 1: Get output object
  vtkInformation *output = outputVector->GetInformationObject( 0 );
  assert( "pre: Null output information object!" && (output != NULL) );
  vtkMultiBlockDataSet *mbds=
     vtkMultiBlockDataSet::SafeDownCast(
      output->Get(vtkDataObject::DATA_OBJECT()));
  assert( "pre: ouput multi-block dataset is NULL" && (mbds != NULL) );

  // STEP 2: Initialize input
//  this->InitializeRegionBounds(amrds);
  this->SubdivideExtractionRegion();

  // STEP 3: Extract region
  this->ExtractRegion( amrds, mbds );

  return 1;
}

//-----------------------------------------------------------------------------
bool vtkAMRToGrid::FoundDonor(
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
void vtkAMRToGrid::InitializeFields(
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
      array->SetNumberOfTuples( size );
      array->SetNumberOfComponents(
         src->GetArray(arrayIdx)->GetNumberOfComponents() );

      f->AddArray( array );
      array->Delete();
    } // END for all arrays

}

//-----------------------------------------------------------------------------
void vtkAMRToGrid::CopyData(
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
void vtkAMRToGrid::ComputeCellCentroid(
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
void vtkAMRToGrid::TransferToCellCenters(
        vtkUniformGrid *g, vtkHierarchicalBoxDataSet *amrds )
{
  assert( "pre: uniform grid is NULL" && (g != NULL) );
  assert( "pre: AMR data-strucutre is NULL" && (amrds != NULL) );

  // STEP 0: Get the first block so that we know the arrays
  vtkUniformGrid *refGrid = amrds->GetDataSet(0,0);
  assert( "pre: Block(0,0) is NULL!" && (refGrid != NULL) );

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
void vtkAMRToGrid::TransferToGridNodes(
    vtkUniformGrid *g, vtkHierarchicalBoxDataSet *amrds )
{
  assert( "pre: uniform grid is NULL" && (g != NULL) );
  assert( "pre: AMR data-structure is NULL" && (amrds != NULL) );

  vtkUniformGrid *refGrid = amrds->GetDataSet(0,0);
  assert( "pre: Block(0,0) is NULL!" && (refGrid != NULL) );

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
void vtkAMRToGrid::TransferSolution(
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
void vtkAMRToGrid::ExtractRegion(
    vtkHierarchicalBoxDataSet *amrds, vtkMultiBlockDataSet *mbds )
{
  assert( "pre: input AMR data-structure is NULL" && (amrds != NULL) );
  assert( "pre: output data-structure is NULL" && (mbds != NULL) );

  int numBoxes = this->boxes.size()/6;
  mbds->SetNumberOfBlocks( numBoxes );

  unsigned int maxLevelToLoad = 0;
  if( this->LevelOfResolution < amrds->GetNumberOfLevels() )
    maxLevelToLoad = this->LevelOfResolution;
  else
    maxLevelToLoad = amrds->GetNumberOfLevels()-1;

  double spacings[3];
  vtkUniformGrid *dummyGrid = amrds->GetDataSet( maxLevelToLoad, 0 );
  dummyGrid->GetSpacing( spacings );

  for(int box=0; box < numBoxes; ++box )
    {
      double xMin = this->boxes[ box*6 ];
      double yMin = this->boxes[ box*6+1 ];
      double zMin = this->boxes[ box*6+2 ];
      double xMax = this->boxes[ box*6+3 ];
      double yMax = this->boxes[ box*6+4 ];
      double zMax = this->boxes[ box*6+5 ];

      int dims[3];
      dims[0] = ( ( xMax-xMin ) / spacings[0] ) + 1;
      dims[1] = ( ( yMax-yMin ) / spacings[1] ) + 1;
      dims[2] = ( ( zMax-zMin ) / spacings[2] ) + 1;

      vtkUniformGrid *grd = vtkUniformGrid::New();
      grd->SetDimensions( dims );
      grd->SetOrigin( xMin, yMin, zMin );
      grd->SetSpacing( spacings );

      this->TransferSolution( grd, amrds );

      mbds->SetBlock( box, grd );
      grd->Delete();
    } // END for all boxes in the multi-block

}

//-----------------------------------------------------------------------------
void vtkAMRToGrid::ComputeAMRBlocksToLoad( vtkHierarchicalBoxDataSet *metadata )
{
  assert( "pre: metadata is NULL" && (metadata != NULL) );

  this->blocksToLoad.clear();

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
               this->blocksToLoad.push_back(
                 metadata->GetCompositeIndex(level,dataIdx) );
             } // END check if the block is within the bounds of the ROI

        } // END for all data
    } // END for all levels

   std::sort( this->blocksToLoad.begin(), this->blocksToLoad.end() );

}

//-----------------------------------------------------------------------------
bool vtkAMRToGrid::IsBlockWithinBounds( vtkUniformGrid *grd )
{
  assert( "pre: Input AMR grid is NULL" && (grd != NULL) );

  // [xmim,xmax,ymin,ymax,zmin,zmax]
  double gridBounds[6];
  grd->GetBounds( gridBounds );

  vtkBoundingBox gridBoundingBox;
  gridBoundingBox.SetBounds( gridBounds );

  int numBoxes = this->boxes.size()/6;
  for( int box=0; box < numBoxes; ++box )
    {
       // [xmim,xmax,ymin,ymax,zmin,zmax]
       double regionBounds[6];
       regionBounds[0] = this->boxes[ box*6   ]; // xmin
       regionBounds[1] = this->boxes[ box*6+3 ]; // xmax
       regionBounds[2] = this->boxes[ box*6+1 ]; // ymin
       regionBounds[3] = this->boxes[ box*6+4 ]; // ymax
       regionBounds[4] = this->boxes[ box*6+2 ]; // zmin
       regionBounds[5] = this->boxes[ box*6+5 ]; // zmax

       vtkBoundingBox regionBox;
       regionBox.SetBounds( regionBounds );

       if( gridBoundingBox.IntersectBox( regionBox ) )
         return true;
    } // END for all boxes
  return false;
}

//-----------------------------------------------------------------------------
void vtkAMRToGrid::SubdivideExtractionRegion()
{
  this->boxes.resize(0);
//  if( this->subdividedRegion )
//    return;

  // Currently we do not subdivide, and just do this in a single process.
  this->boxes.push_back( this->Min[0] );
  this->boxes.push_back( this->Min[1] );
  this->boxes.push_back( this->Min[2] );

  this->boxes.push_back( this->Max[0] );
  this->boxes.push_back( this->Max[1] );
  this->boxes.push_back( this->Max[2] );

//  this->subdividedRegion = true;
}

//-----------------------------------------------------------------------------
void vtkAMRToGrid::InitializeRegionBounds( vtkHierarchicalBoxDataSet *inp )
{
   assert( "pre: input AMR dataset is NULL" && (inp != NULL) );

//   if( this->initializedRegion )
//     return;

   double bounds[6];
   vtkAMRUtilities::ComputeGlobalBounds( bounds, inp, this->Controller);

   double offset[3];
   for( int i=0; i < 3; ++i )
     offset[ i ] = vtkMath::Floor( (bounds[i+3]-bounds[i])/2.0 );

   this->Min[0]= bounds[0] + vtkMath::Floor( (offset[0]-bounds[0])/2.0 );
   this->Min[1]= bounds[1] + vtkMath::Floor( (offset[1]-bounds[1])/2.0 );
   this->Min[2]= bounds[2] + vtkMath::Floor( (offset[2]-bounds[2])/2.0 );

   this->Max[0]= bounds[3] - vtkMath::Floor( (bounds[3]-offset[0])/2.0 );
   this->Max[1]= bounds[4] - vtkMath::Floor( (bounds[4]-offset[1])/2.0 );
   this->Max[2]= bounds[5] - vtkMath::Floor( (bounds[5]-offset[2])/2.0 );
//   this->initializedRegion = true;
}
