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
#ifndef viskores_rendering_Quadralizer_h
#define viskores_rendering_Quadralizer_h

#include <typeinfo>
#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/CellSetPermutation.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/UncertainCellSet.h>
#include <viskores/rendering/raytracing/MeshConnectivityBuilder.h>
#include <viskores/worklet/DispatcherMapTopology.h>
#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/WorkletMapTopology.h>


#define QUAD_PER_CSS 6

namespace viskores
{
namespace rendering
{

class Quadralizer
{
public:
  class CountQuads : public viskores::worklet::WorkletVisitCellsWithPoints
  {
  public:
    VISKORES_CONT
    CountQuads() {}
    typedef void ControlSignature(CellSetIn cellset, FieldOut);
    typedef void ExecutionSignature(CellShape, _2);

    VISKORES_EXEC
    void operator()(viskores::CellShapeTagGeneric shapeType, viskores::Id& quads) const
    {
      if (shapeType.Id == viskores::CELL_SHAPE_QUAD)
        quads = 1;
      else if (shapeType.Id == CELL_SHAPE_HEXAHEDRON)
        quads = 6;
      else if (shapeType.Id == viskores::CELL_SHAPE_WEDGE)
        quads = 3;
      else if (shapeType.Id == viskores::CELL_SHAPE_PYRAMID)
        quads = 1;

      else
        quads = 0;
    }

    VISKORES_EXEC
    void operator()(viskores::CellShapeTagHexahedron viskoresNotUsed(shapeType),
                    viskores::Id& quads) const
    {
      quads = 6;
    }

    VISKORES_EXEC
    void operator()(viskores::CellShapeTagQuad shapeType, viskores::Id& quads) const
    {
      if (shapeType.Id == viskores::CELL_SHAPE_QUAD)
        quads = 1;
      else
        quads = 0;
    }
    VISKORES_EXEC
    void operator()(viskores::CellShapeTagWedge viskoresNotUsed(shapeType),
                    viskores::Id& quads) const
    {
      quads = 3;
    }
  }; //class CountQuads

  template <int DIM>
  class SegmentedStructured : public viskores::worklet::WorkletVisitCellsWithPoints
  {

  public:
    typedef void ControlSignature(CellSetIn cellset, FieldInCell, WholeArrayOut);
    typedef void ExecutionSignature(IncidentElementIndices, _2, _3);
    //typedef _1 InputDomain;
    VISKORES_CONT
    SegmentedStructured() {}

#if defined(VISKORES_MSVC)
#pragma warning(push)
#pragma warning(disable : 4127) //conditional expression is constant
#endif
    template <typename CellNodeVecType, typename OutIndicesPortal>
    VISKORES_EXEC void cell2quad(viskores::Id4 idx,
                                 viskores::Vec<Id, 5>& quad,
                                 const viskores::Id offset,
                                 const CellNodeVecType& cellIndices,
                                 OutIndicesPortal& outputIndices) const
    {

      quad[1] = cellIndices[viskores::IdComponent(idx[0])];
      quad[2] = cellIndices[viskores::IdComponent(idx[1])];
      quad[3] = cellIndices[viskores::IdComponent(idx[2])];
      quad[4] = cellIndices[viskores::IdComponent(idx[3])];
      outputIndices.Set(offset, quad);
    }

    template <typename CellNodeVecType, typename OutIndicesPortal>
    VISKORES_EXEC void operator()(const CellNodeVecType& cellIndices,
                                  const viskores::Id& cellIndex,
                                  OutIndicesPortal& outputIndices) const
    {
      if (DIM == 2)
      {
        outputIndices.Set(
          cellIndex, { cellIndex, cellIndices[0], cellIndices[1], cellIndices[2], cellIndices[3] });
      }
      else if (DIM == 3)
      {
        viskores::Id offset = cellIndex * QUAD_PER_CSS;
        viskores::Vec<viskores::Id, 5> quad;
        quad[0] = cellIndex;
        viskores::Id4 idx;
        idx[0] = 0;
        idx[1] = 1;
        idx[2] = 5;
        idx[3] = 4;
        cell2quad(idx, quad, offset, cellIndices, outputIndices);

        idx[0] = 1;
        idx[1] = 2;
        idx[2] = 6;
        idx[3] = 5;
        offset++;
        cell2quad(idx, quad, offset, cellIndices, outputIndices);

        idx[0] = 3;
        idx[1] = 7;
        idx[2] = 6;
        idx[3] = 2;
        offset++;
        cell2quad(idx, quad, offset, cellIndices, outputIndices);

        idx[0] = 0;
        idx[1] = 4;
        idx[2] = 7;
        idx[3] = 3;
        offset++;
        cell2quad(idx, quad, offset, cellIndices, outputIndices);

        idx[0] = 0;
        idx[1] = 3;
        idx[2] = 2;
        idx[3] = 1;
        offset++;
        cell2quad(idx, quad, offset, cellIndices, outputIndices);

        idx[0] = 4;
        idx[1] = 5;
        idx[2] = 6;
        idx[3] = 7;
        offset++;
        cell2quad(idx, quad, offset, cellIndices, outputIndices);
      }
    }
#if defined(VISKORES_MSVC)
#pragma warning(pop)
#endif
  };


