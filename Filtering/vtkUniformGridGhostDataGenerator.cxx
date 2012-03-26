/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkUniformGridGhostDataGenerator.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkUniformGridGhostDataGenerator.h"
#include "vtkObjectFactory.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkStructuredGridConnectivity.h"
#include "vtkUniformGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro( vtkUniformGridGhostDataGenerator );

//------------------------------------------------------------------------------
vtkUniformGridGhostDataGenerator::vtkUniformGridGhostDataGenerator()
{
  this->GridConnectivity = vtkStructuredGridConnectivity::New();

  this->GlobalOrigin[0] =
  this->GlobalOrigin[1] =
  this->GlobalOrigin[2] = VTK_DOUBLE_MAX;

  this->GlobalSpacing[0] =
  this->GlobalSpacing[1] =
  this->GlobalSpacing[2] = VTK_DOUBLE_MIN;
}

//------------------------------------------------------------------------------
vtkUniformGridGhostDataGenerator::~vtkUniformGridGhostDataGenerator()
{
  this->GridConnectivity->Delete();
}

//------------------------------------------------------------------------------
void vtkUniformGridGhostDataGenerator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkUniformGridGhostDataGenerator::ComputeOrigin(
    vtkMultiBlockDataSet *in)
{
  assert("pre: Multi-block dataset is NULL" && (in != NULL) );

  for( unsigned int i=0; i < in->GetNumberOfBlocks(); ++i )
    {
    vtkUniformGrid *grid = vtkUniformGrid::SafeDownCast(in->GetBlock(i));
    assert("pre: grid block is NULL" && (grid != NULL) );

    double blkOrigin[3];
    grid->GetOrigin( blkOrigin );
    if( blkOrigin[0] < this->GlobalOrigin[0] )
      {
      this->GlobalOrigin[0] = blkOrigin[0];
      }
    if( blkOrigin[1] < this->GlobalOrigin[1] )
      {
      this->GlobalOrigin[1] = blkOrigin[1];
      }
    if( blkOrigin[2] < this->GlobalOrigin[2] )
      {
      this->GlobalOrigin[2] = blkOrigin[2];
      }
    } // END for all blocks
}

//------------------------------------------------------------------------------
void vtkUniformGridGhostDataGenerator::ComputeGlobalSpacingVector(
    vtkMultiBlockDataSet *in)
{
  assert("pre: Multi-block dataset is NULL" && (in != NULL) );

  // NOTE: we assume that the spacing of the all the blocks is the same.
  vtkUniformGrid *block0 = vtkUniformGrid::SafeDownCast(in->GetBlock(0));
  assert("pre: grid block is NULL" && (block0 != NULL) );

  block0->GetSpacing( this->GlobalSpacing );
}

//------------------------------------------------------------------------------
void vtkUniformGridGhostDataGenerator::RegisterGrids(vtkMultiBlockDataSet *in)
{
  assert("pre: Multi-block dataset is NULL" && (in != NULL) );

  this->GridConnectivity->SetNumberOfGrids( in->GetNumberOfBlocks() );
  this->GridConnectivity->SetNumberOfGhostLayers( 0 );
  this->GridConnectivity->SetWholeExtent(
      in->GetInformation()->Get(
          vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));

  for( unsigned int i=0; i < in->GetNumberOfBlocks(); ++i )
    {
    vtkUniformGrid *grid = vtkUniformGrid::SafeDownCast( in->GetBlock( i ) );
    assert("pre: grid block is NULL" && (grid != NULL) );

    vtkInformation *info = in->GetMetaData( i );
    assert("pre: NULL meta-data" && (info != NULL) );
    assert("pre: No piece meta-data" &&
           info->Has(vtkDataObject::PIECE_EXTENT()));

    this->GridConnectivity->RegisterGrid(
        static_cast<int>(i), info->Get(vtkDataObject::PIECE_EXTENT()),
        grid->GetPointVisibilityArray(),
        grid->GetCellVisibilityArray(),
        grid->GetPointData(),
        grid->GetCellData(),
        NULL);
    } // END for all blocks
}

