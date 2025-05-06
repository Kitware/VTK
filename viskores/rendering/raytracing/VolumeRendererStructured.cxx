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
#include <viskores/rendering/raytracing/VolumeRendererStructured.h>

#include <cmath>
#include <iostream>
#include <viskores/cont/ArrayHandleCartesianProduct.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/ArrayHandleUniformPointCoordinates.h>
#include <viskores/cont/CellLocatorRectilinearGrid.h>
#include <viskores/cont/CellLocatorUniformGrid.h>
#include <viskores/cont/CellSetStructured.h>
#include <viskores/cont/ColorTable.h>
#include <viskores/cont/ErrorBadValue.h>
#include <viskores/cont/Invoker.h>
#include <viskores/cont/Timer.h>
#include <viskores/cont/TryExecute.h>
#include <viskores/rendering/raytracing/Logger.h>
#include <viskores/rendering/raytracing/Ray.h>
#include <viskores/rendering/raytracing/RayTracingTypeDefs.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace rendering
{
namespace raytracing
{
using DefaultHandle = viskores::cont::ArrayHandle<viskores::FloatDefault>;
using CartesianArrayHandle =
  viskores::cont::ArrayHandleCartesianProduct<DefaultHandle, DefaultHandle, DefaultHandle>;

namespace
{

template <typename Device, typename Derived>
class LocatorAdapterBase
{
private:
public:
  VISKORES_EXEC
  inline bool IsInside(const viskores::Vec3f_32& point) const
  {
    return static_cast<const Derived*>(this)->Locator.IsInside(point);
  }

  // Assumes point inside the data set
  VISKORES_EXEC
  inline void LocateCell(viskores::Id3& cell,
                         const viskores::Vec3f_32& point,
                         viskores::Vec3f_32& invSpacing,
                         viskores::Vec3f& parametric) const
  {
    viskores::Id cellId{};
    auto self = static_cast<const Derived*>(this);
    self->Locator.FindCell(point, cellId, parametric);
    cell = self->Conn.FlatToLogicalVisitIndex(cellId);
    self->ComputeInvSpacing(cell, point, invSpacing, parametric);
  }

  VISKORES_EXEC
  inline void GetCellIndices(const viskores::Id3& cell,
                             viskores::Vec<viskores::Id, 8>& cellIndices) const
  {
    cellIndices = static_cast<const Derived*>(this)->Conn.GetIndices(cell);
  }

  VISKORES_EXEC
  inline viskores::Id GetCellIndex(const viskores::Id3& cell) const
  {
    return static_cast<const Derived*>(this)->Conn.LogicalToFlatVisitIndex(cell);
  }

  VISKORES_EXEC
  inline void GetPoint(const viskores::Id& index, viskores::Vec3f_32& point) const
  {
    BOUNDS_CHECK(static_cast<const Derived*>(this)->Coordinates, index);
    point = static_cast<const Derived*>(this)->Coordinates.Get(index);
  }

  VISKORES_EXEC
  inline void GetMinPoint(const viskores::Id3& cell, viskores::Vec3f_32& point) const
  {
    const viskores::Id pointIndex =
      static_cast<const Derived*>(this)->Conn.LogicalToFlatIncidentIndex(cell);
    point = static_cast<const Derived*>(this)->Coordinates.Get(pointIndex);
  }
};

template <typename Device>
class RectilinearLocatorAdapter
  : public LocatorAdapterBase<Device, RectilinearLocatorAdapter<Device>>
{
private:
  friend LocatorAdapterBase<Device, RectilinearLocatorAdapter<Device>>;
  using DefaultConstHandle = typename DefaultHandle::ReadPortalType;
  using CartesianConstPortal = typename CartesianArrayHandle::ReadPortalType;

  CartesianConstPortal Coordinates;
  viskores::exec::
    ConnectivityStructured<viskores::TopologyElementTagCell, viskores::TopologyElementTagPoint, 3>
      Conn;
  viskores::exec::CellLocatorRectilinearGrid Locator;

  DefaultConstHandle CoordPortals[3];

  VISKORES_EXEC
  inline void ComputeInvSpacing(viskores::Id3& cell,
                                const viskores::Vec3f_32&,
                                viskores::Vec3f_32& invSpacing,
                                viskores::Vec3f) const
  {
    viskores::Vec3f p0{ CoordPortals[0].Get(cell[0]),
                        CoordPortals[1].Get(cell[1]),
                        CoordPortals[2].Get(cell[2]) };
    viskores::Vec3f p1{ CoordPortals[0].Get(cell[0] + 1),
                        CoordPortals[1].Get(cell[1] + 1),
                        CoordPortals[2].Get(cell[2] + 1) };
    invSpacing = 1.f / (p1 - p0);
  }

public:
  RectilinearLocatorAdapter(const CartesianArrayHandle& coordinates,
                            viskores::cont::CellSetStructured<3>& cellset,
                            viskores::cont::CellLocatorRectilinearGrid& locator,
                            viskores::cont::Token& token)
    : Coordinates(coordinates.PrepareForInput(Device(), token))
    , Conn(cellset.PrepareForInput(Device(),
                                   viskores::TopologyElementTagCell(),
                                   viskores::TopologyElementTagPoint(),
                                   token))
    , Locator((locator.PrepareForExecution(Device(), token)))
  {
    CoordPortals[0] = Coordinates.GetFirstPortal();
    CoordPortals[1] = Coordinates.GetSecondPortal();
    CoordPortals[2] = Coordinates.GetThirdPortal();
  }
}; // class RectilinearLocatorAdapter

template <typename Device>
class UniformLocatorAdapter : public LocatorAdapterBase<Device, UniformLocatorAdapter<Device>>
{
private:
  friend LocatorAdapterBase<Device, UniformLocatorAdapter<Device>>;
  using UniformArrayHandle = viskores::cont::ArrayHandleUniformPointCoordinates;
  using UniformConstPortal = typename UniformArrayHandle::ReadPortalType;

  UniformConstPortal Coordinates;
  viskores::exec::
    ConnectivityStructured<viskores::TopologyElementTagCell, viskores::TopologyElementTagPoint, 3>
      Conn;
  viskores::exec::CellLocatorUniformGrid Locator;

  viskores::Vec3f_32 InvSpacing{ 0, 0, 0 };

  VISKORES_EXEC
  inline void ComputeInvSpacing(viskores::Id3&,
                                const viskores::Vec3f_32&,
                                viskores::Vec3f_32& invSpacing,
                                viskores::Vec3f&) const
  {
    invSpacing = InvSpacing;
  }

public:
  UniformLocatorAdapter(const UniformArrayHandle& coordinates,
                        viskores::cont::CellSetStructured<3>& cellset,
                        viskores::cont::CellLocatorUniformGrid& locator,
                        viskores::cont::Token& token)
    : Coordinates(coordinates.PrepareForInput(Device(), token))
    , Conn(cellset.PrepareForInput(Device(),
                                   viskores::TopologyElementTagCell(),
                                   viskores::TopologyElementTagPoint(),
                                   token))
    , Locator(locator.PrepareForExecution(Device(), token))
  {
    viskores::Vec3f_32 spacing = Coordinates.GetSpacing();
    InvSpacing[0] = 1.f / spacing[0];
    InvSpacing[1] = 1.f / spacing[1];
    InvSpacing[2] = 1.f / spacing[2];
  }
}; // class UniformLocatorAdapter

} //namespace


template <typename DeviceAdapterTag, typename LocatorType>
class Sampler : public viskores::worklet::WorkletMapField
{
private:
  using ColorArrayHandle = typename viskores::cont::ArrayHandle<viskores::Vec4f_32>;
  using ColorArrayPortal = typename ColorArrayHandle::ReadPortalType;
  ColorArrayPortal ColorMap;
  viskores::Id ColorMapSize;
  viskores::Float32 MinScalar;
  viskores::Float32 SampleDistance;
  viskores::Float32 InverseDeltaScalar;
  LocatorType Locator;
  viskores::Float32 MeshEpsilon;

public:
  VISKORES_CONT
  Sampler(const ColorArrayHandle& colorMap,
          const viskores::Float32& minScalar,
          const viskores::Float32& maxScalar,
          const viskores::Float32& sampleDistance,
          const LocatorType& locator,
          const viskores::Float32& meshEpsilon,
          viskores::cont::Token& token)
    : ColorMap(colorMap.PrepareForInput(DeviceAdapterTag(), token))
    , MinScalar(minScalar)
    , SampleDistance(sampleDistance)
    , InverseDeltaScalar(minScalar)
    , Locator(locator)
    , MeshEpsilon(meshEpsilon)
  {
    ColorMapSize = colorMap.GetNumberOfValues() - 1;
    if ((maxScalar - minScalar) != 0.f)
    {
      InverseDeltaScalar = 1.f / (maxScalar - minScalar);
    }
  }

  using ControlSignature = void(FieldIn, FieldIn, FieldIn, FieldIn, WholeArrayInOut, WholeArrayIn);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, WorkIndex);

  template <typename ScalarPortalType, typename ColorBufferType>
  VISKORES_EXEC void operator()(const viskores::Vec3f_32& rayDir,
                                const viskores::Vec3f_32& rayOrigin,
                                const viskores::Float32& minDistance,
                                const viskores::Float32& maxDistance,
                                ColorBufferType& colorBuffer,
                                ScalarPortalType& scalars,
                                const viskores::Id& pixelIndex) const
  {
    viskores::Vec4f_32 color;
    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 0);
    color[0] = colorBuffer.Get(pixelIndex * 4 + 0);
    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 1);
    color[1] = colorBuffer.Get(pixelIndex * 4 + 1);
    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 2);
    color[2] = colorBuffer.Get(pixelIndex * 4 + 2);
    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 3);
    color[3] = colorBuffer.Get(pixelIndex * 4 + 3);

    if (minDistance == -1.f)
    {
      return; //TODO: Compact? or just image subset...
    }

    //get the initial sample position;
    viskores::Vec3f_32 sampleLocation;
    // find the distance to the first sample
    viskores::Float32 distance = minDistance + MeshEpsilon;
    sampleLocation = rayOrigin + distance * rayDir;
    // since the calculations are slightly different, we could hit an
    // edge case where the first sample location may not be in the data set.
    // Thus, advance to the next sample location
    while (!Locator.IsInside(sampleLocation) && distance < maxDistance)
    {
      distance += SampleDistance;
      sampleLocation = rayOrigin + distance * rayDir;
    }

    /*
            7----------6
           /|         /|
          4----------5 |
          | |        | |
          | 3--------|-2    z y
          |/         |/     |/
          0----------1      |__ x
    */
    bool newCell = true;
    viskores::Vec3f parametric{ -1.f, -1.f, -1.f };
    viskores::Vec3f_32 bottomLeft(0.f, 0.f, 0.f);

    viskores::Float32 scalar0 = 0.f;
    viskores::Float32 scalar1minus0 = 0.f;
    viskores::Float32 scalar2minus3 = 0.f;
    viskores::Float32 scalar3 = 0.f;
    viskores::Float32 scalar4 = 0.f;
    viskores::Float32 scalar5minus4 = 0.f;
    viskores::Float32 scalar6minus7 = 0.f;
    viskores::Float32 scalar7 = 0.f;

    viskores::Id3 cell(0, 0, 0);
    viskores::Vec3f_32 invSpacing(0.f, 0.f, 0.f);

    while (Locator.IsInside(sampleLocation) && distance < maxDistance)
    {
      viskores::Float32 mint =
        viskores::Min(parametric[0], viskores::Min(parametric[1], parametric[2]));
      viskores::Float32 maxt =
        viskores::Max(parametric[0], viskores::Max(parametric[1], parametric[2]));
      if (maxt > 1.f || mint < 0.f)
        newCell = true;
      if (newCell)
      {
        viskores::Vec<viskores::Id, 8> cellIndices;
        Locator.LocateCell(cell, sampleLocation, invSpacing, parametric);
        Locator.GetCellIndices(cell, cellIndices);
        Locator.GetPoint(cellIndices[0], bottomLeft);

        scalar0 = viskores::Float32(scalars.Get(cellIndices[0]));
        auto scalar1 = viskores::Float32(scalars.Get(cellIndices[1]));
        auto scalar2 = viskores::Float32(scalars.Get(cellIndices[2]));
        scalar3 = viskores::Float32(scalars.Get(cellIndices[3]));
        scalar4 = viskores::Float32(scalars.Get(cellIndices[4]));
        auto scalar5 = viskores::Float32(scalars.Get(cellIndices[5]));
        auto scalar6 = viskores::Float32(scalars.Get(cellIndices[6]));
        scalar7 = viskores::Float32(scalars.Get(cellIndices[7]));

        // save ourselves a couple extra instructions
        scalar6minus7 = scalar6 - scalar7;
        scalar5minus4 = scalar5 - scalar4;
        scalar1minus0 = scalar1 - scalar0;
        scalar2minus3 = scalar2 - scalar3;

        newCell = false;
      }

      viskores::Float32 lerped76 = scalar7 + parametric[0] * scalar6minus7;
      viskores::Float32 lerped45 = scalar4 + parametric[0] * scalar5minus4;
      viskores::Float32 lerpedTop = lerped45 + parametric[1] * (lerped76 - lerped45);

      viskores::Float32 lerped01 = scalar0 + parametric[0] * scalar1minus0;
      viskores::Float32 lerped32 = scalar3 + parametric[0] * scalar2minus3;
      viskores::Float32 lerpedBottom = lerped01 + parametric[1] * (lerped32 - lerped01);

      viskores::Float32 finalScalar = lerpedBottom + parametric[2] * (lerpedTop - lerpedBottom);

      //normalize scalar
      finalScalar = (finalScalar - MinScalar) * InverseDeltaScalar;

      auto colorIndex =
        static_cast<viskores::Id>(finalScalar * static_cast<viskores::Float32>(ColorMapSize));
      if (colorIndex < 0)
        colorIndex = 0;
      if (colorIndex > ColorMapSize)
        colorIndex = ColorMapSize;
      viskores::Vec4f_32 sampleColor = ColorMap.Get(colorIndex);

      //composite
      viskores::Float32 alpha = sampleColor[3] * (1.f - color[3]);
      color[0] = color[0] + sampleColor[0] * alpha;
      color[1] = color[1] + sampleColor[1] * alpha;
      color[2] = color[2] + sampleColor[2] * alpha;
      color[3] = alpha + color[3];

      // terminate the ray early if it became completely opaque.
      if (color[3] >= 1.f)
        break;

      //advance
      distance += SampleDistance;
      sampleLocation = sampleLocation + SampleDistance * rayDir;

      parametric = (sampleLocation - bottomLeft) * invSpacing;
    }

    color[0] = viskores::Min(color[0], 1.f);
    color[1] = viskores::Min(color[1], 1.f);
    color[2] = viskores::Min(color[2], 1.f);
    color[3] = viskores::Min(color[3], 1.f);

    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 0);
    colorBuffer.Set(pixelIndex * 4 + 0, color[0]);
    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 1);
    colorBuffer.Set(pixelIndex * 4 + 1, color[1]);
    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 2);
    colorBuffer.Set(pixelIndex * 4 + 2, color[2]);
    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 3);
    colorBuffer.Set(pixelIndex * 4 + 3, color[3]);
  }
}; //Sampler

