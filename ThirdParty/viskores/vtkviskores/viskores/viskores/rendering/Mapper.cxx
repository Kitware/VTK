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

#include <viskores/cont/BoundsCompute.h>
#include <viskores/rendering/Mapper.h>

namespace viskores
{
namespace rendering
{

Mapper::~Mapper() {}

void Mapper::RenderCells(const viskores::cont::UnknownCellSet& cellset,
                         const viskores::cont::CoordinateSystem& coords,
                         const viskores::cont::Field& scalarField,
                         const viskores::cont::ColorTable& colorTable,
                         const viskores::rendering::Camera& camera,
                         const viskores::Range& scalarRange)
{
  RenderCellsImpl(cellset,
                  coords,
                  scalarField,
                  colorTable,
                  camera,
                  scalarRange,
                  make_FieldCell(viskores::cont::GetGlobalGhostCellFieldName(),
                                 viskores::cont::ArrayHandleConstant<viskores::UInt8>(
                                   0, cellset.GetNumberOfCells())));
};

void Mapper::RenderCells(const viskores::cont::UnknownCellSet& cellset,
                         const viskores::cont::CoordinateSystem& coords,
                         const viskores::cont::Field& scalarField,
                         const viskores::cont::ColorTable& colorTable,
                         const viskores::rendering::Camera& camera,
                         const viskores::Range& scalarRange,
                         const viskores::cont::Field& ghostField)
{
  RenderCellsImpl(cellset, coords, scalarField, colorTable, camera, scalarRange, ghostField);
};

struct CompareIndices
{
  viskores::Vec3f CameraDirection;
  std::vector<viskores::Vec3f>& Centers;
  bool SortBackToFront;
  CompareIndices(std::vector<viskores::Vec3f>& centers,
                 viskores::Vec3f cameraDirection,
                 bool sortBackToFront)
    : CameraDirection(cameraDirection)
    , Centers(centers)
    , SortBackToFront(sortBackToFront)
  {
  }

  bool operator()(viskores::Id i, viskores::Id j) const
  {
    if (this->SortBackToFront)
    {
      return (viskores::Dot(this->Centers[i], this->CameraDirection) >
              viskores::Dot(this->Centers[j], this->CameraDirection));
    }
    else
    {
      return (viskores::Dot(this->Centers[i], this->CameraDirection) <
              viskores::Dot(this->Centers[j], this->CameraDirection));
    }
  }
};

void Mapper::RenderCellsPartitioned(const viskores::cont::PartitionedDataSet partitionedData,
                                    const std::string fieldName,
                                    const viskores::cont::ColorTable& colorTable,
                                    const viskores::rendering::Camera& camera,
                                    const viskores::Range& scalarRange)
{
  // sort partitions back to front for best rendering with the volume renderer
  std::vector<viskores::Vec3f> centers(
    partitionedData.GetNumberOfPartitions()); // vector for centers
  std::vector<viskores::Id> indices(partitionedData.GetNumberOfPartitions());
  for (viskores::Id p = 0; p < partitionedData.GetNumberOfPartitions(); p++)
  {
    indices[static_cast<size_t>(p)] = p;
    centers[static_cast<size_t>(p)] =
      viskores::cont::BoundsCompute(partitionedData.GetPartition(p)).Center();
  }
  CompareIndices comparator(
    centers, camera.GetLookAt() - camera.GetPosition(), this->SortBackToFront);
  std::sort(indices.begin(), indices.end(), comparator);

  for (viskores::Id p = 0; p < partitionedData.GetNumberOfPartitions(); p++)
  {
    auto partition = partitionedData.GetPartition(indices[static_cast<size_t>(p)]);
    this->RenderCells(partition.GetCellSet(),
                      partition.GetCoordinateSystem(),
                      partition.GetField(fieldName),
                      colorTable,
                      camera,
                      scalarRange,
                      partition.GetGhostCellField());
  }
}

void Mapper::SetActiveColorTable(const viskores::cont::ColorTable& colorTable)
{

  constexpr viskores::Float32 conversionToFloatSpace = (1.0f / 255.0f);

  viskores::cont::ArrayHandle<viskores::Vec4ui_8> temp;

  {
    viskores::cont::ScopedRuntimeDeviceTracker tracker(viskores::cont::DeviceAdapterTagSerial{});
    colorTable.Sample(1024, temp);
  }

  this->ColorMap.Allocate(1024);
  auto portal = this->ColorMap.WritePortal();
  auto colorPortal = temp.ReadPortal();
  for (viskores::Id i = 0; i < 1024; ++i)
  {
    auto color = colorPortal.Get(i);
    viskores::Vec4f_32 t(color[0] * conversionToFloatSpace,
                         color[1] * conversionToFloatSpace,
                         color[2] * conversionToFloatSpace,
                         color[3] * conversionToFloatSpace);
    portal.Set(i, t);
  }
}

void Mapper::SetLogarithmX(bool l)
{
  this->LogarithmX = l;
}

void Mapper::SetLogarithmY(bool l)
{
  this->LogarithmY = l;
}
}
}
