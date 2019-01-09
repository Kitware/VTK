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

#include <vtkm/cont/internal/ReverseConnectivityBuilder.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/DispatcherMapField.h>

namespace
{


// Converts [0, rconnSize) to [0, connSize), skipping cell length entries.
struct SingleTypeRConnToConn
{
  vtkm::Id PointsPerCell;

  VTKM_EXEC
  vtkm::Id operator()(vtkm::Id rconnIdx) const
  {
    return rconnIdx + 1 + (rconnIdx / this->PointsPerCell);
  }
};

// Converts a connectivity index to a cell id:
struct SingleTypeCellIdCalc
{
  vtkm::Id EncodedCellSize;

  VTKM_EXEC
  vtkm::Id operator()(vtkm::Id connIdx) const
  {
    return connIdx / this->EncodedCellSize;
  }
};

} // end anon namespace

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
    const vtkm::Id numberOfPoints = this->GetNumberOfPoints();
    const vtkm::Id numberOfCells = this->GetNumberOfCells();
    const vtkm::Id numberOfPointsPerCell = this->DetermineNumberOfPoints();
    const vtkm::Id rconnSize = numberOfCells*numberOfPointsPerCell;

    const SingleTypeRConnToConn rconnToConnCalc{numberOfPointsPerCell};
    const SingleTypeCellIdCalc cellIdCalc{numberOfPointsPerCell + 1}; // +1 for cell length entries

    vtkm::cont::internal::ReverseConnectivityBuilder builder;

    builder.Run(this->Connectivity,
                this->RConn,
                this->RNumIndices,
                this->RIndexOffsets,
                rconnToConnCalc,
                cellIdCalc,
                numberOfPoints,
                rconnSize,
                Device{});

    this->NumberOfPoints = this->RIndexOffsets.GetNumberOfValues();
    this->ReverseConnectivityBuilt = true;
  } // End if !RConnBuilt

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

#ifdef VTKM_ENABLE_OPENMP
template VTKACCELERATORSVTKM_EXPORT
  vtkm::exec::ConnectivityVTKSingleType<vtkm::cont::DeviceAdapterTagOpenMP>
    vtkmCellSetSingleType::PrepareForInput(vtkm::cont::DeviceAdapterTagOpenMP,
      vtkm::TopologyElementTagPoint, vtkm::TopologyElementTagCell) const;

template VTKACCELERATORSVTKM_EXPORT
  vtkm::exec::ReverseConnectivityVTK<vtkm::cont::DeviceAdapterTagOpenMP>
    vtkmCellSetSingleType::PrepareForInput(vtkm::cont::DeviceAdapterTagOpenMP,
      vtkm::TopologyElementTagCell, vtkm::TopologyElementTagPoint) const;
#endif

#ifdef VTKM_ENABLE_CUDA
template VTKACCELERATORSVTKM_EXPORT
  vtkm::exec::ConnectivityVTKSingleType<vtkm::cont::DeviceAdapterTagCuda>
    vtkmCellSetSingleType::PrepareForInput(vtkm::cont::DeviceAdapterTagCuda,
      vtkm::TopologyElementTagPoint, vtkm::TopologyElementTagCell) const;

template VTKACCELERATORSVTKM_EXPORT
  vtkm::exec::ReverseConnectivityVTK<vtkm::cont::DeviceAdapterTagCuda>
    vtkmCellSetSingleType::PrepareForInput(vtkm::cont::DeviceAdapterTagCuda,
      vtkm::TopologyElementTagCell, vtkm::TopologyElementTagPoint) const;
#endif
}
}
