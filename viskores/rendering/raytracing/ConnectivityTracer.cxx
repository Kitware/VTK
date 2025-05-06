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
#include <viskores/rendering/raytracing/ConnectivityTracer.h>

#include <viskores/cont/DeviceAdapterAlgorithm.h>
#include <viskores/cont/ErrorBadValue.h>
#include <viskores/cont/Timer.h>
#include <viskores/cont/TryExecute.h>
#include <viskores/cont/internal/DeviceAdapterListHelpers.h>

#include <viskores/rendering/raytracing/Camera.h>
#include <viskores/rendering/raytracing/CellIntersector.h>
#include <viskores/rendering/raytracing/CellSampler.h>
#include <viskores/rendering/raytracing/CellTables.h>
#include <viskores/rendering/raytracing/MeshConnectivityBuilder.h>
#include <viskores/rendering/raytracing/Ray.h>
#include <viskores/rendering/raytracing/RayOperations.h>
#include <viskores/rendering/raytracing/RayTracingTypeDefs.h>
#include <viskores/rendering/raytracing/Worklets.h>

#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/DispatcherMapTopology.h>
#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/WorkletMapTopology.h>

#include <iomanip>

#ifndef CELL_SHAPE_ZOO
#define CELL_SHAPE_ZOO 255
#endif

#ifndef CELL_SHAPE_STRUCTURED
#define CELL_SHAPE_STRUCTURED 254
#endif

