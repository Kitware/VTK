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

#include <vtkm/cont/ArrayHandleImplicit.h>
#include <vtkm/cont/internal/ReverseConnectivityBuilder.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/DispatcherMapField.h>

#include <utility>

namespace
{

// Converts [0, rconnSize) to [0, connSize), skipping cell length entries.
template <typename Device>
struct ExplicitRConnToConn
{
  using OffsetsArray = vtkm::cont::ArrayHandle<vtkm::Id, tovtkm::vtkAOSArrayContainerTag>;
  using OffsetsPortal = decltype(std::declval<OffsetsArray>().PrepareForInput(Device()));

  // Functor that modifies the offsets array so we can compute point id indices
  // Output is:
  // modOffset[i] = offsets[i] - i
  struct OffsetsModifier
  {
    OffsetsPortal Offsets;

    VTKM_CONT
    OffsetsModifier(const OffsetsPortal& offsets = OffsetsPortal{})
      : Offsets(offsets)
    {
    }

    VTKM_EXEC
    vtkm::Id operator()(vtkm::Id inIdx) const
    {
      return this->Offsets.Get(inIdx) - inIdx;
    }
  };

  using ModOffsetsArray = vtkm::cont::ArrayHandleImplicit<OffsetsModifier>;
  using ModOffsetsPortal = decltype(std::declval<ModOffsetsArray>().PrepareForInput(Device()));

  VTKM_CONT
  ExplicitRConnToConn(const ModOffsetsPortal& offsets)
    : Offsets(offsets)
  {
  }

  VTKM_EXEC
  vtkm::Id operator()(vtkm::Id rConnIdx) const
  {
    // Compute the connectivity array index (skipping cell length entries)
    // The number of cell length entries can be found by taking the index of
    // the upper bound of inIdx in Offsets and adding it to inIdx.
    //
    // Example:
    // Conn:  |  3  X  X  X  |  4  X  X  X  X  |  3  X  X  X  |  2  X  X  |
    // Idx:   |  0  1  2  3  |  4  5  6  7  8  |  9  10 11 12 |  13 14 15 |
    // InIdx:       0  1  2        3  4  5  6  |     7  8  9        10 11
    //
    // ModOffset[i] = Offsets[i] - i:
    // Offsets:     0  4  9  13 (16)
    // ModOffsets:  0  3  7  10 (12)
    //
    // Define UB(x) to return the index of the upper bound of x in ModOffsets,
    // the i'th point index's location in Conn is computed as:
    // OutId = UB(InIdx) + InIdx
    //
    // This gives us the expected out indices:
    // InIdx:     0  1  2  3  4  5  6  7  8  9  10 11
    // UB(InIdx): 1  1  1  2  2  2  2  3  3  3  4  4
    // OutIdx:    1  2  3  5  6  7  8  10 11 12 14 15
    const vtkm::Id uBIdx = this->UpperBoundIdx(rConnIdx);
    const vtkm::Id connIdx = rConnIdx + uBIdx;
    return connIdx;
  }

private:

  ModOffsetsPortal Offsets;

  VTKM_EXEC
  vtkm::Id UpperBoundIdx(vtkm::Id inIdx) const
  {
    vtkm::Id first = 0;
    vtkm::Id length = this->Offsets.GetNumberOfValues();

    while (length > 0)
    {
      vtkm::Id half = length / 2;
      vtkm::Id pos = first + half;
      vtkm::Id val = this->Offsets.Get(pos);
      if (val <= inIdx)
      {
        first = pos + 1;
        length -= half + 1;
      }
      else
      {
        length = half;
      }
    }

    return first;
  }
};

// Converts a connectivity index to a cell id:
template <typename Device>
struct ExplicitCellIdCalc
{
  using OffsetsArray = vtkm::cont::ArrayHandle<vtkm::Id, tovtkm::vtkAOSArrayContainerTag>;
  using OffsetsPortal = decltype(std::declval<OffsetsArray>().PrepareForInput(Device()));

