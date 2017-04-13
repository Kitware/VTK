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
#ifndef vtkmCellSetExplicit_h
#define vtkmCellSetExplicit_h

#include "vtkmTags.h"

#include <vtkm/CellShape.h>
#include <vtkm/TopologyElementTag.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/CellSet.h>

#include <vtkm/cont/serial/DeviceAdapterSerial.h>
#include <vtkm/cont/cuda/DeviceAdapterCuda.h>
#include <vtkm/cont/tbb/DeviceAdapterTBB.h>

#include "vtkmConnectivityExec.h"

namespace vtkm {
namespace cont {

class VTKACCELERATORSVTKM_EXPORT vtkmCellSetExplicitAOS : public CellSet
{
public:
  vtkmCellSetExplicitAOS(const std::string& name = std::string())
    : CellSet(name), Shapes(), Connectivity(), IndexOffsets(),
      ReverseConnectivityBuilt(false),RConn(), RNumIndices(), RIndexOffsets(),
      NumberOfPoints(0)
  {
  }

  virtual ~vtkmCellSetExplicitAOS()
  {
  }

  vtkmCellSetExplicitAOS& operator=(const vtkmCellSetExplicitAOS& src)
  {
    this->CellSet::operator=(src);
    this->Shapes = src.Shapes;
    this->Connectivity = src.Connectivity;
    this->IndexOffsets = src.IndexOffsets;
    this->ReverseConnectivityBuilt = src.ReverseConnectivityBuilt;
    this->RConn = src.RConn;
    this->RNumIndices = src.RNumIndices;
    this->RIndexOffsets = src.RIndexOffsets;
    this->NumberOfPoints = src.NumberOfPoints;
    return *this;
  }