namespace viskores
{
namespace rendering
{
namespace raytracing
{
namespace detail
{

class AdjustSample : public viskores::worklet::WorkletMapField
{
  viskores::Float64 SampleDistance;

public:
  VISKORES_CONT
  AdjustSample(const viskores::Float64 sampleDistance)
    : SampleDistance(sampleDistance)
  {
  }
  using ControlSignature = void(FieldIn, FieldInOut);
  using ExecutionSignature = void(_1, _2);
  template <typename FloatType>
  VISKORES_EXEC inline void operator()(const viskores::UInt8& status,
                                       FloatType& currentDistance) const
  {
    if (status != RAY_ACTIVE)
      return;

    currentDistance += FMod(currentDistance, (FloatType)SampleDistance);
  }
}; //class AdvanceRay

template <typename FloatType>
void RayTracking<FloatType>::Compact(viskores::cont::ArrayHandle<FloatType>& compactedDistances,
                                     viskores::cont::ArrayHandle<UInt8>& masks)
{
  //
  // These distances are stored in the rays, and it has
  // already been compacted.
  //
  CurrentDistance = compactedDistances;

  viskores::cont::ArrayHandleCast<viskores::Id, viskores::cont::ArrayHandle<viskores::UInt8>>
    castedMasks(masks);

  bool distance1IsEnter = EnterDist == &Distance1;

  viskores::cont::ArrayHandle<FloatType> compactedDistance1;
  viskores::cont::Algorithm::CopyIf(Distance1, masks, compactedDistance1);
  Distance1 = compactedDistance1;

  viskores::cont::ArrayHandle<FloatType> compactedDistance2;
  viskores::cont::Algorithm::CopyIf(Distance2, masks, compactedDistance2);
  Distance2 = compactedDistance2;

  viskores::cont::ArrayHandle<viskores::Int32> compactedExitFace;
  viskores::cont::Algorithm::CopyIf(ExitFace, masks, compactedExitFace);
  ExitFace = compactedExitFace;

  if (distance1IsEnter)
  {
    EnterDist = &Distance1;
    ExitDist = &Distance2;
  }
  else
  {
    EnterDist = &Distance2;
    ExitDist = &Distance1;
  }
}

template <typename FloatType>
void RayTracking<FloatType>::Init(const viskores::Id size,
                                  viskores::cont::ArrayHandle<FloatType>& distances)
{

  ExitFace.Allocate(size);
  Distance1.Allocate(size);
  Distance2.Allocate(size);

  CurrentDistance = distances;
  //
  // Set the initial Distances
  //
  viskores::worklet::DispatcherMapField<CopyAndOffset<FloatType>> resetDistancesDispatcher(
    CopyAndOffset<FloatType>(0.0f));
  resetDistancesDispatcher.Invoke(distances, *EnterDist);

  //
  // Init the exit faces. This value is used to load the next cell
  // base on the cell and face it left
  //
  viskores::cont::ArrayHandleConstant<viskores::Int32> negOne(-1, size);
  viskores::cont::Algorithm::Copy(negOne, ExitFace);

  viskores::cont::ArrayHandleConstant<FloatType> negOnef(-1.f, size);
  viskores::cont::Algorithm::Copy(negOnef, *ExitDist);
}

template <typename FloatType>
void RayTracking<FloatType>::Swap()
{
  viskores::cont::ArrayHandle<FloatType>* tmpPtr;
  tmpPtr = EnterDist;
  EnterDist = ExitDist;
  ExitDist = tmpPtr;
}

} //namespace detail

void ConnectivityTracer::Init()
{
  //
  // Check to see if a sample distance was set
  //
  viskores::Bounds coordsBounds = Coords.GetBounds();
  viskores::Float64 maxLength = 0.;
  maxLength = viskores::Max(maxLength, coordsBounds.X.Length());
  maxLength = viskores::Max(maxLength, coordsBounds.Y.Length());
  maxLength = viskores::Max(maxLength, coordsBounds.Z.Length());
  BumpDistance = maxLength * BumpEpsilon;

  if (SampleDistance <= 0)
  {
    BoundingBox[0] = viskores::Float32(coordsBounds.X.Min);
    BoundingBox[1] = viskores::Float32(coordsBounds.X.Max);
    BoundingBox[2] = viskores::Float32(coordsBounds.Y.Min);
    BoundingBox[3] = viskores::Float32(coordsBounds.Y.Max);
    BoundingBox[4] = viskores::Float32(coordsBounds.Z.Min);
    BoundingBox[5] = viskores::Float32(coordsBounds.Z.Max);

    BackgroundColor[0] = 1.f;
    BackgroundColor[1] = 1.f;
    BackgroundColor[2] = 1.f;
    BackgroundColor[3] = 1.f;
    const viskores::Float32 defaultSampleRate = 200.f;
    // We need to set some default sample distance
    viskores::Vec3f_32 extent;
    extent[0] = BoundingBox[1] - BoundingBox[0];
    extent[1] = BoundingBox[3] - BoundingBox[2];
    extent[2] = BoundingBox[5] - BoundingBox[4];
    SampleDistance = viskores::Magnitude(extent) / defaultSampleRate;
  }
}

viskores::Id ConnectivityTracer::GetNumberOfMeshCells() const
{
  return CellSet.GetNumberOfCells();
}

void ConnectivityTracer::SetColorMap(
  const viskores::cont::ArrayHandle<viskores::Vec4f_32>& colorMap)
{
  ColorMap = colorMap;
}

void ConnectivityTracer::SetVolumeData(const viskores::cont::Field& scalarField,
                                       const viskores::Range& scalarBounds,
                                       const viskores::cont::UnknownCellSet& cellSet,
                                       const viskores::cont::CoordinateSystem& coords,
                                       const viskores::cont::Field& ghostField)
{
  //TODO: Need a way to tell if we have been updated
  ScalarField = scalarField;
  GhostField = ghostField;
  ScalarBounds = scalarBounds;
  CellSet = cellSet;
  Coords = coords;
  MeshConnIsConstructed = false;

  const bool isSupportedField = ScalarField.IsCellField() || ScalarField.IsPointField();
  if (!isSupportedField)
  {
    throw viskores::cont::ErrorBadValue("Field not accociated with cell set or points");
  }
  FieldAssocPoints = ScalarField.IsPointField();

  this->Integrator = Volume;

  if (MeshContainer == nullptr)
  {
    delete MeshContainer;
  }
  MeshConnectivityBuilder builder;
  MeshContainer = builder.BuildConnectivity(cellSet, coords);

  Locator.SetCellSet(this->CellSet);
  Locator.SetCoordinates(this->Coords);
  Locator.Update();
}

void ConnectivityTracer::SetEnergyData(const viskores::cont::Field& absorption,
                                       const viskores::Int32 numBins,
                                       const viskores::cont::UnknownCellSet& cellSet,
                                       const viskores::cont::CoordinateSystem& coords,
                                       const viskores::cont::Field& emission)
{
  bool isSupportedField = absorption.GetAssociation() == viskores::cont::Field::Association::Cells;
  if (!isSupportedField)
    throw viskores::cont::ErrorBadValue("Absorption Field '" + absorption.GetName() +
                                        "' not accociated with cells");
  ScalarField = absorption;
  CellSet = cellSet;
  Coords = coords;
  MeshConnIsConstructed = false;
  // Check for emission
  HasEmission = false;

  if (emission.GetAssociation() != viskores::cont::Field::Association::Any)
  {
    if (emission.GetAssociation() != viskores::cont::Field::Association::Cells)
      throw viskores::cont::ErrorBadValue("Emission Field '" + emission.GetName() +
                                          "' not accociated with cells");
    HasEmission = true;
    EmissionField = emission;
  }
  // Do some basic range checking
  if (numBins < 1)
    throw viskores::cont::ErrorBadValue("Number of energy bins is less than 1");
  viskores::Id binCount = ScalarField.GetNumberOfValues();
  viskores::Id cellCount = this->GetNumberOfMeshCells();
  if (cellCount != (binCount / viskores::Id(numBins)))
  {
    std::stringstream message;
    message << "Invalid number of absorption bins\n";
    message << "Number of cells: " << cellCount << "\n";
    message << "Number of field values: " << binCount << "\n";
    message << "Number of bins: " << numBins << "\n";
    throw viskores::cont::ErrorBadValue(message.str());
  }
  if (HasEmission)
  {
    binCount = EmissionField.GetNumberOfValues();
    if (cellCount != (binCount / viskores::Id(numBins)))
    {
      std::stringstream message;
      message << "Invalid number of emission bins\n";
      message << "Number of cells: " << cellCount << "\n";
      message << "Number of field values: " << binCount << "\n";
      message << "Number of bins: " << numBins << "\n";
      throw viskores::cont::ErrorBadValue(message.str());
    }
  }
  //TODO: Need a way to tell if we have been updated
  this->Integrator = Energy;

  if (MeshContainer == nullptr)
  {
    delete MeshContainer;
  }

  MeshConnectivityBuilder builder;
  MeshContainer = builder.BuildConnectivity(cellSet, coords);
  Locator.SetCellSet(this->CellSet);
  Locator.SetCoordinates(this->Coords);
  Locator.Update();
}

void ConnectivityTracer::SetBackgroundColor(const viskores::Vec4f_32& backgroundColor)
{
  BackgroundColor = backgroundColor;
}

void ConnectivityTracer::SetSampleDistance(const viskores::Float32& distance)
{
  if (distance <= 0.f)
    throw viskores::cont::ErrorBadValue("Sample distance must be positive.");
  SampleDistance = distance;
}

void ConnectivityTracer::ResetTimers()
{
  IntersectTime = 0.;
  IntegrateTime = 0.;
  SampleTime = 0.;
  LostRayTime = 0.;
  MeshEntryTime = 0.;
}

void ConnectivityTracer::LogTimers()
{
  Logger* logger = Logger::GetInstance();
  logger->AddLogData("intersect ", IntersectTime);
  logger->AddLogData("integrate ", IntegrateTime);
  logger->AddLogData("sample_cells ", SampleTime);
  logger->AddLogData("lost_rays ", LostRayTime);
  logger->AddLogData("mesh_entry", LostRayTime);
}

template <typename FloatType>
void ConnectivityTracer::PrintRayStatus(Ray<FloatType>& rays)
{
  viskores::Id raysExited = RayOperations::GetStatusCount(rays, RAY_EXITED_MESH);
  viskores::Id raysActive = RayOperations::GetStatusCount(rays, RAY_ACTIVE);
  viskores::Id raysAbandoned = RayOperations::GetStatusCount(rays, RAY_ABANDONED);
  viskores::Id raysExitedDom = RayOperations::GetStatusCount(rays, RAY_EXITED_DOMAIN);
  std::cout << "\r Ray Status " << std::setw(10) << std::left << " Lost " << std::setw(10)
            << std::left << RaysLost << std::setw(10) << std::left << " Exited " << std::setw(10)
            << std::left << raysExited << std::setw(10) << std::left << " Active " << std::setw(10)
            << raysActive << std::setw(10) << std::left << " Abandoned " << std::setw(10)
            << raysAbandoned << " Exited Domain " << std::setw(10) << std::left << raysExitedDom
            << "\n";
}

//
//  Advance Ray
//      After a ray leaves the mesh, we need to check to see
//      of the ray re-enters the mesh within this domain. This
//      function moves the ray forward some offset to prevent
//      "shadowing" and hitting the same exit point.
//
template <typename FloatType>
class AdvanceRay : public viskores::worklet::WorkletMapField
{
  FloatType Offset;

public:
  VISKORES_CONT
  AdvanceRay(const FloatType offset = 0.00001)
    : Offset(offset)
  {
  }
  using ControlSignature = void(FieldIn, FieldInOut);
  using ExecutionSignature = void(_1, _2);

  VISKORES_EXEC inline void operator()(const viskores::UInt8& status, FloatType& distance) const
  {
    if (status == RAY_EXITED_MESH)
      distance += Offset;
  }
}; //class AdvanceRay

class LocateCell : public viskores::worklet::WorkletMapField
{
private:
  CellIntersector<255> Intersector;

public:
  LocateCell() {}

  using ControlSignature = void(FieldInOut,
                                WholeArrayIn,
                                FieldIn,
                                FieldInOut,
                                FieldInOut,
                                FieldInOut,
                                FieldInOut,
                                FieldIn,
                                ExecObject meshConnectivity);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7, _8, _9);

