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
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================
#include "CellSetConverters.h"

#include "ArrayConverters.h"
#include "DataSetConverters.h"
#include "Storage.h"

#include "vtkmCellSetExplicit.h"
#include "vtkmCellSetSingleType.h"
#include "vtkmConnectivityExec.h"
#include "vtkmFilterPolicy.h"

#include <vtkm/cont/serial/DeviceAdapterSerial.h>
#include <vtkm/cont/tbb/DeviceAdapterTBB.h>
#include <vtkm/cont/openmp/DeviceAdapterOpenMP.h>

#include <vtkm/cont/TryExecute.h>
#include <vtkm/worklet/DispatcherMapTopology.h>

#include "vtkIdTypeArray.h"
#include "vtkUnsignedCharArray.h"

namespace tovtkm {

namespace {

struct ReorderHex : public vtkm::exec::FunctorBase
{
  ReorderHex(): Data(nullptr) {}
  ReorderHex(vtkCellArray* fc): Data(fc->GetPointer()) {}

  void operator()(vtkm::Id index) const
  {
    const std::size_t offset = (index * 9) + 1;
    vtkIdType t = this->Data[offset+3];
    this->Data[offset+3] = this->Data[offset+2];
    this->Data[offset+2] = t;

    t = this->Data[offset+7];
    this->Data[offset+7] = this->Data[offset+6];
    this->Data[offset+6] = t;
  }

  vtkIdType* Data;
};

struct RunReorder
{
  RunReorder(): Reorder(), Size(0) {}
  RunReorder(vtkCellArray* fc, vtkm::Id size): Reorder(fc), Size(size) {}

  template<typename DeviceAdapter>
  bool operator()(DeviceAdapter ) const
  {
    using Algorithms =
        typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;
    Algorithms::Schedule(this->Reorder, this->Size);
    return true;
  }

