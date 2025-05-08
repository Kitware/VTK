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
#ifndef viskores_rendering_Triangulator_h
#define viskores_rendering_Triangulator_h

#include <typeinfo>
#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/CellSetPermutation.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/UncertainCellSet.h>
#include <viskores/rendering/raytracing/MeshConnectivityBuilder.h>
#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/DispatcherMapTopology.h>
#include <viskores/worklet/ScatterUniform.h>
#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/WorkletMapTopology.h>

namespace viskores
{
namespace rendering
{
/// \brief Triangulator creates a minimal set of triangles from a cell set.
///
///  This class creates a array of triangle indices from both 3D and 2D
///  explicit cell sets. This list can serve as input to opengl and the
///  ray tracer scene renderers.
///
class Triangulator
{
public:
  struct InterleaveArrays12 : viskores::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn, FieldOut);
    using ExecutionSignature = void(_1, _2);
    using InputDomain = _1;
    using ScatterType = viskores::worklet::ScatterUniform<12>;
    template <typename T>
    VISKORES_EXEC void operator()(const T& input, T& output) const
    {
      if (int(input) == 0)
        output = 1;
    }
  };

  struct InterleaveArrays2 : viskores::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn, FieldOut);
    using ExecutionSignature = void(_1, _2);
    using InputDomain = _1;
    using ScatterType = viskores::worklet::ScatterUniform<2>;
    template <typename T>
    VISKORES_EXEC void operator()(const T& input, T& output) const
    {
      if (int(input) == 0)
        output = 1;
    }
  };

  class CountTriangles : public viskores::worklet::WorkletVisitCellsWithPoints
  {
  public:
    VISKORES_CONT
    CountTriangles() {}
    using ControlSignature = void(CellSetIn cellset, FieldInCell ghostField, FieldOut triangles);
    using ExecutionSignature = void(CellShape, _2, _3);

    template <typename ghostlArrayType>
    VISKORES_EXEC void operator()(viskores::CellShapeTagGeneric shapeType,
                                  ghostlArrayType& ghostField,
                                  viskores::Id& triangles) const
    {

      if (int(ghostField) != 0)
        triangles = 0;
      else if (shapeType.Id == viskores::CELL_SHAPE_TRIANGLE)
        triangles = 1;
      else if (shapeType.Id == viskores::CELL_SHAPE_QUAD)
        triangles = 2;
      else if (shapeType.Id == viskores::CELL_SHAPE_TETRA)
        triangles = 4;
      else if (shapeType.Id == viskores::CELL_SHAPE_HEXAHEDRON)
        triangles = 12;
      else if (shapeType.Id == viskores::CELL_SHAPE_WEDGE)
        triangles = 8;
      else if (shapeType.Id == viskores::CELL_SHAPE_PYRAMID)
        triangles = 6;
      else
        triangles = 0;
    }

    template <typename ghostlArrayType>
    VISKORES_EXEC void operator()(viskores::CellShapeTagHexahedron viskoresNotUsed(shapeType),
                                  ghostlArrayType& ghostField,
                                  viskores::Id& triangles) const
    {

      if (int(ghostField) != 0)
        triangles = 0;
      else
        triangles = 12;
    }

    template <typename ghostlArrayType>
    VISKORES_EXEC void operator()(viskores::CellShapeTagQuad viskoresNotUsed(shapeType),
                                  ghostlArrayType& ghostField,
                                  viskores::Id& triangles) const
    {
      if (int(ghostField) != 0)
        triangles = 0;
      else
        triangles = 2;
    }

    template <typename ghostlArrayType>
    VISKORES_EXEC void operator()(viskores::CellShapeTagWedge viskoresNotUsed(shapeType),
                                  ghostlArrayType& ghostField,
                                  viskores::Id& triangles) const
    {
      if (int(ghostField) != 0)
        triangles = 0;
      else
        triangles = 8;
    }
  }; //class CountTriangles

  template <int DIM>
  class TriangulateStructured : public viskores::worklet::WorkletVisitCellsWithPoints
  {

  public:
    using ControlSignature = void(CellSetIn cellset, FieldInCell, WholeArrayOut);
    using ExecutionSignature = void(IncidentElementIndices, _2, _3);
    VISKORES_CONT
    TriangulateStructured() {}

#if defined(VISKORES_MSVC)
#pragma warning(push)
#pragma warning(disable : 4127) //conditional expression is constant
#endif
    template <typename CellNodeVecType, typename OutIndicesPortal>
    VISKORES_EXEC void operator()(const CellNodeVecType& cellIndices,
                                  const viskores::Id& cellIndex,
                                  OutIndicesPortal& outputIndices) const
    {
      viskores::Id4 triangle;
      if (DIM == 2)
      {
        const viskores::Id triangleOffset = cellIndex * 2;
        // 0-1-2
        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[1];
        triangle[3] = cellIndices[2];
        triangle[0] = cellIndex;
        outputIndices.Set(triangleOffset, triangle);
        // 0-3-2
        triangle[2] = cellIndices[3];
        outputIndices.Set(triangleOffset + 1, triangle);
      }
      else if (DIM == 3)
      {
        const viskores::Id triangleOffset = cellIndex * 12;

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[1];
        triangle[3] = cellIndices[5];
        triangle[0] = cellIndex;
        outputIndices.Set(triangleOffset, triangle);

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[5];
        triangle[3] = cellIndices[4];
        outputIndices.Set(triangleOffset + 1, triangle);

        triangle[1] = cellIndices[1];
        triangle[2] = cellIndices[2];
        triangle[3] = cellIndices[6];
        outputIndices.Set(triangleOffset + 2, triangle);

        triangle[1] = cellIndices[1];
        triangle[2] = cellIndices[6];
        triangle[3] = cellIndices[5];
        outputIndices.Set(triangleOffset + 3, triangle);

        triangle[1] = cellIndices[3];
        triangle[2] = cellIndices[7];
        triangle[3] = cellIndices[6];
        outputIndices.Set(triangleOffset + 4, triangle);

        triangle[1] = cellIndices[3];
        triangle[2] = cellIndices[6];
        triangle[3] = cellIndices[2];
        outputIndices.Set(triangleOffset + 5, triangle);

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[4];
        triangle[3] = cellIndices[7];
        outputIndices.Set(triangleOffset + 6, triangle);

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[7];
        triangle[3] = cellIndices[3];
        outputIndices.Set(triangleOffset + 7, triangle);

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[3];
        triangle[3] = cellIndices[2];
        outputIndices.Set(triangleOffset + 8, triangle);

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[2];
        triangle[3] = cellIndices[1];
        outputIndices.Set(triangleOffset + 9, triangle);

        triangle[1] = cellIndices[4];
        triangle[2] = cellIndices[5];
        triangle[3] = cellIndices[6];
        outputIndices.Set(triangleOffset + 10, triangle);

        triangle[1] = cellIndices[4];
        triangle[2] = cellIndices[6];
        triangle[3] = cellIndices[7];
        outputIndices.Set(triangleOffset + 11, triangle);
      }
    }
#if defined(VISKORES_MSVC)
#pragma warning(pop)
#endif
  };


  class IndicesSort : public viskores::worklet::WorkletMapField
  {
  public:
    VISKORES_CONT
    IndicesSort() {}
    using ControlSignature = void(FieldInOut);
    using ExecutionSignature = void(_1);
    VISKORES_EXEC
    void operator()(viskores::Id4& triangleIndices) const
    {
      // first field contains the id of the cell the
      // trianlge belongs to
      viskores::Id temp;
      if (triangleIndices[1] > triangleIndices[3])
      {
        temp = triangleIndices[1];
        triangleIndices[1] = triangleIndices[3];
        triangleIndices[3] = temp;
      }
      if (triangleIndices[1] > triangleIndices[2])
      {
        temp = triangleIndices[1];
        triangleIndices[1] = triangleIndices[2];
        triangleIndices[2] = temp;
      }
      if (triangleIndices[2] > triangleIndices[3])
      {
        temp = triangleIndices[2];
        triangleIndices[2] = triangleIndices[3];
        triangleIndices[3] = temp;
      }
    }
  }; //class IndicesSort

  struct IndicesLessThan
  {
    VISKORES_EXEC_CONT
    bool operator()(const viskores::Id4& a, const viskores::Id4& b) const
    {
      if (a[1] < b[1])
        return true;
      if (a[1] > b[1])
        return false;
      if (a[2] < b[2])
        return true;
      if (a[2] > b[2])
        return false;
      if (a[3] < b[3])
        return true;
      return false;
    }
  };

  class UniqueTriangles : public viskores::worklet::WorkletMapField
  {
  public:
    VISKORES_CONT
    UniqueTriangles() {}

    using ControlSignature = void(WholeArrayIn, WholeArrayOut);
    using ExecutionSignature = void(_1, _2, WorkIndex);

    VISKORES_EXEC
    bool IsTwin(const viskores::Id4& a, const viskores::Id4& b) const
    {
      return (a[1] == b[1] && a[2] == b[2] && a[3] == b[3]);
    }

    template <typename IndicesPortalType, typename OutputFlagsPortalType>
    VISKORES_EXEC void operator()(const IndicesPortalType& indices,
                                  OutputFlagsPortalType& outputFlags,
                                  const viskores::Id& index) const
    {
      if (index == 0)
        return;
      //if we are a shared face, mark ourself and neighbor for destruction
      if (IsTwin(indices.Get(index), indices.Get(index - 1)))
      {
        outputFlags.Set(index, 0);
        outputFlags.Set(index - 1, 0);
      }
    }
  }; //class UniqueTriangles

  class Triangulate : public viskores::worklet::WorkletVisitCellsWithPoints
  {

  public:
    VISKORES_CONT
    Triangulate() {}
    using ControlSignature = void(CellSetIn cellset, FieldInCell, WholeArrayOut);
    using ExecutionSignature = void(_2, CellShape, PointIndices, WorkIndex, _3);

    template <typename VecType, typename OutputPortal>
    VISKORES_EXEC void operator()(const viskores::Id& triangleOffset,
                                  viskores::CellShapeTagWedge viskoresNotUsed(shapeType),
                                  const VecType& cellIndices,
                                  const viskores::Id& cellId,
                                  OutputPortal& outputIndices) const
    {
      viskores::Id4 triangle;

      triangle[1] = cellIndices[0];
      triangle[2] = cellIndices[1];
      triangle[3] = cellIndices[2];
      triangle[0] = cellId;
      outputIndices.Set(triangleOffset, triangle);

      triangle[1] = cellIndices[3];
      triangle[2] = cellIndices[5];
      triangle[3] = cellIndices[4];
      outputIndices.Set(triangleOffset + 1, triangle);

      triangle[1] = cellIndices[3];
      triangle[2] = cellIndices[0];
      triangle[3] = cellIndices[2];
      outputIndices.Set(triangleOffset + 2, triangle);

      triangle[1] = cellIndices[3];
      triangle[2] = cellIndices[2];
      triangle[3] = cellIndices[5];
      outputIndices.Set(triangleOffset + 3, triangle);

      triangle[1] = cellIndices[1];
      triangle[2] = cellIndices[4];
      triangle[3] = cellIndices[5];
      outputIndices.Set(triangleOffset + 4, triangle);

      triangle[1] = cellIndices[1];
      triangle[2] = cellIndices[5];
      triangle[3] = cellIndices[2];
      outputIndices.Set(triangleOffset + 5, triangle);

      triangle[1] = cellIndices[0];
      triangle[2] = cellIndices[3];
      triangle[3] = cellIndices[4];
      outputIndices.Set(triangleOffset + 6, triangle);

      triangle[1] = cellIndices[0];
      triangle[2] = cellIndices[4];
      triangle[3] = cellIndices[1];
      outputIndices.Set(triangleOffset + 7, triangle);
    }
    template <typename VecType, typename OutputPortal>
    VISKORES_EXEC void operator()(const viskores::Id& triangleOffset,
                                  viskores::CellShapeTagQuad viskoresNotUsed(shapeType),
                                  const VecType& cellIndices,
                                  const viskores::Id& cellId,
                                  OutputPortal& outputIndices) const
    {
      viskores::Id4 triangle;


      triangle[1] = cellIndices[0];
      triangle[2] = cellIndices[1];
      triangle[3] = cellIndices[2];
      triangle[0] = cellId;
      outputIndices.Set(triangleOffset, triangle);

      triangle[2] = cellIndices[3];
      outputIndices.Set(triangleOffset + 1, triangle);
    }

    template <typename VecType, typename OutputPortal>
    VISKORES_EXEC void operator()(const viskores::Id& triangleOffset,
                                  viskores::CellShapeTagHexahedron viskoresNotUsed(shapeType),
                                  const VecType& cellIndices,
                                  const viskores::Id& cellId,
                                  OutputPortal& outputIndices) const
    {
      viskores::Id4 triangle;

      triangle[1] = cellIndices[0];
      triangle[2] = cellIndices[1];
      triangle[3] = cellIndices[5];
      triangle[0] = cellId;
      outputIndices.Set(triangleOffset, triangle);

      triangle[1] = cellIndices[0];
      triangle[2] = cellIndices[5];
      triangle[3] = cellIndices[4];
      outputIndices.Set(triangleOffset + 1, triangle);

      triangle[1] = cellIndices[1];
      triangle[2] = cellIndices[2];
      triangle[3] = cellIndices[6];
      outputIndices.Set(triangleOffset + 2, triangle);

      triangle[1] = cellIndices[1];
      triangle[2] = cellIndices[6];
      triangle[3] = cellIndices[5];
      outputIndices.Set(triangleOffset + 3, triangle);

      triangle[1] = cellIndices[3];
      triangle[2] = cellIndices[7];
      triangle[3] = cellIndices[6];
      outputIndices.Set(triangleOffset + 4, triangle);

      triangle[1] = cellIndices[3];
      triangle[2] = cellIndices[6];
      triangle[3] = cellIndices[2];
      outputIndices.Set(triangleOffset + 5, triangle);

      triangle[1] = cellIndices[0];
      triangle[2] = cellIndices[4];
      triangle[3] = cellIndices[7];
      outputIndices.Set(triangleOffset + 6, triangle);

      triangle[1] = cellIndices[0];
      triangle[2] = cellIndices[7];
      triangle[3] = cellIndices[3];
      outputIndices.Set(triangleOffset + 7, triangle);

      triangle[1] = cellIndices[0];
      triangle[2] = cellIndices[3];
      triangle[3] = cellIndices[2];
      outputIndices.Set(triangleOffset + 8, triangle);

      triangle[1] = cellIndices[0];
      triangle[2] = cellIndices[2];
      triangle[3] = cellIndices[1];
      outputIndices.Set(triangleOffset + 9, triangle);

      triangle[1] = cellIndices[4];
      triangle[2] = cellIndices[5];
      triangle[3] = cellIndices[6];
      outputIndices.Set(triangleOffset + 10, triangle);

      triangle[1] = cellIndices[4];
      triangle[2] = cellIndices[6];
      triangle[3] = cellIndices[7];
      outputIndices.Set(triangleOffset + 11, triangle);
    }

    template <typename VecType, typename OutputPortal>
    VISKORES_EXEC void operator()(const viskores::Id& triangleOffset,
                                  viskores::CellShapeTagGeneric shapeType,
                                  const VecType& cellIndices,
                                  const viskores::Id& cellId,
                                  OutputPortal& outputIndices) const
    {
      viskores::Id4 triangle;

      if (shapeType.Id == viskores::CELL_SHAPE_TRIANGLE)
      {

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[1];
        triangle[3] = cellIndices[2];
        triangle[0] = cellId;
        outputIndices.Set(triangleOffset, triangle);
      }
      if (shapeType.Id == viskores::CELL_SHAPE_QUAD)
      {

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[1];
        triangle[3] = cellIndices[2];
        triangle[0] = cellId;
        outputIndices.Set(triangleOffset, triangle);

        triangle[2] = cellIndices[3];
        outputIndices.Set(triangleOffset + 1, triangle);
      }
      if (shapeType.Id == viskores::CELL_SHAPE_TETRA)
      {
        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[3];
        triangle[3] = cellIndices[1];
        triangle[0] = cellId;
        outputIndices.Set(triangleOffset, triangle);

        triangle[1] = cellIndices[1];
        triangle[2] = cellIndices[2];
        triangle[3] = cellIndices[3];
        outputIndices.Set(triangleOffset + 1, triangle);

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[2];
        triangle[3] = cellIndices[3];
        outputIndices.Set(triangleOffset + 2, triangle);

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[2];
        triangle[3] = cellIndices[1];
        outputIndices.Set(triangleOffset + 3, triangle);
      }
      if (shapeType.Id == viskores::CELL_SHAPE_HEXAHEDRON)
      {
        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[1];
        triangle[3] = cellIndices[5];
        triangle[0] = cellId;
        outputIndices.Set(triangleOffset, triangle);

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[5];
        triangle[3] = cellIndices[4];
        outputIndices.Set(triangleOffset + 1, triangle);

        triangle[1] = cellIndices[1];
        triangle[2] = cellIndices[2];
        triangle[3] = cellIndices[6];
        outputIndices.Set(triangleOffset + 2, triangle);

        triangle[1] = cellIndices[1];
        triangle[2] = cellIndices[6];
        triangle[3] = cellIndices[5];
        outputIndices.Set(triangleOffset + 3, triangle);

        triangle[1] = cellIndices[3];
        triangle[2] = cellIndices[7];
        triangle[3] = cellIndices[6];
        outputIndices.Set(triangleOffset + 4, triangle);

        triangle[1] = cellIndices[3];
        triangle[2] = cellIndices[6];
        triangle[3] = cellIndices[2];
        outputIndices.Set(triangleOffset + 5, triangle);

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[4];
        triangle[3] = cellIndices[7];
        outputIndices.Set(triangleOffset + 6, triangle);

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[7];
        triangle[3] = cellIndices[3];
        outputIndices.Set(triangleOffset + 7, triangle);

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[3];
        triangle[3] = cellIndices[2];
        outputIndices.Set(triangleOffset + 8, triangle);

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[2];
        triangle[3] = cellIndices[1];
        outputIndices.Set(triangleOffset + 9, triangle);

        triangle[1] = cellIndices[4];
        triangle[2] = cellIndices[5];
        triangle[3] = cellIndices[6];
        outputIndices.Set(triangleOffset + 10, triangle);

        triangle[1] = cellIndices[4];
        triangle[2] = cellIndices[6];
        triangle[3] = cellIndices[7];
        outputIndices.Set(triangleOffset + 11, triangle);
      }
      if (shapeType.Id == viskores::CELL_SHAPE_WEDGE)
      {
        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[1];
        triangle[3] = cellIndices[2];
        triangle[0] = cellId;
        outputIndices.Set(triangleOffset, triangle);

        triangle[1] = cellIndices[3];
        triangle[2] = cellIndices[5];
        triangle[3] = cellIndices[4];
        outputIndices.Set(triangleOffset + 1, triangle);

        triangle[1] = cellIndices[3];
        triangle[2] = cellIndices[0];
        triangle[3] = cellIndices[2];
        outputIndices.Set(triangleOffset + 2, triangle);

        triangle[1] = cellIndices[3];
        triangle[2] = cellIndices[2];
        triangle[3] = cellIndices[5];
        outputIndices.Set(triangleOffset + 3, triangle);

        triangle[1] = cellIndices[1];
        triangle[2] = cellIndices[4];
        triangle[3] = cellIndices[5];
        outputIndices.Set(triangleOffset + 4, triangle);

        triangle[1] = cellIndices[1];
        triangle[2] = cellIndices[5];
        triangle[3] = cellIndices[2];
        outputIndices.Set(triangleOffset + 5, triangle);

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[3];
        triangle[3] = cellIndices[4];
        outputIndices.Set(triangleOffset + 6, triangle);

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[4];
        triangle[3] = cellIndices[1];
        outputIndices.Set(triangleOffset + 7, triangle);
      }
      if (shapeType.Id == viskores::CELL_SHAPE_PYRAMID)
      {
        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[4];
        triangle[3] = cellIndices[1];
        triangle[0] = cellId;
        outputIndices.Set(triangleOffset, triangle);

        triangle[1] = cellIndices[1];
        triangle[2] = cellIndices[2];
        triangle[3] = cellIndices[4];
        outputIndices.Set(triangleOffset + 1, triangle);

        triangle[1] = cellIndices[2];
        triangle[2] = cellIndices[3];
        triangle[3] = cellIndices[4];
        outputIndices.Set(triangleOffset + 2, triangle);

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[4];
        triangle[3] = cellIndices[3];
        outputIndices.Set(triangleOffset + 3, triangle);

        triangle[1] = cellIndices[3];
        triangle[2] = cellIndices[2];
        triangle[3] = cellIndices[1];
        outputIndices.Set(triangleOffset + 4, triangle);

        triangle[1] = cellIndices[3];
        triangle[2] = cellIndices[1];
        triangle[3] = cellIndices[0];
        outputIndices.Set(triangleOffset + 5, triangle);
      }
    }
  }; //class Triangulate