  template <typename FloatType, typename PointPortalType>
  VISKORES_EXEC inline void operator()(viskores::Id& currentCell,
                                       PointPortalType& vertices,
                                       const viskores::Vec<FloatType, 3>& dir,
                                       FloatType& enterDistance,
                                       FloatType& exitDistance,
                                       viskores::Int32& enterFace,
                                       viskores::UInt8& rayStatus,
                                       const viskores::Vec<FloatType, 3>& origin,
                                       const MeshConnectivity& meshConn) const
  {
    if (enterFace != -1 && rayStatus == RAY_ACTIVE)
    {
      currentCell = meshConn.GetConnectingCell(currentCell, enterFace);
      if (currentCell == -1)
        rayStatus = RAY_EXITED_MESH;
      enterFace = -1;
    }
    //This ray is dead or exited the mesh and needs re-entry
    if (rayStatus != RAY_ACTIVE)
    {
      return;
    }
    FloatType xpoints[8];
    FloatType ypoints[8];
    FloatType zpoints[8];
    viskores::Id cellConn[8];
    FloatType distances[6];

    const viskores::Int32 numIndices = meshConn.GetCellIndices(cellConn, currentCell);
    //load local cell data
    for (int i = 0; i < numIndices; ++i)
    {
      BOUNDS_CHECK(vertices, cellConn[i]);
      viskores::Vec<FloatType, 3> point = viskores::Vec<FloatType, 3>(vertices.Get(cellConn[i]));
      xpoints[i] = point[0];
      ypoints[i] = point[1];
      zpoints[i] = point[2];
    }
    const viskores::UInt8 cellShape = meshConn.GetCellShape(currentCell);
    Intersector.IntersectCell(xpoints, ypoints, zpoints, dir, origin, distances, cellShape);

    CellTables tables;
    const viskores::Int32 numFaces = tables.FaceLookUp(tables.CellTypeLookUp(cellShape), 1);
    //viskores::Int32 minFace = 6;
    viskores::Int32 maxFace = -1;

    FloatType minDistance = static_cast<FloatType>(1e32);
    FloatType maxDistance = static_cast<FloatType>(-1);
    for (viskores::Int32 i = 0; i < numFaces; ++i)
    {
      FloatType dist = distances[i];

      if (dist != -1)
      {
        if (dist < minDistance)
        {
          minDistance = dist;
          //minFace = i;
        }
        if (dist > maxDistance)
        {
          maxDistance = dist;
          maxFace = i;
        }
      }
    }

    if (maxDistance <= enterDistance || minDistance == maxDistance)
    {
      rayStatus = RAY_LOST;
    }
    else
    {
      enterDistance = minDistance;
      exitDistance = maxDistance;
      enterFace = maxFace;
    }

  } //operator
};  //class LocateCell

class RayBumper : public viskores::worklet::WorkletMapField
{
private:
  CellIntersector<255> Intersector;
  viskores::Float64 BumpDistance;

public:
  RayBumper(viskores::Float64 bumpDistance)
    : BumpDistance(bumpDistance)
  {
  }


  using ControlSignature = void(FieldInOut,
                                WholeArrayIn,
                                FieldInOut,
                                FieldInOut,
                                FieldInOut,
                                FieldInOut,
                                FieldIn,
                                FieldInOut,
                                ExecObject meshConnectivity,
                                ExecObject locator);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10);

  template <typename FloatType, typename PointPortalType, typename LocatorType>
  VISKORES_EXEC inline void operator()(viskores::Id& currentCell,
                                       PointPortalType& vertices,
                                       FloatType& enterDistance,
                                       FloatType& exitDistance,
                                       viskores::Int32& enterFace,
                                       viskores::UInt8& rayStatus,
                                       const viskores::Vec<FloatType, 3>& origin,
                                       viskores::Vec<FloatType, 3>& rdir,
                                       const MeshConnectivity& meshConn,
                                       const LocatorType& locator) const
  {
    // We only process lost rays
    if (rayStatus != RAY_LOST)
    {
      return;
    }
    const FloatType bumpDistance = static_cast<FloatType>(BumpDistance);
    FloatType query_distance = enterDistance + bumpDistance;

    bool valid_cell = false;

    viskores::Id cellId = currentCell;

    while (!valid_cell)
    {
      // push forward and look for a new cell
      while (cellId == currentCell)
      {
        query_distance += bumpDistance;
        viskores::Vec<FloatType, 3> location = origin + rdir * (query_distance);
        viskores::Vec<viskores::FloatDefault, 3> pcoords;
        locator.FindCell(location, cellId, pcoords);
      }

      currentCell = cellId;
      if (currentCell == -1)
      {
        rayStatus = RAY_EXITED_MESH;
        return;
      }

      FloatType xpoints[8];
      FloatType ypoints[8];
      FloatType zpoints[8];
      viskores::Id cellConn[8];
      FloatType distances[6];

      const viskores::Int32 numIndices = meshConn.GetCellIndices(cellConn, currentCell);
      //load local cell data
      for (int i = 0; i < numIndices; ++i)
      {
        BOUNDS_CHECK(vertices, cellConn[i]);
        viskores::Vec<FloatType, 3> point = viskores::Vec<FloatType, 3>(vertices.Get(cellConn[i]));
        xpoints[i] = point[0];
        ypoints[i] = point[1];
        zpoints[i] = point[2];
      }

      const viskores::UInt8 cellShape = meshConn.GetCellShape(currentCell);
      Intersector.IntersectCell(xpoints, ypoints, zpoints, rdir, origin, distances, cellShape);

      CellTables tables;
      const viskores::Int32 numFaces = tables.FaceLookUp(tables.CellTypeLookUp(cellShape), 1);

      //viskores::Int32 minFace = 6;
      viskores::Int32 maxFace = -1;
      FloatType minDistance = static_cast<FloatType>(1e32);
      FloatType maxDistance = static_cast<FloatType>(-1);
      for (int i = 0; i < numFaces; ++i)
      {
        FloatType dist = distances[i];

        if (dist != -1)
        {
          if (dist < minDistance)
          {
            minDistance = dist;
            //minFace = i;
          }
          if (dist >= maxDistance)
          {
            maxDistance = dist;
            maxFace = i;
          }
        }
      }

      if (minDistance < maxDistance && minDistance > exitDistance)
      {
        enterDistance = minDistance;
        exitDistance = maxDistance;
        enterFace = maxFace;
        rayStatus = RAY_ACTIVE; //re-activate ray
        valid_cell = true;
      }
    }

  } //operator
};  //class RayBumper

class AddPathLengths : public viskores::worklet::WorkletMapField
{
public:
  VISKORES_CONT
  AddPathLengths() {}

  using ControlSignature = void(FieldIn,     // ray status
                                FieldIn,     // cell enter distance
                                FieldIn,     // cell exit distance
                                FieldInOut); // ray absorption data

  using ExecutionSignature = void(_1, _2, _3, _4);

