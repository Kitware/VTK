//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//=============================================================================

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ErrorFilterExecution.h>
#include <viskores/filter/flow/StreamSurface.h>
#include <viskores/filter/flow/worklet/Field.h>
#include <viskores/filter/flow/worklet/GridEvaluators.h>
#include <viskores/filter/flow/worklet/ParticleAdvection.h>
#include <viskores/filter/flow/worklet/RK4Integrator.h>
#include <viskores/filter/flow/worklet/Stepper.h>
#include <viskores/filter/flow/worklet/StreamSurface.h>

namespace viskores
{
namespace filter
{
namespace flow
{

VISKORES_CONT viskores::cont::PartitionedDataSet StreamSurface::DoExecutePartitions(
  const viskores::cont::PartitionedDataSet& input)
{
  viskores::cont::PartitionedDataSet outputData = Streamline::DoExecutePartitions(input);

  //compute surface from streamlines and replace cells
  for (viskores::IdComponent partitionId = 0; partitionId < outputData.GetNumberOfPartitions();
       ++partitionId)
  {
    viskores::cont::DataSet partition = outputData.GetPartition(partitionId);
    viskores::worklet::flow::StreamSurface streamSurface;
    viskores::cont::CellSetSingleType<> srfCells;
    streamSurface.Run(partition.GetCellSet(), srfCells);
    partition.SetCellSet(srfCells);
    outputData.ReplacePartition(partitionId, partition);
  }

  return outputData;
}

}
}
} // namespace viskores::filter::flow
