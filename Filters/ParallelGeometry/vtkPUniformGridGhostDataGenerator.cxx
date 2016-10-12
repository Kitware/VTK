/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkPUniformGridGhostDataGenerator.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkPUniformGridGhostDataGenerator.h"
#include "vtkObjectFactory.h"
#include "vtkPStructuredGridConnectivity.h"
#include "vtkUniformGrid.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkCommunicator.h"
#include "vtkInformation.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <cassert>

vtkStandardNewMacro(vtkPUniformGridGhostDataGenerator);

vtkPUniformGridGhostDataGenerator::vtkPUniformGridGhostDataGenerator()
{
  this->GridConnectivity = vtkPStructuredGridConnectivity::New();

  this->GlobalOrigin[0] =
  this->GlobalOrigin[1] =
  this->GlobalOrigin[2] = VTK_DOUBLE_MAX;

  this->GlobalSpacing[0] =
  this->GlobalSpacing[1] =
  this->GlobalSpacing[2] = VTK_DOUBLE_MIN;
}

//------------------------------------------------------------------------------
vtkPUniformGridGhostDataGenerator::~vtkPUniformGridGhostDataGenerator()
{
  this->GridConnectivity->Delete();
}

//------------------------------------------------------------------------------
void vtkPUniformGridGhostDataGenerator::PrintSelf(ostream& os,vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//------------------------------------------------------------------------------
void vtkPUniformGridGhostDataGenerator::RegisterGrids(vtkMultiBlockDataSet *in)
{
  assert("pre: input multi-block is NULL" && (in != NULL) );
  assert("pre: Grid Connectivity is NULL" && (this->GridConnectivity != NULL) );

  this->GridConnectivity->SetController( this->Controller );
  this->GridConnectivity->SetNumberOfGrids( in->GetNumberOfBlocks() );
  this->GridConnectivity->SetNumberOfGhostLayers(0);
  this->GridConnectivity->SetWholeExtent(
      in->GetInformation()->Get(
          vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));
  this->GridConnectivity->Initialize();

  for( unsigned int i=0; i < in->GetNumberOfBlocks(); ++i )
  {
    vtkUniformGrid *grid = vtkUniformGrid::SafeDownCast(in->GetBlock(i));
    if( grid != NULL )
    {
      vtkInformation *info = in->GetMetaData( i );
      assert("pre: NULL meta-data" && (info != NULL) );
      assert("pre: No piece meta-data" &&
             info->Has(vtkDataObject::PIECE_EXTENT()));

      this->GridConnectivity->RegisterGrid(
          static_cast<int>(i), info->Get(vtkDataObject::PIECE_EXTENT()),
          grid->GetPointGhostArray(),
          grid->GetCellGhostArray(),
          grid->GetPointData(),
          grid->GetCellData(),
          NULL);
    } // END if
  } // END for all blocks
}

//------------------------------------------------------------------------------
void vtkPUniformGridGhostDataGenerator::GenerateGhostLayers(
    vtkMultiBlockDataSet *in, vtkMultiBlockDataSet *out)
{
  // Sanity check
  assert("pre: input multi-block is NULL" && (in != NULL) );
  assert("pre: output multi-block is NULL" && (out != NULL) );
  assert("pre: initialized" && (this->Initialized) );
  assert("pre: Grid connectivity is NULL" && (this->GridConnectivity != NULL) );
  assert("pre: controller should not be NULL" && (this->Controller != NULL) );

  // STEP 0: Compute global grid parameters
  this->ComputeGlobalSpacing( in );
  this->ComputeOrigin( in );
  this->Barrier();

  // STEP 1: Register grids
  this->RegisterGrids( in );
  this->Barrier();

  // STEP 2: Compute neighbors
  this->GridConnectivity->ComputeNeighbors();

  // STEP 3: Generate ghost layers
  this->GridConnectivity->CreateGhostLayers(this->NumberOfGhostLayers);

  // STEP 4: Create the ghosted data-set
  this->CreateGhostedDataSet(in,out);
  this->Barrier();
}

//------------------------------------------------------------------------------
void vtkPUniformGridGhostDataGenerator::ComputeGlobalSpacing(
    vtkMultiBlockDataSet *in)
{
  assert("pre: input multi-block is NULL" && (in != NULL) );
  assert("pre: Controller should not be NULL" && (this->Controller != NULL) );

  for( unsigned int block=0; block < in->GetNumberOfBlocks(); ++block )
  {
    vtkUniformGrid *grid = vtkUniformGrid::SafeDownCast(in->GetBlock(block));
    if( grid != NULL )
    {
      grid->GetSpacing(this->GlobalSpacing);
    } // END if grid is not NULL
  } // End for all blocks
}

//------------------------------------------------------------------------------
void vtkPUniformGridGhostDataGenerator::CreateGhostedDataSet(
    vtkMultiBlockDataSet *in, vtkMultiBlockDataSet *out)
{
  assert( "pre: input multi-block is NULL" && (in != NULL) );
  assert( "pre: output multi-block is NULL" && (out != NULL) );

  out->SetNumberOfBlocks( in->GetNumberOfBlocks( ) );
  int wholeExt[6];
  in->GetInformation()->Get(
      vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExt);
  out->GetInformation()->Set(
      vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),wholeExt,6);

  int ghostedExtent[6];
  double origin[3];
  int dims[3];

  for( unsigned int i=0; i < out->GetNumberOfBlocks(); ++i )
  {
    if( in->GetBlock(i) != NULL )
    {
      // STEP 0: Get the computed ghosted grid extent
      this->GridConnectivity->GetGhostedGridExtent(i,ghostedExtent);

      // STEP 1: Get the ghosted grid dimensions from the ghosted extent
      vtkStructuredData::GetDimensionsFromExtent(ghostedExtent,dims);

      // STEP 2: Construct the ghosted grid instance
      vtkUniformGrid *ghostedGrid = vtkUniformGrid::New();
      assert( "pre: Cannot create ghosted grid instance" &&
              (ghostedGrid != NULL) );

      // STEP 3: Compute the ghosted grid origin
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

      out->SetBlock(i,ghostedGrid);
      ghostedGrid->Delete();
    }
    else
    {
      out->SetBlock( i, NULL );
    }
  } // END for all blocks
}