  template <typename FloatType>
  VISKORES_EXEC inline void operator()(const viskores::UInt8& rayStatus,
                                       const FloatType& enterDistance,
                                       const FloatType& exitDistance,
                                       FloatType& distance) const
  {
    if (rayStatus != RAY_ACTIVE)
    {
      return;
    }

    if (exitDistance <= enterDistance)
    {
      return;
    }

    FloatType segmentLength = exitDistance - enterDistance;
    distance += segmentLength;
  }
};

class Integrate : public viskores::worklet::WorkletMapField
{
private:
  const viskores::Int32 NumBins;
  const viskores::Float32 UnitScalar;

public:
  VISKORES_CONT
  Integrate(const viskores::Int32 numBins, const viskores::Float32 unitScalar)
    : NumBins(numBins)
    , UnitScalar(unitScalar)
  {
  }

  using ControlSignature = void(FieldIn,         // ray status
                                FieldIn,         // cell enter distance
                                FieldIn,         // cell exit distance
                                FieldInOut,      // current distance
                                WholeArrayIn,    // cell absorption data array
                                WholeArrayInOut, // ray absorption data
                                FieldIn);        // current cell

  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7, WorkIndex);

  template <typename FloatType, typename CellDataPortalType, typename RayDataPortalType>
  VISKORES_EXEC inline void operator()(const viskores::UInt8& rayStatus,
                                       const FloatType& enterDistance,
                                       const FloatType& exitDistance,
                                       FloatType& currentDistance,
                                       const CellDataPortalType& cellData,
                                       RayDataPortalType& energyBins,
                                       const viskores::Id& currentCell,
                                       const viskores::Id& rayIndex) const
  {
    if (rayStatus != RAY_ACTIVE)
    {
      return;
    }
    if (exitDistance <= enterDistance)
    {
      return;
    }

    FloatType segmentLength = exitDistance - enterDistance;

    viskores::Id rayOffset = NumBins * rayIndex;
    viskores::Id cellOffset = NumBins * currentCell;
    for (viskores::Int32 i = 0; i < NumBins; ++i)
    {
      BOUNDS_CHECK(cellData, cellOffset + i);
      FloatType absorb = static_cast<FloatType>(cellData.Get(cellOffset + i));
      absorb *= UnitScalar;
      absorb = viskores::Exp(-absorb * segmentLength);
      BOUNDS_CHECK(energyBins, rayOffset + i);
      FloatType intensity = static_cast<FloatType>(energyBins.Get(rayOffset + i));
      energyBins.Set(rayOffset + i, intensity * absorb);
    }
    currentDistance = exitDistance;
  }
};

class IntegrateEmission : public viskores::worklet::WorkletMapField
{
private:
  const viskores::Int32 NumBins;
  const viskores::Float32 UnitScalar;
  bool DivideEmisByAbsorb;

public:
  VISKORES_CONT
  IntegrateEmission(const viskores::Int32 numBins,
                    const viskores::Float32 unitScalar,
                    const bool divideEmisByAbsorb)
    : NumBins(numBins)
    , UnitScalar(unitScalar)
    , DivideEmisByAbsorb(divideEmisByAbsorb)
  {
  }

  using ControlSignature = void(FieldIn,         // ray status
                                FieldIn,         // cell enter distance
                                FieldIn,         // cell exit distance
                                FieldInOut,      // current distance
                                WholeArrayIn,    // cell absorption data array
                                WholeArrayIn,    // cell emission data array
                                WholeArrayInOut, // ray absorption data
                                WholeArrayInOut, // ray emission data
                                FieldIn);        // current cell

  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7, _8, _9, WorkIndex);

  template <typename FloatType,
            typename CellAbsPortalType,
            typename CellEmisPortalType,
            typename RayDataPortalType>
  VISKORES_EXEC inline void operator()(const viskores::UInt8& rayStatus,
                                       const FloatType& enterDistance,
                                       const FloatType& exitDistance,
                                       FloatType& currentDistance,
                                       const CellAbsPortalType& absorptionData,
                                       const CellEmisPortalType& emissionData,
                                       RayDataPortalType& absorptionBins,
                                       RayDataPortalType& emissionBins,
                                       const viskores::Id& currentCell,
                                       const viskores::Id& rayIndex) const
  {
    if (rayStatus != RAY_ACTIVE)
    {
      return;
    }
    if (exitDistance <= enterDistance)
    {
      return;
    }

    FloatType segmentLength = exitDistance - enterDistance;

    viskores::Id rayOffset = NumBins * rayIndex;
    viskores::Id cellOffset = NumBins * currentCell;
    for (viskores::Int32 i = 0; i < NumBins; ++i)
    {
      BOUNDS_CHECK(absorptionData, cellOffset + i);
      FloatType absorb = static_cast<FloatType>(absorptionData.Get(cellOffset + i));
      BOUNDS_CHECK(emissionData, cellOffset + i);
      FloatType emission = static_cast<FloatType>(emissionData.Get(cellOffset + i));

      absorb *= UnitScalar;
      emission *= UnitScalar;

      if (DivideEmisByAbsorb)
      {
        emission /= absorb;
      }

      FloatType tmp = viskores::Exp(-absorb * segmentLength);
      BOUNDS_CHECK(absorptionBins, rayOffset + i);

      //
      // Traditionally, we would only keep track of a single intensity value per ray
      // per bin and we would integrate from the beginning to end of the ray. In a
      // distributed memory setting, we would move cell data around so that the
      // entire ray could be traced, but in situ, moving that much cell data around
      // could blow memory. Here we are keeping track of two values. Total absorption
      // through this contiguous segment of the mesh, and the amount of emitted energy
      // that makes it out of this mesh segment. If this is really run on a single node,
      // we can get the final energy value by multiplying the background intensity by
      // the total absorption of the mesh segment and add in the amount of emitted
      // energy that escapes.
      //
      FloatType absorbIntensity = static_cast<FloatType>(absorptionBins.Get(rayOffset + i));
      FloatType emissionIntensity = static_cast<FloatType>(emissionBins.Get(rayOffset + i));

      absorptionBins.Set(rayOffset + i, absorbIntensity * tmp);

      emissionIntensity = emissionIntensity * tmp + emission * (1.f - tmp);

      BOUNDS_CHECK(emissionBins, rayOffset + i);
      emissionBins.Set(rayOffset + i, emissionIntensity);
    }
    currentDistance = exitDistance;
  }
};
//
//  IdentifyMissedRay is a debugging routine that detects
//  rays that fail to have any value because of a external
//  intersection and cell intersection mismatch
//
//
class IdentifyMissedRay : public viskores::worklet::WorkletMapField
{
public:
  viskores::Id Width;
  viskores::Id Height;
  viskores::Vec4f_32 BGColor;
  IdentifyMissedRay(const viskores::Id width, const viskores::Id height, viskores::Vec4f_32 bgcolor)
    : Width(width)
    , Height(height)
    , BGColor(bgcolor)
  {
  }
  using ControlSignature = void(FieldIn, WholeArrayIn);
  using ExecutionSignature = void(_1, _2);