template <typename DeviceAdapterTag, typename LocatorType>
class SamplerCellAssoc : public viskores::worklet::WorkletMapField
{
private:
  using ColorArrayHandle = typename viskores::cont::ArrayHandle<viskores::Vec4f_32>;
  using ColorArrayPortal = typename ColorArrayHandle::ReadPortalType;
  ColorArrayPortal ColorMap;
  viskores::Id ColorMapSize;
  viskores::Float32 MinScalar;
  viskores::Float32 SampleDistance;
  viskores::Float32 InverseDeltaScalar;
  LocatorType Locator;
  viskores::Float32 MeshEpsilon;

public:
  VISKORES_CONT
  SamplerCellAssoc(const ColorArrayHandle& colorMap,
                   const viskores::Float32& minScalar,
                   const viskores::Float32& maxScalar,
                   const viskores::Float32& sampleDistance,
                   const LocatorType& locator,
                   const viskores::Float32& meshEpsilon,
                   viskores::cont::Token& token)
    : ColorMap(colorMap.PrepareForInput(DeviceAdapterTag(), token))
    , MinScalar(minScalar)
    , SampleDistance(sampleDistance)
    , InverseDeltaScalar(minScalar)
    , Locator(locator)
    , MeshEpsilon(meshEpsilon)
  {
    ColorMapSize = colorMap.GetNumberOfValues() - 1;
    if ((maxScalar - minScalar) != 0.f)
    {
      InverseDeltaScalar = 1.f / (maxScalar - minScalar);
    }
  }
  using ControlSignature = void(FieldIn, FieldIn, FieldIn, FieldIn, WholeArrayInOut, WholeArrayIn);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, WorkIndex);

  template <typename ScalarPortalType, typename ColorBufferType>
  VISKORES_EXEC void operator()(const viskores::Vec3f_32& rayDir,
                                const viskores::Vec3f_32& rayOrigin,
                                const viskores::Float32& minDistance,
                                const viskores::Float32& maxDistance,
                                ColorBufferType& colorBuffer,
                                const ScalarPortalType& scalars,
                                const viskores::Id& pixelIndex) const
  {
    viskores::Vec4f_32 color;
    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 0);
    color[0] = colorBuffer.Get(pixelIndex * 4 + 0);
    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 1);
    color[1] = colorBuffer.Get(pixelIndex * 4 + 1);
    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 2);
    color[2] = colorBuffer.Get(pixelIndex * 4 + 2);
    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 3);
    color[3] = colorBuffer.Get(pixelIndex * 4 + 3);

    if (minDistance == -1.f)
    {
      return; //TODO: Compact? or just image subset...
    }

    //get the initial sample position;
    viskores::Vec3f_32 sampleLocation;
    // find the distance to the first sample
    viskores::Float32 distance = minDistance + MeshEpsilon;
    sampleLocation = rayOrigin + distance * rayDir;
    // since the calculations are slightly different, we could hit an
    // edge case where the first sample location may not be in the data set.
    // Thus, advance to the next sample location
    while (!Locator.IsInside(sampleLocation) && distance < maxDistance)
    {
      distance += SampleDistance;
      sampleLocation = rayOrigin + distance * rayDir;
    }

    /*
            7----------6
           /|         /|
          4----------5 |
          | |        | |
          | 3--------|-2    z y
          |/         |/     |/
          0----------1      |__ x
    */
    bool newCell = true;
    viskores::Vec3f parametric{ -1.f, -1.f, -1.f };
    viskores::Float32 scalar0 = 0.f;
    viskores::Vec4f_32 sampleColor(0.f, 0.f, 0.f, 0.f);
    viskores::Vec3f_32 bottomLeft(0.f, 0.f, 0.f);

    viskores::Id3 cell(0, 0, 0);
    viskores::Vec3f_32 invSpacing(0.f, 0.f, 0.f);

    while (Locator.IsInside(sampleLocation) && distance < maxDistance)
    {
      viskores::Float32 mint =
        viskores::Min(parametric[0], viskores::Min(parametric[1], parametric[2]));
      viskores::Float32 maxt =
        viskores::Max(parametric[0], viskores::Max(parametric[1], parametric[2]));
      if (maxt > 1.f || mint < 0.f)
        newCell = true;
      if (newCell)
      {
        Locator.LocateCell(cell, sampleLocation, invSpacing, parametric);
        viskores::Id cellId = Locator.GetCellIndex(cell);
        Locator.GetMinPoint(cell, bottomLeft);

        scalar0 = viskores::Float32(scalars.Get(cellId));
        viskores::Float32 normalizedScalar = (scalar0 - MinScalar) * InverseDeltaScalar;

        auto colorIndex = static_cast<viskores::Id>(normalizedScalar *
                                                    static_cast<viskores::Float32>(ColorMapSize));
        if (colorIndex < 0)
          colorIndex = 0;
        if (colorIndex > ColorMapSize)
          colorIndex = ColorMapSize;
        sampleColor = ColorMap.Get(colorIndex);

        newCell = false;
      }

      // just repeatably composite
      viskores::Float32 alpha = sampleColor[3] * (1.f - color[3]);
      color[0] = color[0] + sampleColor[0] * alpha;
      color[1] = color[1] + sampleColor[1] * alpha;
      color[2] = color[2] + sampleColor[2] * alpha;
      color[3] = alpha + color[3];

      // terminate the ray early if it became completely opaque.
      if (color[3] >= 1.f)
        break;

      //advance
      distance += SampleDistance;
      sampleLocation = sampleLocation + SampleDistance * rayDir;

      parametric = (sampleLocation - bottomLeft) * invSpacing;
    }

    color[0] = viskores::Min(color[0], 1.f);
    color[1] = viskores::Min(color[1], 1.f);
    color[2] = viskores::Min(color[2], 1.f);
    color[3] = viskores::Min(color[3], 1.f);

    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 0);
    colorBuffer.Set(pixelIndex * 4 + 0, color[0]);
    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 1);
    colorBuffer.Set(pixelIndex * 4 + 1, color[1]);
    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 2);
    colorBuffer.Set(pixelIndex * 4 + 2, color[2]);
    BOUNDS_CHECK(colorBuffer, pixelIndex * 4 + 3);
    colorBuffer.Set(pixelIndex * 4 + 3, color[3]);
  }
}; //SamplerCell