//------------------------------------------------------------------------------
void vtkPUniformGridGhostDataGenerator::ComputeOrigin(vtkMultiBlockDataSet *in)
{
  assert("pre: input multi-block is NULL" && (in != NULL) );
  assert("pre: Controller should not be NULL" && (this->Controller != NULL) );

  double localOrigin[3];
  localOrigin[0] =
  localOrigin[1] =
  localOrigin[2] = VTK_DOUBLE_MAX;

  // STEP 1: Compute local origin
  double gridOrigin[3];
  for( unsigned int block=0; block < in->GetNumberOfBlocks(); ++block )
  {
    vtkUniformGrid *grid = vtkUniformGrid::SafeDownCast(in->GetBlock(block));
    if( grid != NULL )
    {
      grid->GetOrigin( gridOrigin );
      for( int i=0; i < 3; ++i )
      {
        if( gridOrigin[i] < localOrigin[i] )
        {
          localOrigin[i] = gridOrigin[i];
        }
      } // END for all dimensions
    } // END if grid is not NULL
  } // END for all blocks

  // STEP 2: All reduce
  this->Controller->AllReduce(
      &localOrigin[0],&this->GlobalOrigin[0],1,vtkCommunicator::MIN_OP );
  this->Controller->AllReduce(
      &localOrigin[1],&this->GlobalOrigin[1],1,vtkCommunicator::MIN_OP );
  this->Controller->AllReduce(
      &localOrigin[2],&this->GlobalOrigin[2],1,vtkCommunicator::MIN_OP );

}