  VISKORES_EXEC inline bool IsBGColor(const viskores::Vec4f_32 color) const
  {
    bool isBG = false;

    if (color[0] == BGColor[0] && color[1] == BGColor[1] && color[2] == BGColor[2] &&
        color[3] == BGColor[3])
      isBG = true;
    return isBG;
  }

  template <typename ColorBufferType>
  VISKORES_EXEC inline void operator()(const viskores::Id& pixelId, ColorBufferType& buffer) const
  {
    viskores::Id x = pixelId % Width;
    viskores::Id y = pixelId / Width;

    // Conservative check, we only want to check pixels in the middle
    if (x <= 0 || y <= 0)
      return;
    if (x >= Width - 1 || y >= Height - 1)
      return;
    viskores::Vec4f_32 pixel;
    pixel[0] = static_cast<viskores::Float32>(buffer.Get(pixelId * 4 + 0));
    pixel[1] = static_cast<viskores::Float32>(buffer.Get(pixelId * 4 + 1));
    pixel[2] = static_cast<viskores::Float32>(buffer.Get(pixelId * 4 + 2));
    pixel[3] = static_cast<viskores::Float32>(buffer.Get(pixelId * 4 + 3));
    if (!IsBGColor(pixel))
      return;
    viskores::Id p0 = (y)*Width + (x + 1);
    viskores::Id p1 = (y)*Width + (x - 1);
    viskores::Id p2 = (y + 1) * Width + (x);
    viskores::Id p3 = (y - 1) * Width + (x);
    pixel[0] = static_cast<viskores::Float32>(buffer.Get(p0 * 4 + 0));
    pixel[1] = static_cast<viskores::Float32>(buffer.Get(p0 * 4 + 1));
    pixel[2] = static_cast<viskores::Float32>(buffer.Get(p0 * 4 + 2));
    pixel[3] = static_cast<viskores::Float32>(buffer.Get(p0 * 4 + 3));
    if (IsBGColor(pixel))
      return;
    pixel[0] = static_cast<viskores::Float32>(buffer.Get(p1 * 4 + 0));
    pixel[1] = static_cast<viskores::Float32>(buffer.Get(p1 * 4 + 1));
    pixel[2] = static_cast<viskores::Float32>(buffer.Get(p1 * 4 + 2));
    pixel[3] = static_cast<viskores::Float32>(buffer.Get(p1 * 4 + 3));
    if (IsBGColor(pixel))
      return;
    pixel[0] = static_cast<viskores::Float32>(buffer.Get(p2 * 4 + 0));
    pixel[1] = static_cast<viskores::Float32>(buffer.Get(p2 * 4 + 1));
    pixel[2] = static_cast<viskores::Float32>(buffer.Get(p2 * 4 + 2));
    pixel[3] = static_cast<viskores::Float32>(buffer.Get(p2 * 4 + 3));
    if (IsBGColor(pixel))
      return;
    pixel[0] = static_cast<viskores::Float32>(buffer.Get(p3 * 4 + 0));
    pixel[1] = static_cast<viskores::Float32>(buffer.Get(p3 * 4 + 1));
    pixel[2] = static_cast<viskores::Float32>(buffer.Get(p3 * 4 + 2));
    pixel[3] = static_cast<viskores::Float32>(buffer.Get(p3 * 4 + 3));
    if (IsBGColor(pixel))
      return;

    printf("Possible error ray missed ray %d\n", (int)pixelId);
  }
};

template <typename FloatType>
class SampleCellAssocCells : public viskores::worklet::WorkletMapField
{
private:
  CellSampler<255> Sampler;
  FloatType SampleDistance;
  FloatType MinScalar;
  FloatType InvDeltaScalar;

public:
  SampleCellAssocCells(const FloatType& sampleDistance,
                       const FloatType& minScalar,
                       const FloatType& maxScalar)
    : SampleDistance(sampleDistance)
    , MinScalar(minScalar)
  {
    InvDeltaScalar = (minScalar == maxScalar) ? 1.f : 1.f / (maxScalar - minScalar);
  }

  using ControlSignature = void(FieldIn,
                                WholeArrayIn,
                                WholeArrayIn,
                                FieldIn,
                                FieldIn,
                                FieldInOut,
                                FieldInOut,
                                WholeArrayIn,
                                WholeArrayInOut,
                                FieldIn);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7, _8, _9, WorkIndex, _10);

  template <typename ScalarPortalType,
            typename GhostPortalType,
            typename ColorMapType,
            typename FrameBufferType>
  VISKORES_EXEC inline void operator()(const viskores::Id& currentCell,
                                       ScalarPortalType& scalarPortal,
                                       GhostPortalType& ghostPortal,
                                       const FloatType& enterDistance,
                                       const FloatType& exitDistance,
                                       FloatType& currentDistance,
                                       viskores::UInt8& rayStatus,
                                       const ColorMapType& colorMap,
                                       FrameBufferType& frameBuffer,
                                       const viskores::Id& pixelIndex,
                                       const FloatType& maxDistance) const
  {

    if (rayStatus != RAY_ACTIVE)
      return;
    if (int(ghostPortal.Get(currentCell)) != 0)
      return;

    viskores::Vec4f_32 color;
    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 0);
    color[0] = static_cast<viskores::Float32>(frameBuffer.Get(pixelIndex * 4 + 0));
    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 1);
    color[1] = static_cast<viskores::Float32>(frameBuffer.Get(pixelIndex * 4 + 1));
    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 2);
    color[2] = static_cast<viskores::Float32>(frameBuffer.Get(pixelIndex * 4 + 2));
    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 3);
    color[3] = static_cast<viskores::Float32>(frameBuffer.Get(pixelIndex * 4 + 3));

    viskores::Float32 scalar;
    BOUNDS_CHECK(scalarPortal, currentCell);
    scalar = viskores::Float32(scalarPortal.Get(currentCell));
    //
    // There can be mismatches in the initial enter distance and the current distance
    // due to lost rays at cell borders. For now,
    // we will just advance the current position to the enter distance, since otherwise,
    // the pixel would never be sampled.
    //
    if (currentDistance < enterDistance)
      currentDistance = enterDistance;

    const viskores::Id colorMapSize = colorMap.GetNumberOfValues();
    viskores::Float32 lerpedScalar;
    lerpedScalar = static_cast<viskores::Float32>((scalar - MinScalar) * InvDeltaScalar);
    viskores::Id colorIndex = viskores::Id(lerpedScalar * viskores::Float32(colorMapSize));
    if (colorIndex < 0)
      colorIndex = 0;
    if (colorIndex >= colorMapSize)
      colorIndex = colorMapSize - 1;
    BOUNDS_CHECK(colorMap, colorIndex);
    viskores::Vec4f_32 sampleColor = colorMap.Get(colorIndex);

    while (enterDistance <= currentDistance && currentDistance <= exitDistance)
    {
      //composite
      viskores::Float32 alpha = sampleColor[3] * (1.f - color[3]);
      color[0] = color[0] + sampleColor[0] * alpha;
      color[1] = color[1] + sampleColor[1] * alpha;
      color[2] = color[2] + sampleColor[2] * alpha;
      color[3] = alpha + color[3];

      currentDistance += SampleDistance;
      if (color[3] >= 1.f || currentDistance >= maxDistance)
      {
        rayStatus = RAY_TERMINATED;
        break;
      }
    }

    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 0);
    frameBuffer.Set(pixelIndex * 4 + 0, color[0]);
    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 1);
    frameBuffer.Set(pixelIndex * 4 + 1, color[1]);
    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 2);
    frameBuffer.Set(pixelIndex * 4 + 2, color[2]);
    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 3);
    frameBuffer.Set(pixelIndex * 4 + 3, color[3]);
  }
}; //class Sample cell

