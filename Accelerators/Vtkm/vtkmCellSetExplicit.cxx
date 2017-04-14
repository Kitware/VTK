//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include "vtkmlib/Storage.h"
#include "vtkmCellSetExplicit.h"
#include "vtkmConnectivityExec.h"

#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/DispatcherMapField.h>

namespace {
class ComputeReverseMapping : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<IdType> cellId,
                                FieldIn<IdType> offset,
                                WholeArrayIn<IdType> connectivity,
                                WholeArrayOut<IdType> pointIds,
                                WholeArrayOut<IdType> cellIds);
  typedef void ExecutionSignature(_1, _2, _3, _4, _5);
  typedef _1 InputDomain;


  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename ConnPortalType, typename PortalType>
  VTKM_EXEC void operator()(const vtkm::Id& cellId,
                            const vtkm::Id& read_offset,
                            const ConnPortalType& connectivity,
                            const PortalType& pointIdKey,
                            const PortalType& pointIdValue) const
  {
    //We can compute the correct write_offset by looking at the current
    //read_offset and subtracting the number of cells we already processed
    //which is equivalent to our cellid.
    //This properly removes from read_offset the vtk cell padding that is
    //added to the connectivity array
    const vtkm::Id write_offset = read_offset - cellId;
    const vtkm::Id numIndices = connectivity.Get(read_offset);
    for (vtkm::Id i = 0; i < numIndices; i++)
    {
      pointIdKey.Set(write_offset + i, connectivity.Get(read_offset + i + 1) );
      pointIdValue.Set(write_offset + i, cellId);
    }
  }
};
} //namespace

