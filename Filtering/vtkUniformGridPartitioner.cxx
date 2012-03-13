/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkUniformGridPartitioner.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

#include "vtkUniformGridPartitioner.h"
#include "vtkObjectFactory.h"
#include "vtkIndent.h"
#include "vtkExtentRCBPartitioner.h"
#include "vtkUniformGrid.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkStructuredData.h"
#include "vtkStructuredExtent.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <cassert>

vtkStandardNewMacro( vtkUniformGridPartitioner );

//------------------------------------------------------------------------------
vtkUniformGridPartitioner::vtkUniformGridPartitioner()
{
  this->NumberOfPartitions  = 2;
  this->NumberOfGhostLayers = 0;
}

//------------------------------------------------------------------------------
vtkUniformGridPartitioner::~vtkUniformGridPartitioner()
{

}

//------------------------------------------------------------------------------
void vtkUniformGridPartitioner::PrintSelf(std::ostream &oss, vtkIndent indent)
{
  this->Superclass::PrintSelf( oss, indent );
  oss << "NumberOfPartitions: " << this->NumberOfPartitions << std::endl;
  oss << "NumberOfGhostLayers: " << this->NumberOfGhostLayers << std::endl;
}

//------------------------------------------------------------------------------
int vtkUniformGridPartitioner::FillInputPortInformation(
    int vtkNotUsed(port), vtkInformation *info )
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(),"vtkImageData" );
  return 1;
}

//------------------------------------------------------------------------------
int vtkUniformGridPartitioner::FillOutputPortInformation(
    int vtkNotUsed(port), vtkInformation *info )
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(),"vtkMultiBlockDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int vtkUniformGridPartitioner::RequestData(
    vtkInformation* vtkNotUsed(request),
    vtkInformationVector** inputVector, vtkInformationVector* outputVector )
{
  // STEP 0: Get input object
  vtkInformation *input = inputVector[0]->GetInformationObject( 0 );
  assert( "pre: input information object is NULL" && (input != NULL) );

  vtkImageData *grd =
    vtkImageData::SafeDownCast( input->Get(vtkDataObject::DATA_OBJECT() ) );
  assert( "pre: input grid is NULL!" && (grd != NULL));

  // STEP 1: Get output object
  vtkInformation *output = outputVector->GetInformationObject( 0 );
  assert( "pre: output information object is NULL" && (output != NULL) );
  vtkMultiBlockDataSet *multiblock =
      vtkMultiBlockDataSet::SafeDownCast(
        output->Get( vtkDataObject::DATA_OBJECT() ) );
  assert( "pre: multiblock grid is NULL!" && (multiblock != NULL) );

  // STEP 2: Get the global extent
  int extent[6];
  int dims[3];
  grd->GetDimensions( dims );
  grd->GetExtent( extent );

  // STEP 3: Setup extent partitioner
  vtkExtentRCBPartitioner *extentPartitioner = vtkExtentRCBPartitioner::New();
  assert( "pre: extent partitioner is NULL" && (extentPartitioner != NULL) );
  extentPartitioner->SetGlobalExtent( extent );
  extentPartitioner->SetNumberOfPartitions( this->NumberOfPartitions );
  extentPartitioner->SetNumberOfGhostLayers( this->NumberOfGhostLayers );

  // STEP 4: Partition
  extentPartitioner->Partition();

  // STEP 5: Extract partitions into a multi-block dataset.
  multiblock->SetNumberOfBlocks( extentPartitioner->GetNumExtents() );

  // Set the whole extent of the grid
  multiblock->GetInformation()->Set(
        vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),extent,6);


  unsigned int blockIdx = 0;
  for( ; blockIdx < multiblock->GetNumberOfBlocks(); ++blockIdx )
    {
    int ext[6];
    extentPartitioner->GetPartitionExtent( blockIdx, ext );

    double origin[3];
    int ijk[3];
    ijk[0] = ext[0];
    ijk[1] = ext[2];
    ijk[2] = ext[4];

    int subdims[3];
    vtkStructuredExtent::GetDimensions( ext, subdims );

    int pntIdx = vtkStructuredData::ComputePointId( dims, ijk );

    grd->GetPoint( pntIdx, origin );

    vtkUniformGrid *subgrid = vtkUniformGrid::New();
    subgrid->SetOrigin( origin );
    subgrid->SetSpacing( grd->GetSpacing() );
    subgrid->SetDimensions( subdims );

    // Set the global extent for each block
    vtkInformation *metadata = multiblock->GetMetaData( blockIdx );
    assert( "pre: metadata is NULL" && (metadata != NULL) );
    metadata->Set( vtkDataObject::PIECE_EXTENT(), ext, 6 );

    multiblock->SetBlock( blockIdx, subgrid );
    subgrid->Delete();
    } // END for all blocks

  extentPartitioner->Delete();
  return 1;
}