template <typename FloatType>
class SampleCellAssocPoints : public viskores::worklet::WorkletMapField
{
private:
  CellSampler<255> Sampler;
  FloatType SampleDistance;
  FloatType MinScalar;
  FloatType InvDeltaScalar;

public:
  SampleCellAssocPoints(const FloatType& sampleDistance,
                        const FloatType& minScalar,
                        const FloatType& maxScalar)
    : SampleDistance(sampleDistance)
    , MinScalar(minScalar)
  {
    InvDeltaScalar = (minScalar == maxScalar) ? 1.f : 1.f / (maxScalar - minScalar);
  }


  using ControlSignature = void(FieldIn,
                                WholeArrayIn,
                                WholeArrayIn,
                                FieldIn,
                                FieldIn,
                                FieldInOut,
                                FieldIn,
                                FieldInOut,
                                FieldIn,
                                ExecObject meshConnectivity,
                                WholeArrayIn,
                                WholeArrayInOut,
                                FieldIn);
  using ExecutionSignature =
    void(_1, _2, _3, _4, _5, _6, _7, _8, WorkIndex, _9, _10, _11, _12, _13);

  template <typename PointPortalType,
            typename ScalarPortalType,
            typename ColorMapType,
            typename FrameBufferType>
  VISKORES_EXEC inline void operator()(const viskores::Id& currentCell,
                                       PointPortalType& vertices,
                                       ScalarPortalType& scalarPortal,
                                       const FloatType& enterDistance,
                                       const FloatType& exitDistance,
                                       FloatType& currentDistance,
                                       const viskores::Vec3f_32& dir,
                                       viskores::UInt8& rayStatus,
                                       const viskores::Id& pixelIndex,
                                       const viskores::Vec<FloatType, 3>& origin,
                                       MeshConnectivity& meshConn,
                                       const ColorMapType& colorMap,
                                       FrameBufferType& frameBuffer,
                                       const FloatType& maxDistance) const
  {

    if (rayStatus != RAY_ACTIVE)
      return;

    viskores::Vec4f_32 color;
    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 0);
    color[0] = static_cast<viskores::Float32>(frameBuffer.Get(pixelIndex * 4 + 0));
    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 1);
    color[1] = static_cast<viskores::Float32>(frameBuffer.Get(pixelIndex * 4 + 1));
    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 2);
    color[2] = static_cast<viskores::Float32>(frameBuffer.Get(pixelIndex * 4 + 2));
    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 3);
    color[3] = static_cast<viskores::Float32>(frameBuffer.Get(pixelIndex * 4 + 3));

    if (color[3] >= 1.f)
    {
      rayStatus = RAY_TERMINATED;
      return;
    }
    viskores::Vec<viskores::Float32, 8> scalars;
    viskores::Vec<viskores::Vec<FloatType, 3>, 8> points;
    // silence "may" be uninitialized warning
    for (viskores::Int32 i = 0; i < 8; ++i)
    {
      scalars[i] = 0.f;
      points[i] = viskores::Vec<FloatType, 3>(0.f, 0.f, 0.f);
    }
    //load local scalar cell data
    viskores::Id cellConn[8];
    const viskores::Int32 numIndices = meshConn.GetCellIndices(cellConn, currentCell);
    for (int i = 0; i < numIndices; ++i)
    {
      BOUNDS_CHECK(scalarPortal, cellConn[i]);
      scalars[i] = static_cast<viskores::Float32>(scalarPortal.Get(cellConn[i]));
      BOUNDS_CHECK(vertices, cellConn[i]);
      points[i] = viskores::Vec<FloatType, 3>(vertices.Get(cellConn[i]));
    }
    //
    // There can be mismatches in the initial enter distance and the current distance
    // due to lost rays at cell borders. For now,
    // we will just advance the current position to the enter distance, since otherwise,
    // the pixel would never be sampled.
    //
    if (currentDistance < enterDistance)
    {
      currentDistance = enterDistance;
    }

    const viskores::Id colorMapSize = colorMap.GetNumberOfValues();
    const viskores::Int32 cellShape = meshConn.GetCellShape(currentCell);

    while (enterDistance <= currentDistance && currentDistance <= exitDistance)
    {
      viskores::Vec<FloatType, 3> sampleLoc = origin + currentDistance * dir;
      viskores::Float32 lerpedScalar;
      bool validSample = Sampler.SampleCell(points, scalars, sampleLoc, lerpedScalar, cellShape);
      if (!validSample)
      {
        //
        // There is a slight mismatch between intersections and parametric coordinates
        // which results in a invalid sample very close to the cell edge. Just throw
        // this sample away, and move to the next sample.
        //

        //There should be a sample here, so offset and try again.

        currentDistance += 0.00001f;
        continue;
      }
      lerpedScalar = static_cast<viskores::Float32>((lerpedScalar - MinScalar) * InvDeltaScalar);
      viskores::Id colorIndex = viskores::Id(lerpedScalar * viskores::Float32(colorMapSize));

      colorIndex = viskores::Min(viskores::Max(colorIndex, viskores::Id(0)), colorMapSize - 1);
      BOUNDS_CHECK(colorMap, colorIndex);
      viskores::Vec4f_32 sampleColor = colorMap.Get(colorIndex);
      //composite
      sampleColor[3] *= (1.f - color[3]);
      color[0] = color[0] + sampleColor[0] * sampleColor[3];
      color[1] = color[1] + sampleColor[1] * sampleColor[3];
      color[2] = color[2] + sampleColor[2] * sampleColor[3];
      color[3] = sampleColor[3] + color[3];

      currentDistance += SampleDistance;
      if (color[3] >= 1.0 || currentDistance >= maxDistance)
      {
        rayStatus = RAY_TERMINATED;
        break;
      }
    }

    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 0);
    frameBuffer.Set(pixelIndex * 4 + 0, color[0]);
    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 1);
    frameBuffer.Set(pixelIndex * 4 + 1, color[1]);
    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 2);
    frameBuffer.Set(pixelIndex * 4 + 2, color[2]);
    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 3);
    frameBuffer.Set(pixelIndex * 4 + 3, color[3]);
  }
}; //class Sample cell