  ReorderHex Reorder;
  vtkm::Id Size;
};

}

// convert a cell array of a single type to a vtkm CellSetSingleType
vtkm::cont::DynamicCellSet ConvertSingleType(vtkCellArray* cells, int cellType,
                                             vtkIdType numberOfPoints)
{
  // step 1, convert the integer into a tag type
  // step 2, wrap vtkCellArray in a custom array handle. The reason for the
  // wrapping is the stupid padding in the vtkCellArray that we need to work
  // around

  vtkm::cont::internal::Storage<vtkm::Id, tovtkm::vtkCellArrayContainerTag>
      storage(cells);
  vtkm::cont::ArrayHandle<vtkm::Id, tovtkm::vtkCellArrayContainerTag> handle(
      storage);

  // need to switch on cellType
  typedef vtkm::cont::vtkmCellSetSingleType CellSetType;
  switch (cellType)
  {
  case VTK_LINE:
  {
    CellSetType c(vtkm::CellShapeTagLine(), "cells");
    c.Fill(numberOfPoints, handle);
    return vtkm::cont::DynamicCellSet(c);
  }
  case VTK_HEXAHEDRON:
  {
    CellSetType c(vtkm::CellShapeTagHexahedron(), "cells");
    c.Fill(numberOfPoints, handle);
    return vtkm::cont::DynamicCellSet(c);
  }
  case VTK_VOXEL:
  {
    //This is encountered when you have an unstructured grid that was
    //cleaned / thresholded from an image data. At that point VTK should
    //have converted to hex's as downstream the point coordinates can be
    //transformed and invalidate the voxel requirements

    //We need to construct a new array that has the correct ordering
    //divide the array by 4, gets the number of times we need to flip values

    using SMPTypes = vtkm::ListTagBase<vtkm::cont::DeviceAdapterTagTBB,
                                       vtkm::cont::DeviceAdapterTagOpenMP,
                                       vtkm::cont::DeviceAdapterTagSerial>;
    // construct through vtkm so that the memory is properly
    // de-allocated when the DynamicCellSet is destroyed
    vtkm::cont::ArrayHandle<vtkm::Id, tovtkm::vtkCellArrayContainerTag>
        fixedCells;
    fixedCells.Allocate(cells->GetSize());
    fixedCells.GetStorage().VTKArray()->DeepCopy(cells);

    RunReorder run(fixedCells.GetStorage().VTKArray(), cells->GetNumberOfCells());
    vtkm::cont::TryExecute(run, SMPTypes());

    CellSetType c(vtkm::CellShapeTagHexahedron(), "cells");
    c.Fill(numberOfPoints, fixedCells);
    return vtkm::cont::DynamicCellSet(c);
  }
  case VTK_QUAD:
  {
    CellSetType c(vtkm::CellShapeTagQuad(), "cells");
    c.Fill(numberOfPoints, handle);
    return vtkm::cont::DynamicCellSet(c);
  }
  case VTK_TETRA:
  {
    CellSetType c(vtkm::CellShapeTagTetra(), "cells");
    c.Fill(numberOfPoints, handle);
    return vtkm::cont::DynamicCellSet(c);
  }
  case VTK_TRIANGLE:
  {
    CellSetType c(vtkm::CellShapeTagTriangle(), "cells");
    c.Fill(numberOfPoints, handle);
    return vtkm::cont::DynamicCellSet(c);
  }
  case VTK_VERTEX:
  {
    CellSetType c(vtkm::CellShapeTagVertex(), "cells");
    c.Fill(numberOfPoints, handle);
    return vtkm::cont::DynamicCellSet(c);
  }
  case VTK_WEDGE:
  {
    CellSetType c(vtkm::CellShapeTagWedge(), "cells");
    c.Fill(numberOfPoints, handle);
    return vtkm::cont::DynamicCellSet(c);
  }
  case VTK_PYRAMID:
  {
    CellSetType c(vtkm::CellShapeTagPyramid(), "cells");
    c.Fill(numberOfPoints, handle);
    return vtkm::cont::DynamicCellSet(c);
  }
  default:
    break;
  }

  return vtkm::cont::DynamicCellSet((CellSetType()));
}

// convert a cell array of mixed types to a vtkm CellSetExplicit
vtkm::cont::DynamicCellSet Convert(vtkUnsignedCharArray* types,
                                   vtkCellArray* cells,
                                   vtkIdTypeArray* locations,
                                   vtkIdType numberOfPoints)
{
  typedef vtkAOSDataArrayTemplate<vtkm::Id> DATIdType;
  typedef vtkAOSDataArrayTemplate<vtkm::UInt8> DATUInt8Type;

  typedef tovtkm::vtkAOSArrayContainerTag ArrayTag;
  typedef tovtkm::vtkCellArrayContainerTag CellArrayTag;

  // create the storage containers for everything
  vtkm::cont::internal::Storage<vtkIdType, CellArrayTag> cstorage(cells);
  vtkm::cont::internal::Storage<vtkm::UInt8, ArrayTag> tstorage(
      static_cast<DATUInt8Type*>(types));
  vtkm::cont::internal::Storage<vtkIdType, ArrayTag> lstorage(
      static_cast<DATIdType*>(locations));

  vtkm::cont::ArrayHandle<vtkIdType, CellArrayTag> chandle(cstorage);
  vtkm::cont::ArrayHandle<vtkm::UInt8, ArrayTag> thandle(tstorage);
  vtkm::cont::ArrayHandle<vtkIdType, ArrayTag> lhandle(lstorage);

  vtkm::cont::vtkmCellSetExplicitAOS cellset("cells");
  cellset.Fill(numberOfPoints, thandle, chandle, lhandle);
  return vtkm::cont::DynamicCellSet(cellset);
}

} // namespace tovtkm

namespace fromvtkm {

bool Convert(const vtkm::cont::DynamicCellSet& toConvert, vtkCellArray* cells,
             vtkUnsignedCharArray* types, vtkIdTypeArray* locations)
{
  const auto* cellset = toConvert.GetCellSetBase();

  // small hack as we can't compute properly the number of cells
  // instead we will pre-allocate and than shrink
  const vtkm::Id numCells = cellset->GetNumberOfCells();
  const vtkm::Id size = numCells * 9; // largest cell type is hex

  vtkIdTypeArray* connArray = vtkIdTypeArray::New();
  connArray->SetNumberOfComponents(1);
  connArray->SetNumberOfTuples(size);

  if (locations && types)
  {
    locations->SetNumberOfComponents(1);
    locations->SetNumberOfTuples(numCells);

    types->SetNumberOfComponents(1);
    types->SetNumberOfTuples(numCells);
  }

  vtkm::Id correctSize = 0;
  for (vtkm::Id i = 0; i < numCells; ++i)
  {
    const vtkm::Id numPointsPerCell = cellset->GetNumberOfPointsInCell(i);
    vtkm::Id index = correctSize;

    if (types)
    {
      types->SetValue(i, cellset->GetCellShape(i));
    }
    if (locations)
    {
      locations->SetValue(i, index);
    }

    // update the connectivity
    connArray->SetValue(index++, numPointsPerCell);
    vtkm::Id local[9]; // largest cell type is hex
    cellset->GetCellPointIds(i, local);
    for (vtkIdType j = 0; j < numPointsPerCell; ++j)
    {
      connArray->SetValue(index++, local[j]);
    }

    correctSize += 1 + numPointsPerCell;
  }

  connArray->Resize(correctSize);
  cells->SetCells(numCells, connArray);
  connArray->FastDelete();

  return true;
}
}
