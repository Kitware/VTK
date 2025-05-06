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

#include <viskores/rendering/raytracing/CylinderExtractor.h>

#include <viskores/cont/Algorithm.h>
#include <viskores/rendering/Cylinderizer.h>
#include <viskores/rendering/raytracing/Worklets.h>
#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/DispatcherMapTopology.h>

namespace viskores
{
namespace rendering
{
namespace raytracing
{

namespace detail
{

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
    else
      segments = 0;
  }

  VISKORES_EXEC
  void operator()(viskores::CellShapeTagHexahedron viskoresNotUsed(shapeType),
                  viskores::Id& points) const
  {
    points = 0;
  }

  VISKORES_EXEC
  void operator()(viskores::CellShapeTagQuad viskoresNotUsed(shapeType), viskores::Id& points) const
  {
    points = 0;
  }
  VISKORES_EXEC
  void operator()(viskores::CellShapeTagWedge viskoresNotUsed(shapeType),
                  viskores::Id& points) const
  {
    points = 0;
  }

}; // ClassCountSegments

class Pointify : public viskores::worklet::WorkletVisitCellsWithPoints
{

public:
  VISKORES_CONT
  Pointify() {}
  typedef void ControlSignature(CellSetIn cellset, FieldInCell, WholeArrayOut);
  typedef void ExecutionSignature(_2, CellShape, PointIndices, WorkIndex, _3);

  template <typename VecType, typename OutputPortal>
  VISKORES_EXEC void operator()(const viskores::Id& viskoresNotUsed(pointOffset),
                                viskores::CellShapeTagQuad viskoresNotUsed(shapeType),
                                const VecType& viskoresNotUsed(cellIndices),
                                const viskores::Id& viskoresNotUsed(cellId),
                                OutputPortal& viskoresNotUsed(outputIndices)) const
  {
  }

  template <typename VecType, typename OutputPortal>
  VISKORES_EXEC void operator()(const viskores::Id& viskoresNotUsed(pointOffset),
                                viskores::CellShapeTagWedge viskoresNotUsed(shapeType),
                                const VecType& viskoresNotUsed(cellIndices),
                                const viskores::Id& viskoresNotUsed(cellId),
                                OutputPortal& viskoresNotUsed(outputIndices)) const
  {
  }

  template <typename VecType, typename OutputPortal>
  VISKORES_EXEC void operator()(const viskores::Id& viskoresNotUsed(pointOffset),
                                viskores::CellShapeTagHexahedron viskoresNotUsed(shapeType),
                                const VecType& viskoresNotUsed(cellIndices),
                                const viskores::Id& viskoresNotUsed(cellId),
                                OutputPortal& viskoresNotUsed(outputIndices)) const

  {
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
    else if (shapeType.Id == viskores::CELL_SHAPE_TRIANGLE)
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
    else if (shapeType.Id == viskores::CELL_SHAPE_QUAD)
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
  }
}; //class pointify

class Iterator : public viskores::worklet::WorkletMapField
{

public:
  VISKORES_CONT
  Iterator() {}
  typedef void ControlSignature(FieldOut);
  typedef void ExecutionSignature(_1, WorkIndex);
  VISKORES_EXEC
  void operator()(viskores::Id2& index, const viskores::Id2& idx) const { index = idx; }
}; //class Iterator

class FieldRadius : public viskores::worklet::WorkletMapField
{
protected:
  viskores::Float32 MinRadius;
  viskores::Float32 RadiusDelta;
  viskores::Float32 MinValue;
  viskores::Float32 InverseDelta;

public:
  VISKORES_CONT
  FieldRadius(const viskores::Float32 minRadius,
              const viskores::Float32 maxRadius,
              const viskores::Range scalarRange)
    : MinRadius(minRadius)
    , RadiusDelta(maxRadius - minRadius)
    , MinValue(static_cast<viskores::Float32>(scalarRange.Min))
  {
    viskores::Float32 delta = static_cast<viskores::Float32>(scalarRange.Max - scalarRange.Min);
    if (delta != 0.f)
      InverseDelta = 1.f / (delta);
    else
      InverseDelta = 0.f; // just map scalar to 0;
  }

  typedef void ControlSignature(FieldIn, FieldOut, WholeArrayIn);
  typedef void ExecutionSignature(_1, _2, _3);