class CalcRayStart : public viskores::worklet::WorkletMapField
{
  viskores::Float32 Xmin;
  viskores::Float32 Ymin;
  viskores::Float32 Zmin;
  viskores::Float32 Xmax;
  viskores::Float32 Ymax;
  viskores::Float32 Zmax;

public:
  VISKORES_CONT
  explicit CalcRayStart(const viskores::Bounds boundingBox)
  {
    Xmin = static_cast<viskores::Float32>(boundingBox.X.Min);
    Xmax = static_cast<viskores::Float32>(boundingBox.X.Max);
    Ymin = static_cast<viskores::Float32>(boundingBox.Y.Min);
    Ymax = static_cast<viskores::Float32>(boundingBox.Y.Max);
    Zmin = static_cast<viskores::Float32>(boundingBox.Z.Min);
    Zmax = static_cast<viskores::Float32>(boundingBox.Z.Max);
  }

  VISKORES_EXEC
  static viskores::Float32 rcp(viskores::Float32 f) { return 1.0f / f; }

  VISKORES_EXEC
  static viskores::Float32 rcp_safe(viskores::Float32 f)
  {
    return rcp((fabs(f) < 1e-8f) ? 1e-8f : f);
  }

  using ControlSignature = void(FieldIn, FieldOut, FieldInOut, FieldInOut, FieldIn);
  using ExecutionSignature = void(_1, _2, _3, _4, _5);
  template <typename Precision>
  VISKORES_EXEC void operator()(const viskores::Vec<Precision, 3>& rayDir,
                                viskores::Float32& minDistance,
                                viskores::Float32& distance,
                                viskores::Float32& maxDistance,
                                const viskores::Vec<Precision, 3>& rayOrigin) const
  {
    auto dirx = static_cast<viskores::Float32>(rayDir[0]);
    auto diry = static_cast<viskores::Float32>(rayDir[1]);
    auto dirz = static_cast<viskores::Float32>(rayDir[2]);
    auto origx = static_cast<viskores::Float32>(rayOrigin[0]);
    auto origy = static_cast<viskores::Float32>(rayOrigin[1]);
    auto origz = static_cast<viskores::Float32>(rayOrigin[2]);

    viskores::Float32 invDirx = rcp_safe(dirx);
    viskores::Float32 invDiry = rcp_safe(diry);
    viskores::Float32 invDirz = rcp_safe(dirz);

    viskores::Float32 odirx = origx * invDirx;
    viskores::Float32 odiry = origy * invDiry;
    viskores::Float32 odirz = origz * invDirz;

    viskores::Float32 xmin = Xmin * invDirx - odirx;
    viskores::Float32 ymin = Ymin * invDiry - odiry;
    viskores::Float32 zmin = Zmin * invDirz - odirz;
    viskores::Float32 xmax = Xmax * invDirx - odirx;
    viskores::Float32 ymax = Ymax * invDiry - odiry;
    viskores::Float32 zmax = Zmax * invDirz - odirz;


    minDistance = viskores::Max(
      viskores::Max(viskores::Max(viskores::Min(ymin, ymax), viskores::Min(xmin, xmax)),
                    viskores::Min(zmin, zmax)),
      minDistance);
    viskores::Float32 exitDistance =
      viskores::Min(viskores::Min(viskores::Max(ymin, ymax), viskores::Max(xmin, xmax)),
                    viskores::Max(zmin, zmax));
    maxDistance = viskores::Min(maxDistance, exitDistance);
    if (maxDistance < minDistance)
    {
      minDistance = -1.f; //flag for miss
    }
    else
    {
      distance = minDistance;
    }
  }
}; //class CalcRayStart