template <typename FloatType>
void ConnectivityTracer::IntersectCell(Ray<FloatType>& rays,
                                       detail::RayTracking<FloatType>& tracker)
{
  viskores::cont::Timer timer;
  timer.Start();
  viskores::worklet::DispatcherMapField<LocateCell> locateDispatch;
  locateDispatch.Invoke(rays.HitIdx,
                        this->Coords,
                        rays.Dir,
                        *(tracker.EnterDist),
                        *(tracker.ExitDist),
                        tracker.ExitFace,
                        rays.Status,
                        rays.Origin,
                        MeshContainer);

  if (this->CountRayStatus)
    RaysLost = RayOperations::GetStatusCount(rays, RAY_LOST);
  this->IntersectTime += timer.GetElapsedTime();
}

template <typename FloatType>
void ConnectivityTracer::AccumulatePathLengths(Ray<FloatType>& rays,
                                               detail::RayTracking<FloatType>& tracker)
{
  viskores::worklet::DispatcherMapField<AddPathLengths> dispatcher;
  dispatcher.Invoke(
    rays.Status, *(tracker.EnterDist), *(tracker.ExitDist), rays.GetBuffer("path_lengths").Buffer);
}

template <typename FloatType>
void ConnectivityTracer::FindLostRays(Ray<FloatType>& rays, detail::RayTracking<FloatType>& tracker)
{
  viskores::cont::Timer timer;
  timer.Start();

  viskores::worklet::DispatcherMapField<RayBumper> bumpDispatch(RayBumper(this->BumpDistance));
  bumpDispatch.Invoke(rays.HitIdx,
                      this->Coords,
                      *(tracker.EnterDist),
                      *(tracker.ExitDist),
                      tracker.ExitFace,
                      rays.Status,
                      rays.Origin,
                      rays.Dir,
                      MeshContainer,
                      &this->Locator);

  this->LostRayTime += timer.GetElapsedTime();
}

template <typename FloatType>
void ConnectivityTracer::SampleCells(Ray<FloatType>& rays, detail::RayTracking<FloatType>& tracker)
{
  using SampleP = SampleCellAssocPoints<FloatType>;
  using SampleC = SampleCellAssocCells<FloatType>;
  viskores::cont::Timer timer;
  timer.Start();

  VISKORES_ASSERT(rays.Buffers.at(0).GetNumChannels() == 4);

  if (FieldAssocPoints)
  {
    viskores::worklet::DispatcherMapField<SampleP> dispatcher(
      SampleP(this->SampleDistance,
              viskores::Float32(this->ScalarBounds.Min),
              viskores::Float32(this->ScalarBounds.Max)));
    dispatcher.Invoke(rays.HitIdx,
                      this->Coords,
                      viskores::rendering::raytracing::GetScalarFieldArray(this->ScalarField),
                      *(tracker.EnterDist),
                      *(tracker.ExitDist),
                      tracker.CurrentDistance,
                      rays.Dir,
                      rays.Status,
                      rays.Origin,
                      MeshContainer,
                      this->ColorMap,
                      rays.Buffers.at(0).Buffer,
                      rays.MaxDistance);
  }
  else
  {
    viskores::worklet::DispatcherMapField<SampleC> dispatcher(
      SampleC(this->SampleDistance,
              viskores::Float32(this->ScalarBounds.Min),
              viskores::Float32(this->ScalarBounds.Max)));

    dispatcher.Invoke(rays.HitIdx,
                      viskores::rendering::raytracing::GetScalarFieldArray(this->ScalarField),
                      GhostField.GetData().ExtractComponent<viskores::UInt8>(0),
                      *(tracker.EnterDist),
                      *(tracker.ExitDist),
                      tracker.CurrentDistance,
                      rays.Status,
                      this->ColorMap,
                      rays.Buffers.at(0).Buffer,
                      rays.MaxDistance);
  }

  this->SampleTime += timer.GetElapsedTime();
}

template <typename FloatType>
void ConnectivityTracer::IntegrateCells(Ray<FloatType>& rays,
                                        detail::RayTracking<FloatType>& tracker)
{
  viskores::cont::Timer timer;
  timer.Start();
  if (HasEmission)
  {
    bool divideEmisByAbsorp = false;
    viskores::cont::ArrayHandle<FloatType> absorp = rays.Buffers.at(0).Buffer;
    viskores::cont::ArrayHandle<FloatType> emission = rays.GetBuffer("emission").Buffer;
    viskores::worklet::DispatcherMapField<IntegrateEmission> dispatcher(
      IntegrateEmission(rays.Buffers.at(0).GetNumChannels(), UnitScalar, divideEmisByAbsorp));
    dispatcher.Invoke(rays.Status,
                      *(tracker.EnterDist),
                      *(tracker.ExitDist),
                      rays.Distance,
                      viskores::rendering::raytracing::GetScalarFieldArray(this->ScalarField),
                      viskores::rendering::raytracing::GetScalarFieldArray(this->EmissionField),
                      absorp,
                      emission,
                      rays.HitIdx);
  }
  else
  {
    viskores::worklet::DispatcherMapField<Integrate> dispatcher(
      Integrate(rays.Buffers.at(0).GetNumChannels(), UnitScalar));
    dispatcher.Invoke(rays.Status,
                      *(tracker.EnterDist),
                      *(tracker.ExitDist),
                      rays.Distance,
                      viskores::rendering::raytracing::GetScalarFieldArray(this->ScalarField),
                      rays.Buffers.at(0).Buffer,
                      rays.HitIdx);
  }

  IntegrateTime += timer.GetElapsedTime();
}

// template <typename FloatType>
// void ConnectivityTracer<CellType>::PrintDebugRay(Ray<FloatType>& rays, viskores::Id rayId)
// {
//   viskores::Id index = -1;
//   for (viskores::Id i = 0; i < rays.NumRays; ++i)
//   {
//     if (rays.PixelIdx.WritePortal().Get(i) == rayId)
//     {
//       index = i;
//       break;
//     }
//   }
//   if (index == -1)
//   {
//     return;
//   }

//   std::cout << "++++++++RAY " << rayId << "++++++++\n";
//   std::cout << "Status: " << (int)rays.Status.WritePortal().Get(index) << "\n";
//   std::cout << "HitIndex: " << rays.HitIdx.WritePortal().Get(index) << "\n";
//   std::cout << "Dist " << rays.Distance.WritePortal().Get(index) << "\n";
//   std::cout << "MinDist " << rays.MinDistance.WritePortal().Get(index) << "\n";
//   std::cout << "Origin " << rays.Origin.ReadPortal().Get(index) << "\n";
//   std::cout << "Dir " << rays.Dir.ReadPortal().Get(index) << "\n";
//   std::cout << "+++++++++++++++++++++++++\n";
// }

