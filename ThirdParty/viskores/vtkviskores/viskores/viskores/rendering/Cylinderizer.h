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
#ifndef viskores_rendering_Cylinderizer_h
#define viskores_rendering_Cylinderizer_h

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

#define SEG_PER_TRI 3
//CSS is CellSetStructured
#define TRI_PER_CSS 12

namespace viskores
{
namespace rendering
{

class Cylinderizer
{
public:
  class CountSegments : public viskores::worklet::WorkletVisitCellsWithPoints
  {
  public:
    VISKORES_CONT
    CountSegments() {}
    typedef void ControlSignature(CellSetIn cellset, FieldOut);
    typedef void ExecutionSignature(CellShape, _2);

    VISKORES_EXEC
    void operator()(viskores::CellShapeTagGeneric shapeType, viskores::Id& segments) const
    {
      if (shapeType.Id == viskores::CELL_SHAPE_LINE)
        segments = 1;
      else if (shapeType.Id == viskores::CELL_SHAPE_TRIANGLE)
        segments = 3;
      else if (shapeType.Id == viskores::CELL_SHAPE_QUAD)
        segments = 4;
      else if (shapeType.Id == viskores::CELL_SHAPE_TETRA)
        segments = 12;
      else if (shapeType.Id == viskores::CELL_SHAPE_WEDGE)
        segments = 24;
      else if (shapeType.Id == viskores::CELL_SHAPE_PYRAMID)
        segments = 18;
      else if (shapeType.Id == viskores::CELL_SHAPE_HEXAHEDRON)
        segments = 36;
      else
        segments = 0;
    }

    VISKORES_EXEC
    void operator()(viskores::CellShapeTagHexahedron viskoresNotUsed(shapeType),
                    viskores::Id& segments) const
    {
      segments = 36;
    }

    VISKORES_EXEC
    void operator()(viskores::CellShapeTagQuad viskoresNotUsed(shapeType),
                    viskores::Id& segments) const
    {
      segments = 4;
    }
    VISKORES_EXEC
    void operator()(viskores::CellShapeTagWedge viskoresNotUsed(shapeType),
                    viskores::Id& segments) const
    {
      segments = 24;
    }
  }; //class CountSegments

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
    VISKORES_EXEC void cell2seg(viskores::Id3 idx,
                                viskores::Vec<Id, 3>& segment,
                                const viskores::Id offset,
                                const CellNodeVecType& cellIndices,
                                OutIndicesPortal& outputIndices) const
    {

      segment[1] = cellIndices[viskores::IdComponent(idx[0])];
      segment[2] = cellIndices[viskores::IdComponent(idx[1])];
      outputIndices.Set(offset, segment);

      segment[1] = cellIndices[viskores::IdComponent(idx[1])];
      segment[2] = cellIndices[viskores::IdComponent(idx[2])];
      outputIndices.Set(offset + 1, segment);

      segment[1] = cellIndices[viskores::IdComponent(idx[2])];
      segment[2] = cellIndices[viskores::IdComponent(idx[0])];
      outputIndices.Set(offset + 2, segment);
    }
    template <typename CellNodeVecType, typename OutIndicesPortal>
    VISKORES_EXEC void operator()(const CellNodeVecType& cellIndices,
                                  const viskores::Id& cellIndex,
                                  OutIndicesPortal& outputIndices) const
    {
      if (DIM == 2)
      {
        // Do nothing mark says
      }
      else if (DIM == 3)
      {
        viskores::Id offset = cellIndex * TRI_PER_CSS * SEG_PER_TRI;
        viskores::Id3 segment;
        segment[0] = cellIndex;
        viskores::Id3 idx;
        idx[0] = 0;
        idx[1] = 1;
        idx[2] = 5;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
        idx[0] = 0;
        idx[1] = 5;
        idx[2] = 4;
        offset += 3;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
        idx[0] = 1;
        idx[1] = 2;
        idx[2] = 6;
        offset += 3;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
        idx[0] = 1;
        idx[1] = 6;
        idx[2] = 5;
        offset += 3;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
        idx[0] = 3;
        idx[1] = 7;
        idx[2] = 6;
        offset += 3;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
        idx[0] = 3;
        idx[1] = 6;
        idx[2] = 2;
        offset += 3;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
        idx[0] = 0;
        idx[1] = 4;
        idx[2] = 7;
        offset += 3;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
        idx[0] = 0;
        idx[1] = 7;
        idx[2] = 3;
        offset += 3;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
        idx[0] = 0;
        idx[1] = 3;
        idx[2] = 2;
        offset += 3;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
        idx[0] = 0;
        idx[1] = 2;
        idx[2] = 1;
        offset += 3;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
        idx[0] = 4;
        idx[1] = 5;
        idx[2] = 6;
        offset += 3;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
        idx[0] = 4;
        idx[1] = 6;
        idx[2] = 7;
        offset += 3;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
      }
    }
#if defined(VISKORES_MSVC)
#pragma warning(pop)
#endif
  };