  class Quadralize : public viskores::worklet::WorkletVisitCellsWithPoints
  {

  public:
    VISKORES_CONT
    Quadralize() {}
    typedef void ControlSignature(CellSetIn cellset, FieldInCell, WholeArrayOut);
    typedef void ExecutionSignature(_2, CellShape, PointIndices, WorkIndex, _3);

    template <typename VecType, typename OutputPortal>
    VISKORES_EXEC void cell2quad(viskores::Id& offset,
                                 const VecType& cellIndices,
                                 const viskores::Id& cellId,
                                 const viskores::Id Id0,
                                 const viskores::Id Id1,
                                 const viskores::Id Id2,
                                 const viskores::Id Id3,
                                 OutputPortal& outputIndices) const
    {
      viskores::Vec<viskores::Id, 5> quad;
      quad[0] = cellId;
      quad[1] = static_cast<viskores::Id>(cellIndices[viskores::IdComponent(Id0)]);
      quad[2] = static_cast<viskores::Id>(cellIndices[viskores::IdComponent(Id1)]);
      quad[3] = static_cast<viskores::Id>(cellIndices[viskores::IdComponent(Id2)]);
      quad[4] = static_cast<viskores::Id>(cellIndices[viskores::IdComponent(Id3)]);
      outputIndices.Set(offset++, quad);
    }

    template <typename VecType, typename OutputPortal>
    VISKORES_EXEC void operator()(const viskores::Id& pointOffset,
                                  viskores::CellShapeTagWedge viskoresNotUsed(shapeType),
                                  const VecType& cellIndices,
                                  const viskores::Id& cellId,
                                  OutputPortal& outputIndices) const
    {
      viskores::Id offset = pointOffset;

      cell2quad(offset, cellIndices, cellId, 3, 0, 2, 5, outputIndices);
      cell2quad(offset, cellIndices, cellId, 1, 4, 5, 2, outputIndices);
      cell2quad(offset, cellIndices, cellId, 0, 3, 4, 1, outputIndices);
    }
    template <typename VecType, typename OutputPortal>
    VISKORES_EXEC void operator()(const viskores::Id& offset,
                                  viskores::CellShapeTagQuad shapeType,
                                  const VecType& cellIndices,
                                  const viskores::Id& cellId,
                                  OutputPortal& outputIndices) const
    {
      if (shapeType.Id == viskores::CELL_SHAPE_QUAD)
      {
        viskores::Vec<viskores::Id, 5> quad;
        quad[0] = cellId;
        quad[1] = static_cast<viskores::Id>(cellIndices[0]);
        quad[2] = static_cast<viskores::Id>(cellIndices[1]);
        quad[3] = static_cast<viskores::Id>(cellIndices[2]);
        quad[4] = static_cast<viskores::Id>(cellIndices[3]);
        outputIndices.Set(offset, quad);
      }
    }

    template <typename VecType, typename OutputPortal>
    VISKORES_EXEC void operator()(const viskores::Id& pointOffset,
                                  viskores::CellShapeTagHexahedron viskoresNotUsed(shapeType),
                                  const VecType& cellIndices,
                                  const viskores::Id& cellId,
                                  OutputPortal& outputIndices) const

    {
      viskores::Id offset = pointOffset;
      cell2quad(offset, cellIndices, cellId, 0, 1, 5, 4, outputIndices);
      cell2quad(offset, cellIndices, cellId, 1, 2, 6, 5, outputIndices);
      cell2quad(offset, cellIndices, cellId, 3, 7, 6, 2, outputIndices);
      cell2quad(offset, cellIndices, cellId, 0, 4, 7, 3, outputIndices);
      cell2quad(offset, cellIndices, cellId, 0, 3, 2, 1, outputIndices);
      cell2quad(offset, cellIndices, cellId, 4, 5, 6, 7, outputIndices);
    }