namespace vtkm {
namespace cont {

//------------------------------------------------------------------------------
void vtkmCellSetExplicitAOS::Fill(
    vtkm::Id numberOfPoints,
    const vtkm::cont::ArrayHandle<vtkm::UInt8, tovtkm::vtkAOSArrayContainerTag>&
        cellTypes,
    const vtkm::cont::ArrayHandle<vtkm::Id, tovtkm::vtkCellArrayContainerTag>&
        connectivity,
    const vtkm::cont::ArrayHandle<vtkm::Id, tovtkm::vtkAOSArrayContainerTag>&
        offsets)
{
  this->Shapes = cellTypes;
  this->Connectivity = connectivity;
  this->IndexOffsets = offsets;
  this->NumberOfPoints = numberOfPoints;
}

//------------------------------------------------------------------------------
void vtkmCellSetExplicitAOS::PrintSummary(std::ostream& out) const
{
  out << "   vtkmCellSetExplicitAOS: " << this->Name << std::endl;
  out << "   Shapes: " << std::endl;
  vtkm::cont::printSummary_ArrayHandle(this->Shapes, out);
  out << "   Connectivity: " << std::endl;
  vtkm::cont::printSummary_ArrayHandle(this->Connectivity, out);
  out << "   IndexOffsets: " << std::endl;
  vtkm::cont::printSummary_ArrayHandle(this->IndexOffsets, out);
}

//------------------------------------------------------------------------------
template <typename Device>
typename vtkm::exec::ConnectivityVTKAOS<Device>
    vtkmCellSetExplicitAOS::PrepareForInput(Device,
                                            vtkm::TopologyElementTagPoint,
                                            vtkm::TopologyElementTagCell) const
{
  return vtkm::exec::ConnectivityVTKAOS<Device>(
      this->Shapes.PrepareForInput(Device()),
      this->Connectivity.PrepareForInput(Device()),
      this->IndexOffsets.PrepareForInput(Device()));
}

//------------------------------------------------------------------------------
template <typename Device>
typename vtkm::exec::ReverseConnectivityVTK<Device>
    vtkmCellSetExplicitAOS::PrepareForInput(Device,
                                            vtkm::TopologyElementTagCell,
                                            vtkm::TopologyElementTagPoint) const
{
  //One of the biggest questions when computing the reverse connectivity
  //is how are we going to layout the results.
  //We have two options:
  // 1. The layout mirrors that of the point->cell where the connectivity array
  // is VTK with the counts interlaced inside the array
  // 2. We go with a VTK-m approach where we keep a separate array.
  //
  //While #1 has the strength of being easily mapped to VTK, we are going with
  //#2 as it easier to construct
  if(!this->ReverseConnectivityBuilt)
  {
    const vtkm::Id numberOfCells = this->GetNumberOfCells();
    const vtkm::Id connectivityLength = this->Connectivity.GetNumberOfValues();
    const vtkm::Id rconnSize = connectivityLength - this->IndexOffsets.GetNumberOfValues();

    // create a mapping of where each key is the point id and the value
    // is the cell id.
    using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<Device>;
    vtkm::cont::ArrayHandle<vtkm::Id> pointIdKey;

    // We need to allocate pointIdKey and RConn to correct length.
    // which for this is equal to connectivityLength - len(this->IndexOffsets)
    pointIdKey.Allocate(rconnSize);
    this->RConn.Allocate(rconnSize);

    vtkm::worklet::DispatcherMapField<ComputeReverseMapping, Device> dispatcher;
    dispatcher.Invoke(vtkm::cont::make_ArrayHandleCounting(0, 1, numberOfCells),
      this->IndexOffsets, this->Connectivity, pointIdKey, this->RConn);
    Algorithm::SortByKey(pointIdKey, this->RConn);

    // now we can compute the NumIndices
    vtkm::cont::ArrayHandle<vtkm::Id> reducedKeys;
    Algorithm::ReduceByKey(pointIdKey,
      vtkm::cont::make_ArrayHandleConstant(vtkm::IdComponent(1), rconnSize),
      reducedKeys, this->RNumIndices, vtkm::Add());

    // than a scan exclusive will give us the index offsets
    using CastedNumIndices = vtkm::cont::ArrayHandleCast<vtkm::Id,
      vtkm::cont::ArrayHandle<vtkm::IdComponent>>;
    Algorithm::ScanExclusive(
      CastedNumIndices(this->RNumIndices), this->RIndexOffsets);

    this->NumberOfPoints = reducedKeys.GetNumberOfValues();
    this->ReverseConnectivityBuilt = true;
  }

  //no need to have a reverse shapes array, as everything has the shape type
  //of vertex
  return vtkm::exec::ReverseConnectivityVTK<Device>(
      this->RConn.PrepareForInput(Device()),
      this->RNumIndices.PrepareForInput(Device()),
      this->RIndexOffsets.PrepareForInput(Device()));
}

// template methods we want to compile only once
template VTKACCELERATORSVTKM_EXPORT
  vtkm::exec::ConnectivityVTKAOS<vtkm::cont::DeviceAdapterTagSerial>
    vtkmCellSetExplicitAOS::PrepareForInput(vtkm::cont::DeviceAdapterTagSerial,
      vtkm::TopologyElementTagPoint, vtkm::TopologyElementTagCell) const;

template VTKACCELERATORSVTKM_EXPORT
  vtkm::exec::ReverseConnectivityVTK<vtkm::cont::DeviceAdapterTagSerial>
    vtkmCellSetExplicitAOS::PrepareForInput(vtkm::cont::DeviceAdapterTagSerial,
      vtkm::TopologyElementTagCell, vtkm::TopologyElementTagPoint) const;

#ifdef VTKM_ENABLE_TBB
template VTKACCELERATORSVTKM_EXPORT
  vtkm::exec::ConnectivityVTKAOS<vtkm::cont::DeviceAdapterTagTBB>
    vtkmCellSetExplicitAOS::PrepareForInput(vtkm::cont::DeviceAdapterTagTBB,
      vtkm::TopologyElementTagPoint, vtkm::TopologyElementTagCell) const;

template VTKACCELERATORSVTKM_EXPORT
  vtkm::exec::ReverseConnectivityVTK<vtkm::cont::DeviceAdapterTagTBB>
    vtkmCellSetExplicitAOS::PrepareForInput(vtkm::cont::DeviceAdapterTagTBB,
      vtkm::TopologyElementTagCell, vtkm::TopologyElementTagPoint) const;
#endif

}
}