  class Cylinderize : public viskores::worklet::WorkletVisitCellsWithPoints
  {

  public:
    VISKORES_CONT
    Cylinderize() {}
    typedef void ControlSignature(CellSetIn cellset, FieldInCell, WholeArrayOut);
    typedef void ExecutionSignature(_2, CellShape, PointIndices, WorkIndex, _3);

    template <typename VecType, typename OutputPortal>
    VISKORES_EXEC void tri2seg(viskores::Id& offset,
                               const VecType& cellIndices,
                               const viskores::Id& cellId,
                               const viskores::Id Id0,
                               const viskores::Id Id1,
                               const viskores::Id Id2,
                               OutputPortal& outputIndices) const
    {
      viskores::Id3 segment;
      segment[0] = cellId;
      segment[1] = viskores::Id(cellIndices[viskores::IdComponent(Id0)]);
      segment[2] = viskores::Id(cellIndices[viskores::IdComponent(Id1)]);
      outputIndices.Set(offset++, segment);

      segment[1] = viskores::Id(cellIndices[viskores::IdComponent(Id1)]);
      segment[2] = viskores::Id(cellIndices[viskores::IdComponent(Id2)]);
      outputIndices.Set(offset++, segment);

      segment[1] = viskores::Id(cellIndices[viskores::IdComponent(Id2)]);
      segment[2] = viskores::Id(cellIndices[viskores::IdComponent(Id0)]);
      outputIndices.Set(offset++, segment);
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
        viskores::Id3 segment;
        segment[0] = cellId;
        segment[1] = cellIndices[0];
        segment[2] = cellIndices[1];
        outputIndices.Set(offset, segment);

        segment[1] = cellIndices[1];
        segment[2] = cellIndices[2];
        outputIndices.Set(offset + 1, segment);

        segment[1] = cellIndices[2];
        segment[2] = cellIndices[3];
        outputIndices.Set(offset + 2, segment);

        segment[1] = cellIndices[3];
        segment[2] = cellIndices[0];
        outputIndices.Set(offset + 3, segment);
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
      tri2seg(offset, cellIndices, cellId, 0, 1, 5, outputIndices);
      tri2seg(offset, cellIndices, cellId, 0, 5, 4, outputIndices);
      tri2seg(offset, cellIndices, cellId, 1, 2, 6, outputIndices);
      tri2seg(offset, cellIndices, cellId, 1, 6, 5, outputIndices);
      tri2seg(offset, cellIndices, cellId, 3, 7, 6, outputIndices);
      tri2seg(offset, cellIndices, cellId, 3, 6, 2, outputIndices);
      tri2seg(offset, cellIndices, cellId, 0, 4, 7, outputIndices);
      tri2seg(offset, cellIndices, cellId, 0, 7, 3, outputIndices);
      tri2seg(offset, cellIndices, cellId, 0, 3, 2, outputIndices);
      tri2seg(offset, cellIndices, cellId, 0, 2, 1, outputIndices);
      tri2seg(offset, cellIndices, cellId, 4, 5, 6, outputIndices);
      tri2seg(offset, cellIndices, cellId, 4, 6, 7, outputIndices);
    }
    template <typename VecType, typename OutputPortal>
    VISKORES_EXEC void operator()(const viskores::Id& pointOffset,
                                  viskores::CellShapeTagWedge viskoresNotUsed(shapeType),
                                  const VecType& cellIndices,
                                  const viskores::Id& cellId,
                                  OutputPortal& outputIndices) const