  vtkm::Id ConnSize;
  OffsetsPortal Offsets;

  VTKM_CONT
  ExplicitCellIdCalc(vtkm::Id connSize, const OffsetsPortal& offsets)
    : ConnSize(connSize)
    , Offsets(offsets)
  {
  }

  // Computes the cellid of the connectivity index i.
  //
  // For a mixed-cell connectivity, the offset table is used to compute
  // the cell id.
  //
  // Example:
  // Conn:   |  3  X  X  X  |  4  X  X  X  X  |  3  X  X  X  |  2  X  X  |
  // Idx:    |     1  2  3  |     5  6  7  8  |     10 11 12 |     14 15 |
  //
  // Offsets:    0  4  9  13
  // ModOffsets: 4  9  13 16
  //
  // Computing the index of the lower bound in ModOffsets for each Idx gives
  // the expected cell id values:
  // CellId: |     0  0  0  |     1  1  1  1  |     2  2  2  |     3  3  |
  VTKM_EXEC
  vtkm::Id operator()(vtkm::Id i) const
  {
    return this->LowerBound(i);
  }

private:
  /// Returns the i+1 offset, or the full size of the connectivity if at end.
  VTKM_EXEC
  vtkm::Id GetModifiedOffset(vtkm::Id i) const
  {
    ++i;
    return (i >= this->Offsets.GetNumberOfValues()) ? this->ConnSize
                                                    : this->Offsets.Get(i);
  }

  VTKM_EXEC
  vtkm::Id LowerBound(vtkm::Id inVal) const
  {
    vtkm::Id first = 0;
    vtkm::Id length = this->Offsets.GetNumberOfValues();

    while (length > 0)
    {
      vtkm::Id half = length / 2;
      vtkm::Id pos = first + half;
      vtkm::Id val = this->GetModifiedOffset(pos);
      if (val < inVal)
      {
        first = pos + 1;
        length -= half + 1;
      }
      else
      {
        length = half;
      }
    }

    return first;

  }
};

} // end anon namespace

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
    const vtkm::Id numberOfPoints = this->GetNumberOfPoints();
    const vtkm::Id connectivityLength = this->Connectivity.GetNumberOfValues();
    const vtkm::Id rconnSize = connectivityLength - this->IndexOffsets.GetNumberOfValues();

    auto offsetsPortal = this->IndexOffsets.PrepareForInput(Device());
    typename ExplicitRConnToConn<Device>::OffsetsModifier offsetModifier{offsetsPortal};
    auto modOffsets = vtkm::cont::make_ArrayHandleImplicit(offsetModifier,
                                                           this->IndexOffsets.GetNumberOfValues());

    const ExplicitRConnToConn<Device> rconnToConnCalc{modOffsets.PrepareForInput(Device())};
    const ExplicitCellIdCalc<Device> cellIdCalc{this->Connectivity.GetNumberOfValues(),
                                                this->IndexOffsets.PrepareForInput(Device())};

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

#ifdef VTKM_ENABLE_OPENMP
template VTKACCELERATORSVTKM_EXPORT
  vtkm::exec::ConnectivityVTKAOS<vtkm::cont::DeviceAdapterTagOpenMP>
    vtkmCellSetExplicitAOS::PrepareForInput(vtkm::cont::DeviceAdapterTagOpenMP,
      vtkm::TopologyElementTagPoint, vtkm::TopologyElementTagCell) const;

template VTKACCELERATORSVTKM_EXPORT
  vtkm::exec::ReverseConnectivityVTK<vtkm::cont::DeviceAdapterTagOpenMP>
    vtkmCellSetExplicitAOS::PrepareForInput(vtkm::cont::DeviceAdapterTagOpenMP,
      vtkm::TopologyElementTagCell, vtkm::TopologyElementTagPoint) const;
#endif

}
}
