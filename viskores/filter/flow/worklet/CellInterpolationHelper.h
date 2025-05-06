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

#ifndef viskores_filter_flow_worklet_CellInterpolationHelper_h
#define viskores_filter_flow_worklet_CellInterpolationHelper_h

#include <viskores/CellShape.h>
#include <viskores/Types.h>
#include <viskores/VecVariable.h>

#include <viskores/cont/ArrayGetValues.h>
#include <viskores/cont/CellSetStructured.h>
#include <viskores/cont/ExecutionObjectBase.h>
#include <viskores/cont/UnknownCellSet.h>
#include <viskores/exec/CellInterpolate.h>

/*
 * Interface to define the helper classes that can return mesh data
 * on a cell by cell basis.
 */
namespace viskores
{
namespace exec
{

class CellInterpolationHelper
{
private:
  using ShapeType = viskores::cont::ArrayHandle<viskores::UInt8>;
  using OffsetType = viskores::cont::ArrayHandle<viskores::Id>;
  using ConnType = viskores::cont::ArrayHandle<viskores::Id>;
  using ShapePortalType = typename ShapeType::ReadPortalType;
  using OffsetPortalType = typename OffsetType::ReadPortalType;
  using ConnPortalType = typename ConnType::ReadPortalType;

public:
  enum class HelperType
  {
    STRUCTURED,
    EXPSINGLE,
    EXPLICIT
  };

  VISKORES_CONT
  CellInterpolationHelper() = default;

  VISKORES_CONT
  CellInterpolationHelper(const viskores::Id3& cellDims, const viskores::Id3& pointDims, bool is3D)
    : CellDims(cellDims)
    , PointDims(pointDims)
    , Is3D(is3D)
  {
    this->Type = HelperType::STRUCTURED;
  }

  CellInterpolationHelper(const viskores::UInt8 cellShape,
                          const viskores::IdComponent pointsPerCell,
                          const ConnType& connectivity,
                          viskores::cont::DeviceAdapterId device,
                          viskores::cont::Token& token)
    : CellShape(cellShape)
    , PointsPerCell(pointsPerCell)
    , Connectivity(connectivity.PrepareForInput(device, token))
  {
    this->Type = HelperType::EXPSINGLE;
  }


  VISKORES_CONT
  CellInterpolationHelper(const ShapeType& shape,
                          const OffsetType& offset,
                          const ConnType& connectivity,
                          viskores::cont::DeviceAdapterId device,
                          viskores::cont::Token& token)
    : Shape(shape.PrepareForInput(device, token))
    , Offset(offset.PrepareForInput(device, token))
    , Connectivity(connectivity.PrepareForInput(device, token))
  {
    this->Type = HelperType::EXPLICIT;
  }

  VISKORES_EXEC
  void GetCellInfo(const viskores::Id& cellId,
                   viskores::UInt8& cellShape,
                   viskores::IdComponent& numVerts,
                   viskores::VecVariable<viskores::Id, 8>& indices) const
  {
    switch (this->Type)
    {
      case HelperType::STRUCTURED:
      {
        viskores::Id3 logicalCellId;
        logicalCellId[0] = cellId % this->CellDims[0];
        logicalCellId[1] = (cellId / this->CellDims[0]) % this->CellDims[1];
        if (this->Is3D)
        {
          logicalCellId[2] = cellId / (this->CellDims[0] * this->CellDims[1]);
          indices.Append((logicalCellId[2] * this->PointDims[1] + logicalCellId[1]) *
                           this->PointDims[0] +
                         logicalCellId[0]);
          indices.Append(indices[0] + 1);
          indices.Append(indices[1] + this->PointDims[0]);
          indices.Append(indices[2] - 1);
          indices.Append(indices[0] + this->PointDims[0] * this->PointDims[1]);
          indices.Append(indices[4] + 1);
          indices.Append(indices[5] + this->PointDims[0]);
          indices.Append(indices[6] - 1);
          cellShape = static_cast<viskores::UInt8>(viskores::CELL_SHAPE_HEXAHEDRON);
          numVerts = 8;
        }
        else
        {
          indices.Append(logicalCellId[1] * this->PointDims[0] + logicalCellId[0]);
          indices.Append(indices[0] + 1);
          indices.Append(indices[1] + this->PointDims[0]);
          indices.Append(indices[2] - 1);
          cellShape = static_cast<viskores::UInt8>(viskores::CELL_SHAPE_QUAD);
          numVerts = 4;
        }
      }
      break;

      case HelperType::EXPSINGLE:
      {
        cellShape = this->CellShape;
        numVerts = this->PointsPerCell;
        viskores::Id n = static_cast<viskores::Id>(PointsPerCell);
        viskores::Id offset = cellId * n;
        for (viskores::Id i = 0; i < n; i++)
          indices.Append(Connectivity.Get(offset + i));
      }
      break;

      case HelperType::EXPLICIT:
      {
        cellShape = this->Shape.Get(cellId);
        const viskores::Id offset = this->Offset.Get(cellId);
        numVerts = static_cast<viskores::IdComponent>(this->Offset.Get(cellId + 1) - offset);
        for (viskores::IdComponent i = 0; i < numVerts; i++)
          indices.Append(this->Connectivity.Get(offset + i));
      }
      break;

      default:
      {
        // Code path not expected to execute in correct cases
        // Supress unused variable warning
        cellShape = viskores::UInt8(0);
        numVerts = viskores::IdComponent(0);
      }
    }
  }

private:
  HelperType Type;
  // variables for structured type
  viskores::Id3 CellDims;
  viskores::Id3 PointDims;
  bool Is3D = true;
  // variables for single explicit type
  viskores::UInt8 CellShape;
  viskores::IdComponent PointsPerCell;
  // variables for explicit type
  ShapePortalType Shape;
  OffsetPortalType Offset;
  ConnPortalType Connectivity;
};

} // namespace exec

/*
 * Control side base object.
 */
namespace cont
{

class CellInterpolationHelper : public viskores::cont::ExecutionObjectBase
{
private:
  using ExecutionType = viskores::exec::CellInterpolationHelper;
  using Structured2DType = viskores::cont::CellSetStructured<2>;
  using Structured3DType = viskores::cont::CellSetStructured<3>;
  using SingleExplicitType = viskores::cont::CellSetSingleType<>;
  using ExplicitType = viskores::cont::CellSetExplicit<>;

public:
  VISKORES_CONT
  CellInterpolationHelper() = default;