void VolumeRendererStructured::SetColorMap(
  const viskores::cont::ArrayHandle<viskores::Vec4f_32>& colorMap)
{
  ColorMap = colorMap;
}

void VolumeRendererStructured::SetData(const viskores::cont::CoordinateSystem& coords,
                                       const viskores::cont::Field& scalarField,
                                       const viskores::cont::CellSetStructured<3>& cellset,
                                       const viskores::Range& scalarRange)
{
  IsUniformDataSet = !coords.GetData().IsType<CartesianArrayHandle>();
  IsSceneDirty = true;
  SpatialExtent = coords.GetBounds();
  Coordinates = coords;
  ScalarField = &scalarField;
  Cellset = cellset;
  ScalarRange = scalarRange;
}

void VolumeRendererStructured::Render(viskores::rendering::raytracing::Ray<viskores::Float32>& rays)
{
  auto functor = [&](auto device)
  {
    using Device = typename std::decay_t<decltype(device)>;
    VISKORES_IS_DEVICE_ADAPTER_TAG(Device);

    this->RenderOnDevice(rays, device);
    return true;
  };
  viskores::cont::TryExecute(functor);
}

//void
//VolumeRendererStructured::Render(viskores::rendering::raytracing::Ray<viskores::Float64>& rays)
//{
//  RenderFunctor<viskores::Float64> functor(this, rays);
//  viskores::cont::TryExecute(functor);
//}

