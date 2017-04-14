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
#ifndef vtkmCellSetSingleType_h
#define vtkmCellSetSingleType_h

#include "vtkmTags.h"

#include <vtkm/CellShape.h>
#include <vtkm/CellTraits.h>
#include <vtkm/TopologyElementTag.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/CellSet.h>

#include <vtkm/VecFromPortal.h>

#include <vtkm/cont/serial/DeviceAdapterSerial.h>
#include <vtkm/cont/cuda/DeviceAdapterCuda.h>
#include <vtkm/cont/tbb/DeviceAdapterTBB.h>

#include "vtkmConnectivityExec.h"

namespace vtkm {
namespace cont {

class VTKACCELERATORSVTKM_EXPORT vtkmCellSetSingleType : public CellSet
{
  typedef tovtkm::vtkCellArrayContainerTag ConnectivityStorageTag;

public:
  vtkmCellSetSingleType()
    : CellSet((std::string())),
      NumberOfCells(0),
      NumberOfPoints(0),
      CellTypeAsId(CellShapeTagEmpty::Id),
      Connectivity(),
      ReverseConnectivityBuilt(false),
      RConn(),
      RNumIndices(),
      RIndexOffsets()
  {
  }

  template <typename CellShapeTag>
  vtkmCellSetSingleType(CellShapeTag, const std::string& name)
    : CellSet(name),
      NumberOfCells(0),
      NumberOfPoints(0),
      CellTypeAsId(CellShapeTag::Id),
      Connectivity(),
      ReverseConnectivityBuilt(false),
      RConn(),
      RNumIndices(),
      RIndexOffsets()
  {
  }

  vtkmCellSetSingleType& operator=(const vtkmCellSetSingleType& src)
  {
    this->CellSet::operator=(src);
    this->NumberOfCells = src.NumberOfCells;
    this->NumberOfPoints = src.NumberOfPoints;
    this->CellTypeAsId = src.CellTypeAsId;
    this->ReverseConnectivityBuilt = src.ReverseConnectivityBuilt;
    this->Connectivity = src.Connectivity;
    this->RConn = src.RConn;
    this->RNumIndices = src.RNumIndices;
    this->RIndexOffsets  = src.RIndexOffsets;
    return *this;
  }

  vtkm::Id GetNumberOfCells() const
  {
    return this->NumberOfCells;
  }

  vtkm::Id GetNumberOfPoints() const
  {
    return this->NumberOfPoints;
  }

  virtual vtkm::Id GetNumberOfFaces() const { return -1; }

  virtual vtkm::Id GetNumberOfEdges() const { return -1; }

  vtkm::Id GetSchedulingRange(vtkm::TopologyElementTagCell) const
  {
    return this->GetNumberOfCells();
  }

  vtkm::Id GetSchedulingRange(vtkm::TopologyElementTagPoint) const
  {
    return this->GetNumberOfPoints();
  }

  // This is the way you can fill the memory from another system without copying
  void Fill(
      vtkm::Id numberOfPoints,
      const vtkm::cont::ArrayHandle<vtkm::Id, tovtkm::vtkCellArrayContainerTag>&
          connectivity);

  template <typename DeviceAdapter, typename FromTopology, typename ToTopology>
  struct ExecutionTypes;

  template <typename DeviceAdapter>
  struct ExecutionTypes<DeviceAdapter, vtkm::TopologyElementTagPoint,
                        vtkm::TopologyElementTagCell>
  {
    typedef vtkm::exec::ConnectivityVTKSingleType<DeviceAdapter> ExecObjectType;
  };

  template <typename DeviceAdapter>
  struct ExecutionTypes<DeviceAdapter, vtkm::TopologyElementTagCell,
                        vtkm::TopologyElementTagPoint>
  {
    typedef vtkm::exec::ReverseConnectivityVTK<DeviceAdapter> ExecObjectType;
  };

  template <typename Device>
  typename vtkm::exec::ConnectivityVTKSingleType<Device>
      PrepareForInput(Device, vtkm::TopologyElementTagPoint,
                      vtkm::TopologyElementTagCell) const;

  template <typename Device>
  typename vtkm::exec::ReverseConnectivityVTK<Device>
      PrepareForInput(Device, vtkm::TopologyElementTagCell,
                      vtkm::TopologyElementTagPoint) const;


