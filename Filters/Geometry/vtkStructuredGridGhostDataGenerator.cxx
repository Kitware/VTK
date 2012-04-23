/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkStructuredGridGhostDataGenerator.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkStructuredGridGhostDataGenerator.h"
#include "vtkObjectFactory.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkStructuredGridConnectivity.h"
#include "vtkStructuredGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"

#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkStructuredGridGhostDataGenerator);

//------------------------------------------------------------------------------
vtkStructuredGridGhostDataGenerator::vtkStructuredGridGhostDataGenerator()
{
  this->GridConnectivity = vtkStructuredGridConnectivity::New();
}

//------------------------------------------------------------------------------
vtkStructuredGridGhostDataGenerator::~vtkStructuredGridGhostDataGenerator()
{
  this->GridConnectivity->Delete();
}

//------------------------------------------------------------------------------
void vtkStructuredGridGhostDataGenerator::PrintSelf(
    ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf( os, indent );
}

//------------------------------------------------------------------------------
void vtkStructuredGridGhostDataGenerator::RegisterGrids(
    vtkMultiBlockDataSet *in)
{
  assert("pre: Input multi-block is NULL" && (in != NULL) );
  assert("pre: Grid connectivity should not be NULL" &&
         (this->GridConnectivity != NULL) );

  this->GridConnectivity->SetNumberOfGrids( in->GetNumberOfBlocks() );
  this->GridConnectivity->SetNumberOfGhostLayers( 0 );
  this->GridConnectivity->SetWholeExtent(
   in->GetInformation()->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));

  for( unsigned int i=0; i < in->GetNumberOfBlocks(); ++i )
    {
    vtkStructuredGrid *grid = vtkStructuredGrid::SafeDownCast( in->GetBlock(i));
    assert("pre: grid block is NULL" && (grid != NULL) );

    vtkInformation *info = in->GetMetaData( i );
    assert("pre: NULL meta-data" && (info != NULL) );
    assert("pre: No piece meta-data" &&
            info->Has(vtkDataObject::PIECE_EXTENT()));

    this->GridConnectivity->RegisterGrid(
        static_cast<int>(i),info->Get(vtkDataObject::PIECE_EXTENT()),
        grid->GetPointVisibilityArray(),
        grid->GetCellVisibilityArray(),
        grid->GetPointData(),
        grid->GetCellData(),
        grid->GetPoints() );
    } // END for all blocks
}

//------------------------------------------------------------------------------
void vtkStructuredGridGhostDataGenerator::CreateGhostedDataSet(
    vtkMultiBlockDataSet *in, vtkMultiBlockDataSet *out)
{
  assert("pre: Input multi-block is NULL" && (in != NULL) );
  assert("pre: Output multi-block is NULL" && (out != NULL) );
  assert("pre: Grid connectivity should not be NULL" &&
         (this->GridConnectivity != NULL) );

  out->SetNumberOfBlocks( in->GetNumberOfBlocks() );
  int wholeExt[6];
  in->GetInformation()->Get(
      vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExt );
  vtkInformation *outInfo = out->GetInformation();
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),wholeExt,6);

  int ghostedExtent[6];
  for( unsigned int i=0; i < out->GetNumberOfBlocks(); ++i )
    {
    // STEP 0: Get the computed ghosted grid extent
    this->GridConnectivity->GetGhostedGridExtent( i, ghostedExtent );

    // STEP 1: Construct the ghosted structured grid instance
    vtkStructuredGrid *ghostedGrid = vtkStructuredGrid::New();
    assert("pre: Cannot create ghosted grid instance" && (ghostedGrid != NULL));
    ghostedGrid->SetExtent( ghostedExtent );

    vtkPoints *ghostedGridPoints = vtkPoints::New();
    ghostedGridPoints->DeepCopy(this->GridConnectivity->GetGhostedPoints(i));
    ghostedGrid->SetPoints(ghostedGridPoints);
    ghostedGridPoints->Delete();

    // STEP 2: Copy the node/cell data
    ghostedGrid->GetPointData()->DeepCopy(
        this->GridConnectivity->GetGhostedGridPointData(i) );
    ghostedGrid->GetCellData()->DeepCopy(
        this->GridConnectivity->GetGhostedGridCellData(i) );

    // STEP 3: Copy the ghost arrays
    ghostedGrid->SetPointVisibilityArray(
        this->GridConnectivity->GetGhostedPointGhostArray(i) );
    ghostedGrid->SetPointVisibilityArray(
        this->GridConnectivity->GetGhostedCellGhostArray(i) );

    out->SetBlock(i,ghostedGrid);
    ghostedGrid->Delete();
    } // END for all blocks
}

//------------------------------------------------------------------------------
void vtkStructuredGridGhostDataGenerator::GenerateGhostLayers(
    vtkMultiBlockDataSet *in, vtkMultiBlockDataSet *out )
{
  assert("pre: Input multi-block is NULL" && (in != NULL) );
  assert("pre: Output multi-block is NULL" && (out != NULL) );
  assert("pre: Grid connectivity should not be NULL" &&
         (this->GridConnectivity != NULL) );

  // STEP 0: Register the input grids
  this->RegisterGrids( in );

  // STEP 1: Computes the neighbors
  this->GridConnectivity->ComputeNeighbors();

  // STEP 2: Generate the ghost layers
  this->GridConnectivity->CreateGhostLayers( this->NumberOfGhostLayers );

  // STEP 3: Get the output dataset
  this->CreateGhostedDataSet( in ,out );
}