template <typename FloatType>
void ConnectivityTracer::OffsetMinDistances(Ray<FloatType>& rays)
{
  viskores::worklet::DispatcherMapField<AdvanceRay<FloatType>> dispatcher(
    AdvanceRay<FloatType>(FloatType(this->BumpDistance)));
  dispatcher.Invoke(rays.Status, rays.MinDistance);
}

template <typename FloatType>
void ConnectivityTracer::FindMeshEntry(Ray<FloatType>& rays)
{
  viskores::cont::Timer entryTimer;
  entryTimer.Start();
  //
  // if ray misses the external face it will be marked RAY_EXITED_MESH
  //
  MeshContainer->FindEntry(rays);
  MeshEntryTime += entryTimer.GetElapsedTime();
}

template <typename FloatType>
void ConnectivityTracer::IntegrateMeshSegment(Ray<FloatType>& rays)
{
  this->Init(); // sets sample distance
  detail::RayTracking<FloatType> rayTracker;
  rayTracker.Init(rays.NumRays, rays.Distance);

  bool hasPathLengths = rays.HasBuffer("path_lengths");

  if (this->Integrator == Volume)
  {
    viskores::worklet::DispatcherMapField<detail::AdjustSample> adispatcher(SampleDistance);
    adispatcher.Invoke(rays.Status, rayTracker.CurrentDistance);
  }

  while (RayOperations::RaysInMesh(rays))
  {
    //
    // Rays the leave the mesh will be marked as RAYEXITED_MESH
    this->IntersectCell(rays, rayTracker);
    //
    // If the ray was lost due to precision issues, we find it.
    // If it is marked RAY_ABANDONED, then something went wrong.
    //
    this->FindLostRays(rays, rayTracker);
    //
    // integrate along the ray
    //
    if (this->Integrator == Volume)
      this->SampleCells(rays, rayTracker);
    else
      this->IntegrateCells(rays, rayTracker);

    if (hasPathLengths)
    {
      this->AccumulatePathLengths(rays, rayTracker);
    }
    //swap enter and exit distances
    rayTracker.Swap();
    if (this->CountRayStatus)
      this->PrintRayStatus(rays);
  } //for
}

template <typename FloatType>
void ConnectivityTracer::FullTrace(Ray<FloatType>& rays)
{

  this->RaysLost = 0;
  RayOperations::ResetStatus(rays, RAY_EXITED_MESH);

  if (this->CountRayStatus)
  {
    this->PrintRayStatus(rays);
  }

  bool cullMissedRays = true;
  bool workRemaining = true;

  do
  {
    FindMeshEntry(rays);

    if (cullMissedRays)
    {
      viskores::cont::ArrayHandle<UInt8> activeRays;
      activeRays = RayOperations::CompactActiveRays(rays);
      cullMissedRays = false;
    }

    IntegrateMeshSegment(rays);

    workRemaining = RayOperations::RaysProcessed(rays) != rays.NumRays;
    //
    // Ensure that we move the current distance forward some
    // epsilon so we don't re-enter the cell we just left.
    //
    if (workRemaining)
    {
      RayOperations::CopyDistancesToMin(rays);
      this->OffsetMinDistances(rays);
    }
  } while (workRemaining);
}

template <typename FloatType>
std::vector<PartialComposite<FloatType>> ConnectivityTracer::PartialTrace(Ray<FloatType>& rays)
{

  //this->CountRayStatus = true;
  bool hasPathLengths = rays.HasBuffer("path_lengths");
  this->RaysLost = 0;
  RayOperations::ResetStatus(rays, RAY_EXITED_MESH);

  std::vector<PartialComposite<FloatType>> partials;

  if (this->CountRayStatus)
  {
    this->PrintRayStatus(rays);
  }

  bool workRemaining = true;

  do
  {
    FindMeshEntry(rays);

    viskores::cont::ArrayHandle<UInt8> activeRays;
    activeRays = RayOperations::CompactActiveRays(rays);

    if (rays.NumRays == 0)
      break;

    IntegrateMeshSegment(rays);

    PartialComposite<FloatType> partial;
    partial.Buffer = rays.Buffers.at(0).Copy();
    viskores::cont::Algorithm::Copy(rays.Distance, partial.Distances);
    viskores::cont::Algorithm::Copy(rays.PixelIdx, partial.PixelIds);

    if (HasEmission && this->Integrator == Energy)
    {
      partial.Intensities = rays.GetBuffer("emission").Copy();
    }
    if (hasPathLengths)
    {
      partial.PathLengths = rays.GetBuffer("path_lengths").Copy().Buffer;
    }
    partials.push_back(partial);

    // reset buffers
    if (this->Integrator == Volume)
    {
      viskores::cont::ArrayHandle<FloatType> signature;
      signature.Allocate(4);
      signature.WritePortal().Set(0, 0.f);
      signature.WritePortal().Set(1, 0.f);
      signature.WritePortal().Set(2, 0.f);
      signature.WritePortal().Set(3, 0.f);
      rays.Buffers.at(0).InitChannels(signature);
    }
    else
    {
      rays.Buffers.at(0).InitConst(1.f);
      if (HasEmission)
      {
        rays.GetBuffer("emission").InitConst(0.f);
      }
      if (hasPathLengths)
      {
        rays.GetBuffer("path_lengths").InitConst(0.f);
      }
    }

    workRemaining = RayOperations::RaysProcessed(rays) != rays.NumRays;
    //
    // Ensure that we move the current distance forward some
    // epsilon so we don't re-enter the cell we just left.
    //
    if (workRemaining)
    {
      RayOperations::CopyDistancesToMin(rays);
      this->OffsetMinDistances(rays);
    }
  } while (workRemaining);

  return partials;
}

template class detail::RayTracking<viskores::Float32>;
template class detail::RayTracking<viskores::Float64>;

template struct PartialComposite<viskores::Float32>;
template struct PartialComposite<viskores::Float64>;

template void ConnectivityTracer::FullTrace<viskores::Float32>(Ray<viskores::Float32>& rays);

template std::vector<PartialComposite<viskores::Float32>>
ConnectivityTracer::PartialTrace<viskores::Float32>(Ray<viskores::Float32>& rays);

template void ConnectivityTracer::IntegrateMeshSegment<viskores::Float32>(
  Ray<viskores::Float32>& rays);

template void ConnectivityTracer::FindMeshEntry<viskores::Float32>(Ray<viskores::Float32>& rays);

template void ConnectivityTracer::FullTrace<viskores::Float64>(Ray<viskores::Float64>& rays);

template std::vector<PartialComposite<viskores::Float64>>
ConnectivityTracer::PartialTrace<viskores::Float64>(Ray<viskores::Float64>& rays);

template void ConnectivityTracer::IntegrateMeshSegment<viskores::Float64>(
  Ray<viskores::Float64>& rays);

template void ConnectivityTracer::FindMeshEntry<viskores::Float64>(Ray<viskores::Float64>& rays);
}
}
} // namespace viskores::rendering::raytracing
