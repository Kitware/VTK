//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/CellLocatorPartitioned.h>
#include <viskores/cont/PartitionedDataSet.h>
#include <viskores/exec/CellLocatorPartitioned.h>

namespace viskores
{
namespace cont
{

void CellLocatorPartitioned::Update()
{
  if (this->Modified)
  {
    this->Build();
    this->Modified = false;
  }
}


void CellLocatorPartitioned::Build()
{
  viskores::Id numPartitions = this->Partitions.GetNumberOfPartitions();
  this->LocatorsCont.resize(numPartitions);
  this->GhostsCont.resize(numPartitions);
  for (viskores::Id index = 0; index < numPartitions; ++index)
  {
    const viskores::cont::DataSet& dataset = this->Partitions.GetPartition(index);

    // fill vector of cellLocators
    viskores::cont::CellLocatorGeneral cellLocator;
    cellLocator.SetCellSet(dataset.GetCellSet());
    cellLocator.SetCoordinates(dataset.GetCoordinateSystem());
    cellLocator.Update();
    this->LocatorsCont.at(index) = cellLocator;

    // fill vector of ghostFields
    this->GhostsCont.at(index) =
      dataset.GetGhostCellField().GetData().ExtractComponent<viskores::UInt8>(0);
  }
}

const viskores::exec::CellLocatorPartitioned CellLocatorPartitioned::PrepareForExecution(
  viskores::cont::DeviceAdapterId device,
  viskores::cont::Token& token)
{
  this->Update();

  viskores::Id numPartitions = this->Partitions.GetNumberOfPartitions();
  this->LocatorsExec.Allocate(numPartitions, viskores::CopyFlag::Off, token);
  auto portalLocators = this->LocatorsExec.WritePortal(token);
  this->GhostsExec.Allocate(numPartitions, viskores::CopyFlag::Off, token);
  auto portalGhosts = this->GhostsExec.WritePortal(token);
  for (viskores::Id index = 0; index < numPartitions; ++index)
  {
    // fill arrayhandle of cellLocators
    portalLocators.Set(index, this->LocatorsCont.at(index).PrepareForExecution(device, token));

    // fill arrayhandle of ghostFields
    portalGhosts.Set(index, this->GhostsCont.at(index).PrepareForInput(device, token));
  }
  return viskores::exec::CellLocatorPartitioned(this->LocatorsExec.PrepareForInput(device, token),
                                                this->GhostsExec.PrepareForInput(device, token));
}

} // namespace cont
} //namespace viskores