//------------------------------------------------------------------------------
void vtkUniformGridGhostDataGenerator::CreateGhostedDataSet(
    vtkMultiBlockDataSet *in, vtkMultiBlockDataSet *out )
{
  assert("pre: input multi-block is NULL" && (in != NULL) );
  assert("pre: output multi-block is NULL" && (out != NULL) );

  out->SetNumberOfBlocks( in->GetNumberOfBlocks() );

  int wholeExt[6];
  in->GetInformation()->Get(
      vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),wholeExt);
  vtkInformation *outInfo = out->GetInformation();
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),wholeExt,6);

  int ghostedExtent[6];
  double origin[3];
  int dims[3];

  for( unsigned int i=0; i < out->GetNumberOfBlocks(); ++i )
    {
    // STEP 0: Get the computed ghosted grid extent
    this->GridConnectivity->GetGhostedGridExtent( i,ghostedExtent );

    // STEP 1: Get the ghosted grid dimensions from the ghosted extent
    vtkStructuredData::GetDimensionsFromExtent( ghostedExtent, dims );

    // STEP 2: Construct ghosted grid instance
    vtkUniformGrid *ghostedGrid = vtkUniformGrid::New();
    assert("pre: Cannot create ghosted grid instance" && (ghostedGrid != NULL));

    // STEP 3: Get ghosted grid origin
    origin[0] = this->GlobalOrigin[0]+ghostedExtent[0]*this->GlobalSpacing[0];
    origin[1] = this->GlobalOrigin[1]+ghostedExtent[2]*this->GlobalSpacing[1];
    origin[2] = this->GlobalOrigin[2]+ghostedExtent[4]*this->GlobalSpacing[2];

    // STEP 4: Set ghosted uniform grid attributes
    ghostedGrid->SetOrigin( origin );
    ghostedGrid->SetDimensions( dims );
    ghostedGrid->SetSpacing( this->GlobalSpacing );

    // STEP 5: Copy the node/cell data
    ghostedGrid->GetPointData()->DeepCopy(
        this->GridConnectivity->GetGhostedGridPointData(i) );
    ghostedGrid->GetCellData()->DeepCopy(
        this->GridConnectivity->GetGhostedGridCellData(i) );

    // STEP 6: Copy the ghost arrays
    ghostedGrid->SetPointVisibilityArray(
        this->GridConnectivity->GetGhostedPointGhostArray( i ) );
    ghostedGrid->SetCellVisibilityArray(
        this->GridConnectivity->GetGhostedCellGhostArray( i ) );

    out->SetBlock(i,ghostedGrid);
    ghostedGrid->Delete();
    } // END for all blocks
}

//------------------------------------------------------------------------------
void vtkUniformGridGhostDataGenerator::GenerateGhostLayers(
    vtkMultiBlockDataSet *in, vtkMultiBlockDataSet *out )
{
  assert("pre: Number of ghost-layers must be greater than 0!" &&
         (this->NumberOfGhostLayers > 0) );
  assert("pre: Input dataset is NULL!" && (in != NULL));
  assert("pre: Output dataset is NULL!" && (out != NULL) );
  assert("pre: GridConnectivity is NULL!" && (this->GridConnectivity != NULL));

  // STEP 0: Register grids & compute global grid parameters
  this->RegisterGrids( in );
  this->ComputeOrigin( in );
  this->ComputeGlobalSpacingVector( in );

  // STEP 1: Compute Neighbors
  this->GridConnectivity->ComputeNeighbors();

  // STEP 2: Generate ghost layers
  this->GridConnectivity->CreateGhostLayers( this->NumberOfGhostLayers );

  // STEP 3: Get output data-set
  this->CreateGhostedDataSet( in, out );
}