  VISKORES_CONT
  CellInterpolationHelper(const viskores::cont::UnknownCellSet& cellSet)
  {
    if (cellSet.CanConvert<Structured2DType>())
    {
      this->Is3D = false;
      viskores::Id2 cellDims = cellSet.AsCellSet<Structured2DType>().GetSchedulingRange(
        viskores::TopologyElementTagCell());
      viskores::Id2 pointDims = cellSet.AsCellSet<Structured2DType>().GetSchedulingRange(
        viskores::TopologyElementTagPoint());
      this->CellDims = viskores::Id3(cellDims[0], cellDims[1], 0);
      this->PointDims = viskores::Id3(pointDims[0], pointDims[1], 1);
      this->Type = viskores::exec::CellInterpolationHelper::HelperType::STRUCTURED;
    }
    else if (cellSet.CanConvert<Structured3DType>())
    {
      this->Is3D = true;
      this->CellDims = cellSet.AsCellSet<Structured3DType>().GetSchedulingRange(
        viskores::TopologyElementTagCell());
      this->PointDims = cellSet.AsCellSet<Structured3DType>().GetSchedulingRange(
        viskores::TopologyElementTagPoint());
      this->Type = viskores::exec::CellInterpolationHelper::HelperType::STRUCTURED;
    }
    else if (cellSet.CanConvert<SingleExplicitType>())
    {
      SingleExplicitType CellSet = cellSet.AsCellSet<SingleExplicitType>();
      const auto cellShapes = CellSet.GetShapesArray(viskores::TopologyElementTagCell(),
                                                     viskores::TopologyElementTagPoint());
      const auto numIndices = CellSet.GetNumIndicesArray(viskores::TopologyElementTagCell(),
                                                         viskores::TopologyElementTagPoint());
      CellShape = viskores::cont::ArrayGetValue(0, cellShapes);
      PointsPerCell = viskores::cont::ArrayGetValue(0, numIndices);
      Connectivity = CellSet.GetConnectivityArray(viskores::TopologyElementTagCell(),
                                                  viskores::TopologyElementTagPoint());
      this->Type = viskores::exec::CellInterpolationHelper::HelperType::EXPSINGLE;
    }
    else if (cellSet.CanConvert<ExplicitType>())
    {
      viskores::cont::CellSetExplicit<> CellSet =
        cellSet.AsCellSet<viskores::cont::CellSetExplicit<>>();
      Shape = CellSet.GetShapesArray(viskores::TopologyElementTagCell(),
                                     viskores::TopologyElementTagPoint());
      Offset = CellSet.GetOffsetsArray(viskores::TopologyElementTagCell(),
                                       viskores::TopologyElementTagPoint());
      Connectivity = CellSet.GetConnectivityArray(viskores::TopologyElementTagCell(),
                                                  viskores::TopologyElementTagPoint());
      this->Type = viskores::exec::CellInterpolationHelper::HelperType::EXPLICIT;
    }
    else
      throw viskores::cont::ErrorInternal("Unsupported cellset type");
  }

  VISKORES_CONT
  const ExecutionType PrepareForExecution(viskores::cont::DeviceAdapterId device,
                                          viskores::cont::Token& token) const
  {
    switch (this->Type)
    {
      case ExecutionType::HelperType::STRUCTURED:
        return ExecutionType(this->CellDims, this->PointDims, this->Is3D);

      case ExecutionType::HelperType::EXPSINGLE:
        return ExecutionType(
          this->CellShape, this->PointsPerCell, this->Connectivity, device, token);

      case ExecutionType::HelperType::EXPLICIT:
        return ExecutionType(this->Shape, this->Offset, this->Connectivity, device, token);
    }
    throw viskores::cont::ErrorInternal("Undefined case for building cell interpolation helper");
  }

private:
  // Variables required for strucutred grids
  viskores::Id3 CellDims;
  viskores::Id3 PointDims;
  bool Is3D = true;
  // variables for single explicit type
  viskores::UInt8 CellShape;
  viskores::IdComponent PointsPerCell;
  // Variables required for unstructured grids
  viskores::cont::ArrayHandle<viskores::UInt8> Shape;
  viskores::cont::ArrayHandle<viskores::Id> Offset;
  viskores::cont::ArrayHandle<viskores::Id> Connectivity;
  ExecutionType::HelperType Type;
};

} //namespace cont
} //namespace viskores

#endif //viskores_filter_flow_worklet_CellInterpolationHelper_h