public:
  VISKORES_CONT
  Triangulator() {}

  VISKORES_CONT
  void ExternalTriangles(viskores::cont::ArrayHandle<viskores::Id4>& outputIndices,
                         viskores::Id& outputTriangles)
  {
    //Eliminate unseen triangles
    viskores::worklet::DispatcherMapField<IndicesSort> sortInvoker;
    sortInvoker.Invoke(outputIndices);

    viskores::cont::Algorithm::Sort(outputIndices, IndicesLessThan());
    viskores::cont::ArrayHandle<viskores::UInt8> flags;
    flags.Allocate(outputTriangles);

    viskores::cont::ArrayHandleConstant<viskores::Id> one(1, outputTriangles);
    viskores::cont::Algorithm::Copy(one, flags);
    //Unique triangles will have a flag = 1
    viskores::worklet::DispatcherMapField<UniqueTriangles>().Invoke(outputIndices, flags);

    viskores::cont::ArrayHandle<viskores::Id4> subset;
    viskores::cont::Algorithm::CopyIf(outputIndices, flags, subset);
    outputIndices = subset;
    outputTriangles = subset.GetNumberOfValues();
  }

  VISKORES_CONT
  void Run(const viskores::cont::UnknownCellSet& cellset,
           viskores::cont::ArrayHandle<viskores::Id4>& outputIndices,
           viskores::Id& outputTriangles,
           const viskores::cont::Field& ghostField = viskores::cont::Field())
  {
    bool fastPath = false;
    if (cellset.CanConvert<viskores::cont::CellSetStructured<3>>())
    {
      //viskores::cont::CellSetStructured<3> cellSetStructured3D =
      //  cellset.Cast<viskores::cont::CellSetStructured<3>>();

      //raytracing::MeshConnectivityBuilder<Device> builder;
      //outputIndices = builder.ExternalTrianglesStructured(cellSetStructured3D);
      //outputTriangles = outputIndices.GetNumberOfValues();
      //fastPath = true;
      viskores::cont::CellSetStructured<3> cellSetStructured3D =
        cellset.AsCellSet<viskores::cont::CellSetStructured<3>>();
      const viskores::Id numCells = cellSetStructured3D.GetNumberOfCells();

      viskores::cont::ArrayHandleCounting<viskores::Id> cellIdxs(0, 1, numCells);
      outputIndices.Allocate(numCells * 12);
      viskores::worklet::DispatcherMapTopology<TriangulateStructured<3>>(TriangulateStructured<3>())
        .Invoke(cellSetStructured3D, cellIdxs, outputIndices);

      outputTriangles = numCells * 12;

      // removed blanked triangles
      viskores::cont::ArrayHandle<viskores::UInt8> triangleGhostArrayHandle;
      triangleGhostArrayHandle.AllocateAndFill(outputTriangles, 0); //numCells * 12
      viskores::worklet::DispatcherMapField<InterleaveArrays12>(InterleaveArrays12())
        .Invoke(ghostField.GetData().ExtractComponent<viskores::UInt8>(0),
                triangleGhostArrayHandle);

      viskores::cont::ArrayHandle<viskores::Id4> nonGhostTriangles;
      viskores::cont::Algorithm::CopyIf(outputIndices, triangleGhostArrayHandle, nonGhostTriangles);
      outputTriangles = nonGhostTriangles.GetNumberOfValues();
      outputIndices = nonGhostTriangles;
    }
    else if (cellset.CanConvert<viskores::cont::CellSetStructured<2>>())
    {
      viskores::cont::CellSetStructured<2> cellSetStructured2D =
        cellset.AsCellSet<viskores::cont::CellSetStructured<2>>();
      const viskores::Id numCells = cellSetStructured2D.GetNumberOfCells();

      viskores::cont::ArrayHandleCounting<viskores::Id> cellIdxs(0, 1, numCells);
      outputIndices.Allocate(numCells * 2);
      viskores::worklet::DispatcherMapTopology<TriangulateStructured<2>>(TriangulateStructured<2>())
        .Invoke(cellSetStructured2D, cellIdxs, outputIndices);

      outputTriangles = numCells * 2;

      // removed blanked triangles
      viskores::cont::ArrayHandle<viskores::UInt8> triangleGhostArrayHandle;
      triangleGhostArrayHandle.AllocateAndFill(outputTriangles, 0); //numCells * 2
      viskores::worklet::DispatcherMapField<InterleaveArrays2>(InterleaveArrays2())
        .Invoke(ghostField.GetData().ExtractComponent<viskores::UInt8>(0),
                triangleGhostArrayHandle);

      viskores::cont::ArrayHandle<viskores::Id4> nonGhostTriangles;
      viskores::cont::Algorithm::CopyIf(outputIndices, triangleGhostArrayHandle, nonGhostTriangles);
      outputTriangles = nonGhostTriangles.GetNumberOfValues();
      outputIndices = nonGhostTriangles;

      // no need to do external faces on 2D cell set
      fastPath = true;
    }
    else
    {
      auto cellSetUnstructured =
        cellset.ResetCellSetList(VISKORES_DEFAULT_CELL_SET_LIST_UNSTRUCTURED{});
      viskores::cont::ArrayHandle<viskores::Id> trianglesPerCell;

      viskores::worklet::DispatcherMapTopology<CountTriangles>(CountTriangles())
        .Invoke(cellSetUnstructured,
                ghostField.GetData().ExtractComponent<viskores::UInt8>(0),
                trianglesPerCell);

      viskores::Id totalTriangles = 0;
      totalTriangles = viskores::cont::Algorithm::Reduce(trianglesPerCell, viskores::Id(0));

      viskores::cont::ArrayHandle<viskores::Id> cellOffsets;
      viskores::cont::Algorithm::ScanExclusive(trianglesPerCell, cellOffsets);
      outputIndices.Allocate(totalTriangles);

      viskores::worklet::DispatcherMapTopology<Triangulate>(Triangulate())
        .Invoke(cellSetUnstructured, cellOffsets, outputIndices);

      outputTriangles = totalTriangles;
    }

    //get rid of any triagles we cannot see
    if (!fastPath)
    {
      ExternalTriangles(outputIndices, outputTriangles);
    }
  }
}; // class Triangulator
}
} //namespace viskores::rendering
#endif //viskores_rendering_Triangulator_h