  const vtkm::cont::ArrayHandle<vtkm::Id, tovtkm::vtkCellArrayContainerTag>&
      GetConnectivityArray(vtkm::TopologyElementTagPoint,
                           vtkm::TopologyElementTagCell) const
  {
    return this->Connectivity;
  }

  virtual void PrintSummary(std::ostream& out) const;

private:
  template <typename CellShapeTag>
  void DetermineNumberOfPoints(CellShapeTag, vtkm::CellTraitsTagSizeFixed,
                               vtkm::IdComponent& numberOfPoints) const
  {
    numberOfPoints = vtkm::CellTraits<CellShapeTag>::NUM_POINTS;
  }

  template <typename CellShapeTag>
  void DetermineNumberOfPoints(CellShapeTag, vtkm::CellTraitsTagSizeVariable,
                               vtkm::IdComponent& numberOfPoints) const
  { // variable length cells can't be used with this class
    numberOfPoints = -1;
  }

  vtkm::IdComponent DetermineNumberOfPoints() const;

  vtkm::Id NumberOfCells;
  mutable vtkm::Id NumberOfPoints;
  vtkm::Id CellTypeAsId;
  vtkm::cont::ArrayHandle<vtkm::Id, ConnectivityStorageTag> Connectivity;

  mutable bool ReverseConnectivityBuilt;
  mutable vtkm::cont::ArrayHandle<vtkm::Id> RConn;
  mutable vtkm::cont::ArrayHandle<vtkm::IdComponent> RNumIndices;
  mutable vtkm::cont::ArrayHandle<vtkm::Id> RIndexOffsets;
};

// template methods we want to compile only once
extern template VTKACCELERATORSVTKM_TEMPLATE_EXPORT
  vtkm::exec::ConnectivityVTKSingleType<vtkm::cont::DeviceAdapterTagSerial>
    vtkmCellSetSingleType::PrepareForInput(vtkm::cont::DeviceAdapterTagSerial,
      vtkm::TopologyElementTagPoint, vtkm::TopologyElementTagCell) const;

extern template VTKACCELERATORSVTKM_TEMPLATE_EXPORT
  vtkm::exec::ReverseConnectivityVTK<vtkm::cont::DeviceAdapterTagSerial>
    vtkmCellSetSingleType::PrepareForInput(vtkm::cont::DeviceAdapterTagSerial,
      vtkm::TopologyElementTagCell, vtkm::TopologyElementTagPoint) const;

#ifdef VTKM_ENABLE_TBB
extern template VTKACCELERATORSVTKM_TEMPLATE_EXPORT
  vtkm::exec::ConnectivityVTKSingleType<vtkm::cont::DeviceAdapterTagTBB>
    vtkmCellSetSingleType::PrepareForInput(vtkm::cont::DeviceAdapterTagTBB,
      vtkm::TopologyElementTagPoint, vtkm::TopologyElementTagCell) const;

extern template VTKACCELERATORSVTKM_TEMPLATE_EXPORT
  vtkm::exec::ReverseConnectivityVTK<vtkm::cont::DeviceAdapterTagTBB>
    vtkmCellSetSingleType::PrepareForInput(vtkm::cont::DeviceAdapterTagTBB,
      vtkm::TopologyElementTagCell, vtkm::TopologyElementTagPoint) const;
#endif

#if defined(VTKM_ENABLE_CUDA) && defined(VTKM_CUDA)
extern template VTKACCELERATORSVTKM_TEMPLATE_EXPORT
  vtkm::exec::ConnectivityVTKSingleType<vtkm::cont::DeviceAdapterTagCuda>
    vtkmCellSetSingleType::PrepareForInput(vtkm::cont::DeviceAdapterTagCuda,
      vtkm::TopologyElementTagPoint, vtkm::TopologyElementTagCell) const;

extern template VTKACCELERATORSVTKM_TEMPLATE_EXPORT
  vtkm::exec::ReverseConnectivityVTK<vtkm::cont::DeviceAdapterTagCuda>
    vtkmCellSetSingleType::PrepareForInput(vtkm::cont::DeviceAdapterTagCuda,
      vtkm::TopologyElementTagCell, vtkm::TopologyElementTagPoint) const;
#endif
}
} // namespace vtkm::cont

#endif // vtkmlib_vtkmCellSetSingleType_h
// VTK-HeaderTest-Exclude: vtkmCellSetSingleType.h
