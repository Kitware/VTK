/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkStructuredGridPartitioner.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkStructuredGridPartitioner.h"
#include "vtkExtentRCBPartitioner.h"
#include "vtkIndent.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredData.h"
#include "vtkStructuredExtent.h"
#include "vtkStructuredGrid.h"

#include <cassert>

vtkStandardNewMacro(vtkStructuredGridPartitioner);

//------------------------------------------------------------------------------
vtkStructuredGridPartitioner::vtkStructuredGridPartitioner()
{
  this->NumberOfPartitions = 2;
  this->NumberOfGhostLayers = 0;
  this->DuplicateNodes = 1;
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//------------------------------------------------------------------------------
vtkStructuredGridPartitioner::~vtkStructuredGridPartitioner() = default;

//------------------------------------------------------------------------------
void vtkStructuredGridPartitioner::PrintSelf(std::ostream& oss, vtkIndent indent)
{
  this->Superclass::PrintSelf(oss, indent);
  oss << "NumberOfPartitions: " << this->NumberOfPartitions << std::endl;
  oss << "NumberOfGhostLayers: " << this->NumberOfGhostLayers << std::endl;
  oss << "DuplicateNodes: " << this->DuplicateNodes << std::endl;
}

//------------------------------------------------------------------------------
int vtkStructuredGridPartitioner::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkStructuredGrid");
  return 1;
}

//------------------------------------------------------------------------------
int vtkStructuredGridPartitioner::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet");
  return 1;
}

//------------------------------------------------------------------------------
vtkPoints* vtkStructuredGridPartitioner::ExtractSubGridPoints(
  vtkStructuredGrid* wholeGrid, int subext[6])
{
  assert("pre: whole grid is nullptr" && (wholeGrid != nullptr));

  int numNodes = vtkStructuredData::GetNumberOfPoints(subext);
  vtkPoints* pnts = vtkPoints::New();
  pnts->SetDataTypeToDouble();
  pnts->SetNumberOfPoints(numNodes);

  int ijk[3];
  double p[3];
  int dataDescription = vtkStructuredData::GetDataDescriptionFromExtent(subext);
  for (int i = subext[0]; i <= subext[1]; ++i)
  {
    for (int j = subext[2]; j <= subext[3]; ++j)
    {
      for (int k = subext[4]; k <= subext[5]; ++k)
      {
        wholeGrid->GetPoint(i, j, k, p, false);

        ijk[0] = i;
        ijk[1] = j;
        ijk[2] = k;
        vtkIdType pntIdx = vtkStructuredData::ComputePointIdForExtent(subext, ijk, dataDescription);
        assert("pre: point index is out-of-bounds!" && (pntIdx >= 0) && (pntIdx < numNodes));
        pnts->SetPoint(pntIdx, p);
      } // END for all k
    }   // END for all j
  }     // END for all i
  return (pnts);
}

//------------------------------------------------------------------------------
int vtkStructuredGridPartitioner::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // STEP 0: Get input object
  vtkInformation* input = inputVector[0]->GetInformationObject(0);
  assert("pre: input information object is nullptr" && (input != nullptr));
  vtkStructuredGrid* grd =
    vtkStructuredGrid::SafeDownCast(input->Get(vtkDataObject::DATA_OBJECT()));

  // STEP 1: Get output object
  vtkInformation* output = outputVector->GetInformationObject(0);
  assert("pre: output information object is nullptr" && (output != nullptr));
  vtkMultiBlockDataSet* multiblock =
    vtkMultiBlockDataSet::SafeDownCast(output->Get(vtkDataObject::DATA_OBJECT()));
  assert("pre: multi-block grid is nullptr" && (multiblock != nullptr));

  // STEP 2: Get the global extent
  int extent[6];
  grd->GetExtent(extent);

  // STEP 3: Setup extent partitioner
  vtkExtentRCBPartitioner* extentPartitioner = vtkExtentRCBPartitioner::New();
  assert("pre: extent partitioner is nullptr" && (extentPartitioner != nullptr));
  extentPartitioner->SetGlobalExtent(extent);
  extentPartitioner->SetNumberOfPartitions(this->NumberOfPartitions);
  extentPartitioner->SetNumberOfGhostLayers(this->NumberOfGhostLayers);

  if (this->DuplicateNodes == 1)
  {
    extentPartitioner->DuplicateNodesOn();
  }
  else
  {
    extentPartitioner->DuplicateNodesOff();
  }

  // STEP 4: Partition
  extentPartitioner->Partition();

  // STEP 5: Extract partitions in a multi-block
  multiblock->SetNumberOfBlocks(extentPartitioner->GetNumExtents());

  // Set the whole extent of the grid
  multiblock->GetInformation()->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent, 6);

  int subext[6];
  unsigned int blockIdx = 0;
  for (; blockIdx < multiblock->GetNumberOfBlocks(); ++blockIdx)
  {
    extentPartitioner->GetPartitionExtent(blockIdx, subext);

    vtkStructuredGrid* subgrid = vtkStructuredGrid::New();
    subgrid->SetExtent(subext);

    vtkPoints* points = this->ExtractSubGridPoints(grd, subext);
    assert("pre: subgrid points are nullptr" && (points != nullptr));
    subgrid->SetPoints(points);
    points->Delete();

    vtkInformation* metadata = multiblock->GetMetaData(blockIdx);
    assert("pre: metadata is nullptr" && (metadata != nullptr));
    metadata->Set(vtkDataObject::PIECE_EXTENT(), subext, 6);

    multiblock->SetBlock(blockIdx, subgrid);
    subgrid->Delete();
  } // END for all blocks

  extentPartitioner->Delete();
  return 1;
}