    {
      viskores::Id offset = pointOffset;
      tri2seg(offset, cellIndices, cellId, 0, 1, 2, outputIndices);
      tri2seg(offset, cellIndices, cellId, 3, 5, 4, outputIndices);
      tri2seg(offset, cellIndices, cellId, 3, 0, 2, outputIndices);
      tri2seg(offset, cellIndices, cellId, 3, 2, 5, outputIndices);
      tri2seg(offset, cellIndices, cellId, 1, 4, 5, outputIndices);
      tri2seg(offset, cellIndices, cellId, 1, 5, 2, outputIndices);
      tri2seg(offset, cellIndices, cellId, 0, 3, 4, outputIndices);
      tri2seg(offset, cellIndices, cellId, 0, 4, 1, outputIndices);
    }
    template <typename VecType, typename OutputPortal>
    VISKORES_EXEC void operator()(const viskores::Id& pointOffset,
                                  viskores::CellShapeTagGeneric shapeType,
                                  const VecType& cellIndices,
                                  const viskores::Id& cellId,
                                  OutputPortal& outputIndices) const
    {

      if (shapeType.Id == viskores::CELL_SHAPE_LINE)
      {
        viskores::Id3 segment;
        segment[0] = cellId;

        segment[1] = cellIndices[0];
        segment[2] = cellIndices[1];
        outputIndices.Set(pointOffset, segment);
      }
      if (shapeType.Id == viskores::CELL_SHAPE_TRIANGLE)
      {
        viskores::Id3 segment;
        segment[0] = cellId;
        segment[1] = cellIndices[0];
        segment[2] = cellIndices[1];
        outputIndices.Set(pointOffset, segment);

        segment[1] = cellIndices[1];
        segment[2] = cellIndices[2];
        outputIndices.Set(pointOffset + 1, segment);

        segment[1] = cellIndices[2];
        segment[2] = cellIndices[0];
        outputIndices.Set(pointOffset + 2, segment);
      }
      if (shapeType.Id == viskores::CELL_SHAPE_QUAD)
      {
        viskores::Id3 segment;
        segment[0] = cellId;
        segment[1] = cellIndices[0];
        segment[2] = cellIndices[1];
        outputIndices.Set(pointOffset, segment);

        segment[1] = cellIndices[1];
        segment[2] = cellIndices[2];
        outputIndices.Set(pointOffset + 1, segment);

        segment[1] = cellIndices[2];
        segment[2] = cellIndices[3];
        outputIndices.Set(pointOffset + 2, segment);

        segment[1] = cellIndices[3];
        segment[2] = cellIndices[0];
        outputIndices.Set(pointOffset + 3, segment);
      }
      if (shapeType.Id == viskores::CELL_SHAPE_TETRA)
      {
        viskores::Id offset = pointOffset;
        tri2seg(offset, cellIndices, cellId, 0, 3, 1, outputIndices);
        tri2seg(offset, cellIndices, cellId, 1, 2, 3, outputIndices);
        tri2seg(offset, cellIndices, cellId, 0, 2, 3, outputIndices);
        tri2seg(offset, cellIndices, cellId, 0, 2, 1, outputIndices);
      }
      if (shapeType.Id == viskores::CELL_SHAPE_HEXAHEDRON)
      {
        viskores::Id offset = pointOffset;
        tri2seg(offset, cellIndices, cellId, 0, 1, 5, outputIndices);
        tri2seg(offset, cellIndices, cellId, 0, 5, 4, outputIndices);
        tri2seg(offset, cellIndices, cellId, 1, 2, 6, outputIndices);
        tri2seg(offset, cellIndices, cellId, 1, 6, 5, outputIndices);
        tri2seg(offset, cellIndices, cellId, 3, 7, 6, outputIndices);
        tri2seg(offset, cellIndices, cellId, 3, 6, 2, outputIndices);
        tri2seg(offset, cellIndices, cellId, 0, 4, 7, outputIndices);
        tri2seg(offset, cellIndices, cellId, 0, 7, 3, outputIndices);
        tri2seg(offset, cellIndices, cellId, 0, 3, 2, outputIndices);
        tri2seg(offset, cellIndices, cellId, 0, 2, 1, outputIndices);
        tri2seg(offset, cellIndices, cellId, 4, 5, 6, outputIndices);
        tri2seg(offset, cellIndices, cellId, 4, 6, 7, outputIndices);
      }
      if (shapeType.Id == viskores::CELL_SHAPE_WEDGE)
      {
        viskores::Id offset = pointOffset;
        tri2seg(offset, cellIndices, cellId, 0, 1, 2, outputIndices);
        tri2seg(offset, cellIndices, cellId, 3, 5, 4, outputIndices);
        tri2seg(offset, cellIndices, cellId, 3, 0, 2, outputIndices);
        tri2seg(offset, cellIndices, cellId, 3, 2, 5, outputIndices);
        tri2seg(offset, cellIndices, cellId, 1, 4, 5, outputIndices);
        tri2seg(offset, cellIndices, cellId, 1, 5, 2, outputIndices);
        tri2seg(offset, cellIndices, cellId, 0, 3, 4, outputIndices);
        tri2seg(offset, cellIndices, cellId, 0, 4, 1, outputIndices);
      }
      if (shapeType.Id == viskores::CELL_SHAPE_PYRAMID)
      {
        viskores::Id offset = pointOffset;

        tri2seg(offset, cellIndices, cellId, 0, 4, 1, outputIndices);
        tri2seg(offset, cellIndices, cellId, 1, 2, 4, outputIndices);
        tri2seg(offset, cellIndices, cellId, 2, 3, 4, outputIndices);
        tri2seg(offset, cellIndices, cellId, 0, 4, 3, outputIndices);
        tri2seg(offset, cellIndices, cellId, 3, 2, 1, outputIndices);
        tri2seg(offset, cellIndices, cellId, 3, 1, 0, outputIndices);
      }
    }

  }; //class cylinderize

public:
  VISKORES_CONT
  Cylinderizer() {}

