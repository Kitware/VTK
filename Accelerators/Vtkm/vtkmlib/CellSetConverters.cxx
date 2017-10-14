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

namespace {

//------------------------------------------------------------------------------
struct CellInfoCopyWorklet : public vtkm::worklet::WorkletMapPointToCell
{
  CellInfoCopyWorklet(vtkm::Id* size, vtkIdTypeArray* conn,
                      vtkUnsignedCharArray* types, vtkIdTypeArray* locations)
    : ConnIndex(size), ConnArray(conn), Locations(locations), Shapes(types)
  {
  }

  typedef void ControlSignature(CellSetIn);
  typedef void ExecutionSignature(WorkIndex, CellShape, PointCount,
                                  PointIndices);
  typedef _1 InputDomain;

  template <typename CellShapeTag, typename IndicesVecType>
  void operator()(vtkm::Id i, CellShapeTag shape,
                  vtkm::Id numPointsPerCell,
                  const IndicesVecType& indices) const
  {
    vtkm::Id index = *this->ConnIndex;

    // save the shape tag
    this->Shapes->SetValue(i, shape.Id);

    // Visual Studio 2013 was giving warnings about shape not being used.
    // Why?!? It was clearly referenced above. This should shut it up.
    (void)shape;

    // update the offset location
    this->Locations->SetValue(i, index);

    // update the connectivity
    this->ConnArray->SetValue(index++, numPointsPerCell);
    for (vtkIdType j = 0; j < numPointsPerCell; ++j)
    {
      this->ConnArray->SetValue(index++, indices[j]);
    }

    // only update member variable once per iteration to improve locality
    *this->ConnIndex += 1 + numPointsPerCell;
  }

  vtkm::Id* ConnIndex;
  vtkIdTypeArray* ConnArray;
  vtkIdTypeArray* Locations;
  vtkUnsignedCharArray* Shapes;
};

//------------------------------------------------------------------------------
struct CellConnCopyWorklet : public vtkm::worklet::WorkletMapPointToCell
{
  CellConnCopyWorklet(vtkm::Id* size, vtkIdTypeArray* conn)
    : ConnIndex(size), ConnArray(conn)
  {
  }

  typedef void ControlSignature(CellSetIn);
  typedef void ExecutionSignature(PointCount, PointIndices);
  typedef _1 InputDomain;

  template <typename IndicesVecType>
  void operator()(vtkm::Id numPointsPerCell,
                  const IndicesVecType& indices) const
  {
    vtkm::Id index = *this->ConnIndex;

    // update the connectivity
    this->ConnArray->SetValue(index++, numPointsPerCell);
    for (vtkIdType j = 0; j < numPointsPerCell; ++j)
    {
      this->ConnArray->SetValue(index++, static_cast<vtkm::Id>(indices[j]));
    }
    // only update member variable once per iteration to improve locality
    *this->ConnIndex += 1 + numPointsPerCell;
  }

  vtkm::Id* ConnIndex;
  vtkIdTypeArray* ConnArray;
};

//------------------------------------------------------------------------------
struct CellSetConverter
{
  CellSetConverter(bool* didConversion, vtkCellArray* cells,
                   vtkUnsignedCharArray* types, vtkIdTypeArray* locations)
    : Cells(cells), Types(types), Locations(locations), Valid(didConversion)
  {
  }

  ~CellSetConverter()
  {
    this->Cells = nullptr;
    this->Valid = nullptr;
  }

  template <typename T> void operator()(const T& cells) const
  {

    if (this->Cells)
    {
      // small hack as we can't compute properly the number of cells
      // instead we will pre-allocate and than shrink
      const vtkm::Id numCells = cells.GetNumberOfCells();
      const vtkm::Id size = numCells * 9; // largest cell type is hex
      vtkm::Id correctSize = 0;
      vtkIdTypeArray* connArray = vtkIdTypeArray::New();
      connArray->SetNumberOfComponents(1);
      connArray->SetNumberOfTuples(size);

      // These have to be done with the serial back-end only as they
      // aren't safe for parallelization. We only are using the dispatcher
      // to provide a uniform api for accessing cells
      if (this->Locations && this->Types)
      {
        this->Locations->SetNumberOfComponents(1);
        this->Locations->SetNumberOfTuples(numCells);

        this->Types->SetNumberOfComponents(1);
        this->Types->SetNumberOfTuples(numCells);

        CellInfoCopyWorklet worklet(&correctSize, connArray, this->Types,
                                    this->Locations);
        vtkm::worklet::DispatcherMapTopology<CellInfoCopyWorklet,
                                             vtkm::cont::DeviceAdapterTagSerial>
            dispatcher(worklet);

        dispatcher.Invoke(cells);
      }
      else
      {
        CellConnCopyWorklet worklet(&correctSize, connArray);
        vtkm::worklet::DispatcherMapTopology<CellConnCopyWorklet,
                                             vtkm::cont::DeviceAdapterTagSerial>
            dispatcher(worklet);

        dispatcher.Invoke(cells);
      }

      connArray->Resize(correctSize);
      this->Cells->SetCells(numCells, connArray);
      connArray->FastDelete();
      *this->Valid = true;
      return;
    }

    *this->Valid = false;
  }

  vtkCellArray* Cells;
  vtkUnsignedCharArray* Types;
  vtkIdTypeArray* Locations;
  bool* Valid;
};

} // namespace

bool Convert(const vtkm::cont::DynamicCellSet& toConvert, vtkCellArray* cells,
             vtkUnsignedCharArray* types, vtkIdTypeArray* locations)
{
  vtkmOutputFilterPolicy policy;
  bool didConversion = false;
  CellSetConverter cConverter(&didConversion, cells, types, locations);
  vtkm::filter::ApplyPolicy(toConvert, policy).CastAndCall(cConverter);
  return didConversion;
}
}