template <typename Precision, typename Device>
void VolumeRendererStructured::RenderOnDevice(viskores::rendering::raytracing::Ray<Precision>& rays,
                                              Device)
{
  viskores::cont::Timer renderTimer{ Device() };
  renderTimer.Start();

  Logger* logger = Logger::GetInstance();
  logger->OpenLogEntry("volume_render_structured");
  logger->AddLogData("device", GetDeviceString(Device()));

  viskores::Vec3f_32 extent;
  extent[0] = static_cast<viskores::Float32>(this->SpatialExtent.X.Length());
  extent[1] = static_cast<viskores::Float32>(this->SpatialExtent.Y.Length());
  extent[2] = static_cast<viskores::Float32>(this->SpatialExtent.Z.Length());
  viskores::Float32 mag_extent = viskores::Magnitude(extent);
  viskores::Float32 meshEpsilon = mag_extent * 0.0001f;
  if (SampleDistance <= 0.f)
  {
    const viskores::Float32 defaultNumberOfSamples = 200.f;
    SampleDistance = mag_extent / defaultNumberOfSamples;
  }

  viskores::cont::Invoker invoke;

  viskores::cont::Timer timer{ Device() };
  timer.Start();
  invoke(CalcRayStart{ this->SpatialExtent },
         rays.Dir,
         rays.MinDistance,
         rays.Distance,
         rays.MaxDistance,
         rays.Origin);
  viskores::Float64 time = timer.GetElapsedTime();
  logger->AddLogData("calc_ray_start", time);

  timer.Start();

  const bool isSupportedField = ScalarField->IsCellField() || ScalarField->IsPointField();
  if (!isSupportedField)
  {
    throw viskores::cont::ErrorBadValue("Field not accociated with cell set or points");
  }
  const bool isAssocPoints = ScalarField->IsPointField();

  if (IsUniformDataSet)
  {
    viskores::cont::Token token;
    viskores::cont::ArrayHandleUniformPointCoordinates vertices;
    vertices =
      Coordinates.GetData().AsArrayHandle<viskores::cont::ArrayHandleUniformPointCoordinates>();
    viskores::cont::CellLocatorUniformGrid uniLocator;
    uniLocator.SetCellSet(this->Cellset);
    uniLocator.SetCoordinates(this->Coordinates);
    UniformLocatorAdapter<Device> locator(vertices, this->Cellset, uniLocator, token);

    if (isAssocPoints)
    {
      auto sampler =
        Sampler<Device, UniformLocatorAdapter<Device>>(ColorMap,
                                                       viskores::Float32(ScalarRange.Min),
                                                       viskores::Float32(ScalarRange.Max),
                                                       SampleDistance,
                                                       locator,
                                                       meshEpsilon,
                                                       token);
      invoke(sampler,
             rays.Dir,
             rays.Origin,
             rays.MinDistance,
             rays.MaxDistance,
             rays.Buffers.at(0).Buffer,
             viskores::rendering::raytracing::GetScalarFieldArray(*this->ScalarField));
    }
    else
    {
      auto sampler =
        SamplerCellAssoc<Device, UniformLocatorAdapter<Device>>(ColorMap,
                                                                viskores::Float32(ScalarRange.Min),
                                                                viskores::Float32(ScalarRange.Max),
                                                                SampleDistance,
                                                                locator,
                                                                meshEpsilon,
                                                                token);
      invoke(sampler,
             rays.Dir,
             rays.Origin,
             rays.MinDistance,
             rays.MaxDistance,
             rays.Buffers.at(0).Buffer,
             viskores::rendering::raytracing::GetScalarFieldArray(*this->ScalarField));
    }
  }
  else
  {
    viskores::cont::Token token;
    CartesianArrayHandle vertices;
    vertices = Coordinates.GetData().AsArrayHandle<CartesianArrayHandle>();
    viskores::cont::CellLocatorRectilinearGrid rectLocator;
    rectLocator.SetCellSet(this->Cellset);
    rectLocator.SetCoordinates(this->Coordinates);
    RectilinearLocatorAdapter<Device> locator(vertices, Cellset, rectLocator, token);

    if (isAssocPoints)
    {
      auto sampler =
        Sampler<Device, RectilinearLocatorAdapter<Device>>(ColorMap,
                                                           viskores::Float32(ScalarRange.Min),
                                                           viskores::Float32(ScalarRange.Max),
                                                           SampleDistance,
                                                           locator,
                                                           meshEpsilon,
                                                           token);
      invoke(sampler,
             rays.Dir,
             rays.Origin,
             rays.MinDistance,
             rays.MaxDistance,
             rays.Buffers.at(0).Buffer,
             viskores::rendering::raytracing::GetScalarFieldArray(*this->ScalarField));
    }
    else
    {
      auto sampler = SamplerCellAssoc<Device, RectilinearLocatorAdapter<Device>>(
        ColorMap,
        viskores::Float32(ScalarRange.Min),
        viskores::Float32(ScalarRange.Max),
        SampleDistance,
        locator,
        meshEpsilon,
        token);
      invoke(sampler,
             rays.Dir,
             rays.Origin,
             rays.MinDistance,
             rays.MaxDistance,
             rays.Buffers.at(0).Buffer,
             viskores::rendering::raytracing::GetScalarFieldArray(*this->ScalarField));
    }
  }

  time = timer.GetElapsedTime();
  logger->AddLogData("sample", time);

  time = renderTimer.GetElapsedTime();
  logger->CloseLogEntry(time);
} //Render

void VolumeRendererStructured::SetSampleDistance(const viskores::Float32& distance)
{
  if (distance <= 0.f)
    throw viskores::cont::ErrorBadValue("Sample distance must be positive.");
  SampleDistance = distance;
}
}
}
} //namespace viskores::rendering::raytracing