  VISKORES_CONT
  void Run(const viskores::cont::UnknownCellSet& cellset,
           viskores::cont::ArrayHandle<viskores::Id3>& outputIndices,
           viskores::Id& output)
  {
    if (cellset.CanConvert<viskores::cont::CellSetStructured<3>>())
    {
      viskores::cont::CellSetStructured<3> cellSetStructured3D =
        cellset.AsCellSet<viskores::cont::CellSetStructured<3>>();
      const viskores::Id numCells = cellSetStructured3D.GetNumberOfCells();

      viskores::cont::ArrayHandleCounting<viskores::Id> cellIdxs(0, 1, numCells);
      outputIndices.Allocate(numCells * TRI_PER_CSS * SEG_PER_TRI);

      viskores::worklet::DispatcherMapTopology<SegmentedStructured<3>> segInvoker;
      segInvoker.Invoke(cellSetStructured3D, cellIdxs, outputIndices);

      output = numCells * TRI_PER_CSS * SEG_PER_TRI;
    }
    else
    {
      auto cellSetUnstructured =
        cellset.ResetCellSetList(VISKORES_DEFAULT_CELL_SET_LIST_UNSTRUCTURED{});

      viskores::cont::ArrayHandle<viskores::Id> segmentsPerCell;
      viskores::worklet::DispatcherMapTopology<CountSegments> countInvoker;
      countInvoker.Invoke(cellSetUnstructured, segmentsPerCell);

      viskores::Id total = 0;
      total = viskores::cont::Algorithm::Reduce(segmentsPerCell, viskores::Id(0));

      viskores::cont::ArrayHandle<viskores::Id> cellOffsets;
      viskores::cont::Algorithm::ScanExclusive(segmentsPerCell, cellOffsets);
      outputIndices.Allocate(total);

      viskores::worklet::DispatcherMapTopology<Cylinderize> cylInvoker;
      cylInvoker.Invoke(cellSetUnstructured, cellOffsets, outputIndices);

      output = total;
    }
  }
};
}
}
#endif