    template <typename VecType, typename OutputPortal>
    VISKORES_EXEC void operator()(const viskores::Id& pointOffset,
                                  viskores::CellShapeTagGeneric shapeType,
                                  const VecType& cellIndices,
                                  const viskores::Id& cellId,
                                  OutputPortal& outputIndices) const
    {

      if (shapeType.Id == viskores::CELL_SHAPE_QUAD)
      {
        viskores::Vec<viskores::Id, 5> quad;
        quad[0] = cellId;
        quad[1] = cellIndices[0];
        quad[2] = cellIndices[1];
        quad[3] = cellIndices[2];
        quad[4] = cellIndices[3];
        outputIndices.Set(pointOffset, quad);
      }
      if (shapeType.Id == viskores::CELL_SHAPE_HEXAHEDRON)
      {
        viskores::Id offset = pointOffset;
        cell2quad(offset, cellIndices, cellId, 0, 1, 5, 4, outputIndices);
        cell2quad(offset, cellIndices, cellId, 1, 2, 6, 5, outputIndices);
        cell2quad(offset, cellIndices, cellId, 3, 7, 6, 2, outputIndices);
        cell2quad(offset, cellIndices, cellId, 0, 4, 7, 3, outputIndices);
        cell2quad(offset, cellIndices, cellId, 0, 3, 2, 1, outputIndices);
        cell2quad(offset, cellIndices, cellId, 4, 5, 6, 7, outputIndices);
      }
      if (shapeType.Id == viskores::CELL_SHAPE_WEDGE)
      {
        viskores::Id offset = pointOffset;

        cell2quad(offset, cellIndices, cellId, 3, 0, 2, 5, outputIndices);
        cell2quad(offset, cellIndices, cellId, 1, 4, 5, 2, outputIndices);
        cell2quad(offset, cellIndices, cellId, 0, 3, 4, 1, outputIndices);
      }
      if (shapeType.Id == viskores::CELL_SHAPE_PYRAMID)
      {
        viskores::Id offset = pointOffset;

        cell2quad(offset, cellIndices, cellId, 3, 2, 1, 0, outputIndices);
      }
    }

  }; //class Quadralize

public:
  VISKORES_CONT
  Quadralizer() {}

  VISKORES_CONT
  void Run(const viskores::cont::UnknownCellSet& cellset,
           viskores::cont::ArrayHandle<viskores::Vec<viskores::Id, 5>>& outputIndices,
           viskores::Id& output)
  {
    viskores::cont::Invoker invoke;

    if (cellset.CanConvert<viskores::cont::CellSetStructured<3>>())
    {
      viskores::cont::CellSetStructured<3> cellSetStructured3D =
        cellset.AsCellSet<viskores::cont::CellSetStructured<3>>();
      const viskores::Id numCells = cellSetStructured3D.GetNumberOfCells();

      viskores::cont::ArrayHandleIndex cellIdxs(numCells);
      outputIndices.Allocate(numCells * QUAD_PER_CSS);
      invoke(SegmentedStructured<3>{}, cellSetStructured3D, cellIdxs, outputIndices);

      output = numCells * QUAD_PER_CSS;
    }
    else if (cellset.CanConvert<viskores::cont::CellSetStructured<2>>())
    {
      viskores::cont::CellSetStructured<2> cellSetStructured2D =
        cellset.AsCellSet<viskores::cont::CellSetStructured<2>>();
      const viskores::Id numCells = cellSetStructured2D.GetNumberOfCells();

      viskores::cont::ArrayHandleIndex cellIdxs(numCells);
      outputIndices.Allocate(numCells);
      invoke(SegmentedStructured<2>{}, cellSetStructured2D, cellIdxs, outputIndices);

      output = numCells;
    }
    else
    {
      auto cellSetUnstructured =
        cellset.ResetCellSetList(VISKORES_DEFAULT_CELL_SET_LIST_UNSTRUCTURED{});
      viskores::cont::ArrayHandle<viskores::Id> quadsPerCell;
      invoke(CountQuads{}, cellSetUnstructured, quadsPerCell);

      viskores::Id total = 0;
      total = viskores::cont::Algorithm::Reduce(quadsPerCell, viskores::Id(0));

      viskores::cont::ArrayHandle<viskores::Id> cellOffsets;
      viskores::cont::Algorithm::ScanExclusive(quadsPerCell, cellOffsets);
      outputIndices.Allocate(total);

      invoke(Quadralize{}, cellSetUnstructured, cellOffsets, outputIndices);

      output = total;
    }
  }
};
}
}
#endif