  template <typename ScalarPortalType>
  VISKORES_EXEC void operator()(const viskores::Id3& cylId,
                                viskores::Float32& radius,
                                const ScalarPortalType& scalars) const
  {
    viskores::Float32 scalar = static_cast<viskores::Float32>(scalars.Get(cylId[0]));
    viskores::Float32 t = (scalar - MinValue) * InverseDelta;
    radius = MinRadius + t * RadiusDelta;
  }

}; //class FieldRadius

} //namespace detail


void CylinderExtractor::ExtractCells(const viskores::cont::UnknownCellSet& cells,
                                     const viskores::Float32 radius)
{
  viskores::Id numOfSegments;
  viskores::rendering::Cylinderizer geometrizer;
  geometrizer.Run(cells, this->CylIds, numOfSegments);

  this->SetUniformRadius(radius);
}

void CylinderExtractor::ExtractCells(const viskores::cont::UnknownCellSet& cells,
                                     const viskores::cont::Field& field,
                                     const viskores::Float32 minRadius,
                                     const viskores::Float32 maxRadius)
{
  viskores::Id numOfSegments;
  viskores::rendering::Cylinderizer geometrizer;
  geometrizer.Run(cells, this->CylIds, numOfSegments);

  this->SetVaryingRadius(minRadius, maxRadius, field);
}

void CylinderExtractor::SetUniformRadius(const viskores::Float32 radius)
{
  const viskores::Id size = this->CylIds.GetNumberOfValues();
  Radii.Allocate(size);

  viskores::cont::ArrayHandleConstant<viskores::Float32> radiusHandle(radius, size);
  viskores::cont::Algorithm::Copy(radiusHandle, Radii);
}

void CylinderExtractor::SetCylinderIdsFromCells(const viskores::cont::UnknownCellSet& cells)
{
  viskores::Id numCells = cells.GetNumberOfCells();
  if (numCells == 0)
  {
    return;
  }
  //
  // look for points in the cell set
  //
  if (cells.CanConvert<viskores::cont::CellSetExplicit<>>())
  {
    auto cellsExplicit = cells.AsCellSet<viskores::cont::CellSetExplicit<>>();

    viskores::cont::ArrayHandle<viskores::Id> points;
    viskores::worklet::DispatcherMapTopology<detail::CountSegments>(detail::CountSegments())
      .Invoke(cellsExplicit, points);

    viskores::Id totalPoints = 0;
    totalPoints = viskores::cont::Algorithm::Reduce(points, viskores::Id(0));

    viskores::cont::ArrayHandle<viskores::Id> cellOffsets;
    viskores::cont::Algorithm::ScanExclusive(points, cellOffsets);
    CylIds.Allocate(totalPoints);

    viskores::worklet::DispatcherMapTopology<detail::Pointify>(detail::Pointify())
      .Invoke(cellsExplicit, cellOffsets, this->CylIds);
  }
}

void CylinderExtractor::SetVaryingRadius(const viskores::Float32 minRadius,
                                         const viskores::Float32 maxRadius,
                                         const viskores::cont::Field& field)
{
  viskores::cont::ArrayHandle<viskores::Range> rangeArray = field.GetRange();
  if (rangeArray.GetNumberOfValues() != 1)
  {
    throw viskores::cont::ErrorBadValue("Cylinder Extractor: scalar field must have one component");
  }

  viskores::Range range = rangeArray.ReadPortal().Get(0);

  Radii.Allocate(this->CylIds.GetNumberOfValues());
  viskores::worklet::DispatcherMapField<detail::FieldRadius>(
    detail::FieldRadius(minRadius, maxRadius, range))
    .Invoke(this->CylIds, this->Radii, viskores::rendering::raytracing::GetScalarFieldArray(field));
}


viskores::cont::ArrayHandle<viskores::Id3> CylinderExtractor::GetCylIds()
{
  return this->CylIds;
}

viskores::cont::ArrayHandle<viskores::Float32> CylinderExtractor::GetRadii()
{
  return this->Radii;
}

viskores::Id CylinderExtractor::GetNumberOfCylinders() const
{
  return this->CylIds.GetNumberOfValues();
}
}
}
} //namespace viskores::rendering::raytracing
