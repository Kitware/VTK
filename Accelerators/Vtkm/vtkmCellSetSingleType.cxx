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
#include "vtkmCellSetSingleType.h"
#include "vtkmConnectivityExec.h"

#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/DispatcherMapField.h>


namespace {
class ComputeReverseMapping : public vtkm::worklet::WorkletMapField
{
  vtkm::Id NumberOfPointsPerCell;
public:
  ComputeReverseMapping(vtkm::Id numberOfPointsPerCell):
    NumberOfPointsPerCell(numberOfPointsPerCell)
  {

  }
  typedef void ControlSignature(FieldIn<IdType> cellIndex,
                                WholeArrayIn<IdType> connectivity,
                                WholeArrayOut<IdType> pointIds,
                                WholeArrayOut<IdType> cellIds);
  typedef void ExecutionSignature(_1, _2, _3, _4);
  typedef _1 InputDomain;


  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename ConnPortalType, typename PortalType>
  VTKM_EXEC void operator()(const vtkm::Id& cellId,
                            const ConnPortalType& connectivity,
                            const PortalType& pointIdKey,
                            const PortalType& pointIdValue) const
  {
    const vtkm::Id read_offset = (NumberOfPointsPerCell+1) * cellId;
    const vtkm::Id write_offset = NumberOfPointsPerCell * cellId;
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
vtkm::IdComponent vtkmCellSetSingleType::DetermineNumberOfPoints() const
{
  vtkm::IdComponent numberOfPointsPerCell = -1;
  switch (this->CellTypeAsId)
  {
    vtkmGenericCellShapeMacro(this->DetermineNumberOfPoints(
        CellShapeTag(), vtkm::CellTraits<CellShapeTag>::IsSizeFixed(),
        numberOfPointsPerCell));
  default:
    throw vtkm::cont::ErrorBadValue(
        "CellSetSingleType unable to determine the cell type");
  }
  return numberOfPointsPerCell;
}

//------------------------------------------------------------------------------
void vtkmCellSetSingleType::Fill(
    vtkm::Id numberOfPoints,
    const vtkm::cont::ArrayHandle<vtkm::Id, tovtkm::vtkCellArrayContainerTag>&
        connectivity)
{
  const vtkm::IdComponent numberOfPointsPerCell =
      this->DetermineNumberOfPoints();
  this->NumberOfCells =
      connectivity.GetNumberOfValues() / (numberOfPointsPerCell + 1);
  this->Connectivity = connectivity;
  this->NumberOfPoints = numberOfPoints;
}

//------------------------------------------------------------------------------
template <typename Device>
typename vtkm::exec::ConnectivityVTKSingleType<Device>
    vtkmCellSetSingleType::PrepareForInput(Device,
                                           vtkm::TopologyElementTagPoint,
                                           vtkm::TopologyElementTagCell) const
{
  const vtkm::IdComponent numberOfPointsPerCell =
      this->DetermineNumberOfPoints();
  const vtkm::UInt8 shapeTypeValue =
      static_cast<vtkm::UInt8>(this->CellTypeAsId);

  return vtkm::exec::ConnectivityVTKSingleType<Device>(
      this->Connectivity.PrepareForInput(Device()), this->NumberOfCells,
      numberOfPointsPerCell, shapeTypeValue);
}

//------------------------------------------------------------------------------
template <typename Device>
typename vtkm::exec::ReverseConnectivityVTK<Device>
    vtkmCellSetSingleType::PrepareForInput(Device,
                                           vtkm::TopologyElementTagCell,
                                           vtkm::TopologyElementTagPoint) const
{
  if(!this->ReverseConnectivityBuilt)
  {
    const vtkm::Id numberOfCells = this->GetNumberOfCells();
    const vtkm::Id connectivityLength = this->Connectivity.GetNumberOfValues();
    const vtkm::Id numberOfPointsPerCell = this->DetermineNumberOfPoints();
    const vtkm::Id rconnSize = numberOfCells*numberOfPointsPerCell;

    // create a mapping of where each key is the point id and the value
    // is the cell id.
    using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<Device>;
    vtkm::cont::ArrayHandle<vtkm::Id> pointIdKey;

    // We need to allocate pointIdKey and RConn to correct length.
    // which for this is equal to numberOfCells * numberOfPointsPerCell
    // as the connectivity has the vtk padding per cell
    pointIdKey.Allocate(rconnSize);
    this->RConn.Allocate(rconnSize);

    vtkm::worklet::DispatcherMapField<ComputeReverseMapping, Device> dispatcher( ComputeReverseMapping(this->DetermineNumberOfPoints()));
    dispatcher.Invoke(vtkm::cont::make_ArrayHandleCounting(0, 1, numberOfCells),
                      this->Connectivity, pointIdKey, this->RConn);
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

//------------------------------------------------------------------------------
void vtkmCellSetSingleType::PrintSummary(std::ostream& out) const
{
  out << "   vtkmCellSetSingleType: " << this->Name << std::endl;
  out << "   NumberOfCells: " << this->NumberOfCells << std::endl;
  out << "   CellTypeAsId: " << this->CellTypeAsId << std::endl;
  out << "   Connectivity: " << std::endl;
  vtkm::cont::printSummary_ArrayHandle(this->Connectivity, out);
}

// template methods we want to compile only once
template VTKACCELERATORSVTKM_EXPORT
  vtkm::exec::ConnectivityVTKSingleType<vtkm::cont::DeviceAdapterTagSerial>
    vtkmCellSetSingleType::PrepareForInput(vtkm::cont::DeviceAdapterTagSerial,
      vtkm::TopologyElementTagPoint, vtkm::TopologyElementTagCell) const;

template VTKACCELERATORSVTKM_EXPORT
  vtkm::exec::ReverseConnectivityVTK<vtkm::cont::DeviceAdapterTagSerial>
    vtkmCellSetSingleType::PrepareForInput(vtkm::cont::DeviceAdapterTagSerial,
      vtkm::TopologyElementTagCell, vtkm::TopologyElementTagPoint) const;

#ifdef VTKM_ENABLE_TBB
template VTKACCELERATORSVTKM_EXPORT
  vtkm::exec::ConnectivityVTKSingleType<vtkm::cont::DeviceAdapterTagTBB>
    vtkmCellSetSingleType::PrepareForInput(vtkm::cont::DeviceAdapterTagTBB,
      vtkm::TopologyElementTagPoint, vtkm::TopologyElementTagCell) const;

template VTKACCELERATORSVTKM_EXPORT
  vtkm::exec::ReverseConnectivityVTK<vtkm::cont::DeviceAdapterTagTBB>
    vtkmCellSetSingleType::PrepareForInput(vtkm::cont::DeviceAdapterTagTBB,
      vtkm::TopologyElementTagCell, vtkm::TopologyElementTagPoint) const;
#endif
}
}