  vtkm::Id GetNumberOfCells() const
  {
    return this->Shapes.GetNumberOfValues();
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

  vtkm::IdComponent GetNumberOfPointsInCell(vtkm::Id index) const;

  vtkm::Id GetCellShape(vtkm::Id index) const;

  /// Assigns the array handles to the explicit connectivity. This is
  /// the way you can fill the memory from another system without copying
  void Fill(
      vtkm::Id numberOfPoints,
      const vtkm::cont::ArrayHandle<vtkm::UInt8,
                                    tovtkm::vtkAOSArrayContainerTag>& cellTypes,
      const vtkm::cont::ArrayHandle<vtkm::Id, tovtkm::vtkCellArrayContainerTag>&
          connectivity,
      const vtkm::cont::ArrayHandle<vtkm::Id, tovtkm::vtkAOSArrayContainerTag>&
          offsets);

  template <typename DeviceAdapter, typename FromTopology, typename ToTopology>
  struct ExecutionTypes;

  template <typename DeviceAdapter>
  struct ExecutionTypes<DeviceAdapter, vtkm::TopologyElementTagPoint, vtkm::TopologyElementTagCell>
  {
    typedef vtkm::exec::ConnectivityVTKAOS<DeviceAdapter> ExecObjectType;
  };

  template <typename DeviceAdapter>
  struct ExecutionTypes<DeviceAdapter, vtkm::TopologyElementTagCell, vtkm::TopologyElementTagPoint>
  {
    typedef vtkm::exec::ReverseConnectivityVTK<DeviceAdapter> ExecObjectType;
  };

  template <typename Device>
  typename vtkm::exec::ConnectivityVTKAOS<Device>
      PrepareForInput(Device, vtkm::TopologyElementTagPoint,
                      vtkm::TopologyElementTagCell) const;

  template <typename Device>
  typename vtkm::exec::ReverseConnectivityVTK<Device>
      PrepareForInput(Device, vtkm::TopologyElementTagCell,
                      vtkm::TopologyElementTagPoint) const;

  const vtkm::cont::ArrayHandle<vtkm::UInt8, tovtkm::vtkAOSArrayContainerTag>&
      GetShapesArray(vtkm::TopologyElementTagPoint,
                     vtkm::TopologyElementTagCell) const
  {
    return this->Shapes;
  }

  const vtkm::cont::ArrayHandle<vtkm::Id, tovtkm::vtkCellArrayContainerTag>&
      GetConnectivityArray(vtkm::TopologyElementTagPoint,
                           vtkm::TopologyElementTagCell) const
  {
    return this->Connectivity;
  }

  const vtkm::cont::ArrayHandle<vtkm::Id, tovtkm::vtkAOSArrayContainerTag>&
      GetIndexOffsetArray(vtkm::TopologyElementTagPoint,
                          vtkm::TopologyElementTagCell) const
  {
    return this->IndexOffsets;
  }

  virtual void PrintSummary(std::ostream& out) const;

private:
  vtkm::cont::ArrayHandle<vtkm::UInt8, tovtkm::vtkAOSArrayContainerTag> Shapes;
  vtkm::cont::ArrayHandle<vtkm::Id, tovtkm::vtkCellArrayContainerTag>
      Connectivity;
  vtkm::cont::ArrayHandle<vtkm::Id, tovtkm::vtkAOSArrayContainerTag>
      IndexOffsets;

  //Reverse connectivity (cell -> point)
  //Todo: Need a better way to represent that PrepareForInput is a
  //      non-const operation, but that is going to take some real
  //      code refactoring in vtk-m
  mutable bool ReverseConnectivityBuilt;
  mutable vtkm::cont::ArrayHandle<vtkm::Id> RConn;
  mutable vtkm::cont::ArrayHandle<vtkm::IdComponent> RNumIndices;
  mutable vtkm::cont::ArrayHandle<vtkm::Id> RIndexOffsets;
  mutable vtkm::Id NumberOfPoints;

};

// template methods we want to compile only once
extern template VTKACCELERATORSVTKM_TEMPLATE_EXPORT
  vtkm::exec::ConnectivityVTKAOS<vtkm::cont::DeviceAdapterTagSerial>
    vtkmCellSetExplicitAOS::PrepareForInput(vtkm::cont::DeviceAdapterTagSerial,
      vtkm::TopologyElementTagPoint, vtkm::TopologyElementTagCell) const;

extern template VTKACCELERATORSVTKM_TEMPLATE_EXPORT
  vtkm::exec::ReverseConnectivityVTK<vtkm::cont::DeviceAdapterTagSerial>
    vtkmCellSetExplicitAOS::PrepareForInput(vtkm::cont::DeviceAdapterTagSerial,
      vtkm::TopologyElementTagCell, vtkm::TopologyElementTagPoint) const;

#ifdef VTKM_ENABLE_TBB
extern template VTKACCELERATORSVTKM_TEMPLATE_EXPORT
  vtkm::exec::ConnectivityVTKAOS<vtkm::cont::DeviceAdapterTagTBB>
    vtkmCellSetExplicitAOS::PrepareForInput(vtkm::cont::DeviceAdapterTagTBB,
      vtkm::TopologyElementTagPoint, vtkm::TopologyElementTagCell) const;

extern template VTKACCELERATORSVTKM_TEMPLATE_EXPORT
  vtkm::exec::ReverseConnectivityVTK<vtkm::cont::DeviceAdapterTagTBB>
    vtkmCellSetExplicitAOS::PrepareForInput(vtkm::cont::DeviceAdapterTagTBB,
      vtkm::TopologyElementTagCell, vtkm::TopologyElementTagPoint) const;
#endif

#if defined(VTKM_ENABLE_CUDA) && defined(VTKM_CUDA)
extern template VTKACCELERATORSVTKM_TEMPLATE_EXPORT
  vtkm::exec::ConnectivityVTKAOS<vtkm::cont::DeviceAdapterTagCuda>
    vtkmCellSetExplicitAOS::PrepareForInput(vtkm::cont::DeviceAdapterTagCuda,
      vtkm::TopologyElementTagPoint, vtkm::TopologyElementTagCell) const;

extern template VTKACCELERATORSVTKM_TEMPLATE_EXPORT
  vtkm::exec::ReverseConnectivityVTK<vtkm::cont::DeviceAdapterTagCuda>
    vtkmCellSetExplicitAOS::PrepareForInput(vtkm::cont::DeviceAdapterTagCuda,
      vtkm::TopologyElementTagCell, vtkm::TopologyElementTagPoint) const;
#endif
}
}

#endif
// VTK-HeaderTest-Exclude: vtkmCellSetExplicit.h
