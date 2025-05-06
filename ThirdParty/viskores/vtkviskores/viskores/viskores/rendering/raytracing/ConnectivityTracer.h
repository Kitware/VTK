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
#ifndef viskores_rendering_raytracing_ConnectivityTracer_h
#define viskores_rendering_raytracing_ConnectivityTracer_h

#include <viskores/rendering/viskores_rendering_export.h>

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/CellLocatorGeneral.h>

#include <viskores/rendering/raytracing/MeshConnectivityContainers.h>
#include <viskores/rendering/raytracing/PartialComposite.h>


namespace viskores
{
namespace rendering
{
namespace raytracing
{
namespace detail
{

//forward declare so we can be friends
struct RenderFunctor;

//
//  Ray tracker manages memory and pointer
//  swapping for current cell intersection data
//
template <typename FloatType>
class RayTracking
{
public:
  viskores::cont::ArrayHandle<viskores::Int32> ExitFace;
  viskores::cont::ArrayHandle<FloatType> CurrentDistance;
  viskores::cont::ArrayHandle<FloatType> Distance1;
  viskores::cont::ArrayHandle<FloatType> Distance2;
  viskores::cont::ArrayHandle<FloatType>* EnterDist;
  viskores::cont::ArrayHandle<FloatType>* ExitDist;

  RayTracking()
  {
    EnterDist = &Distance1;
    ExitDist = &Distance2;
  }

  void Compact(viskores::cont::ArrayHandle<FloatType>& compactedDistances,
               viskores::cont::ArrayHandle<UInt8>& masks);

  void Init(const viskores::Id size, viskores::cont::ArrayHandle<FloatType>& distances);

  void Swap();
};

} //namespace detail

/**
 * \brief ConnectivityTracer is volumetric ray tracer for unstructured
 *        grids. Capabilities include volume rendering and integrating
 *        absorption and emission of N energy groups for simulated
 *        radiograhy.
 */
class VISKORES_RENDERING_EXPORT ConnectivityTracer
{
public:
  ConnectivityTracer()
    : MeshContainer(nullptr)
    , BumpEpsilon(1e-3)
    , CountRayStatus(false)
    , UnitScalar(1.f)
  {
  }

  ~ConnectivityTracer()
  {
    if (MeshContainer != nullptr)
    {
      delete MeshContainer;
    }
  }

  enum IntegrationMode
  {
    Volume,
    Energy
  };

  void SetVolumeData(const viskores::cont::Field& scalarField,
                     const viskores::Range& scalarBounds,
                     const viskores::cont::UnknownCellSet& cellSet,
                     const viskores::cont::CoordinateSystem& coords,
                     const viskores::cont::Field& ghostField);

  void SetEnergyData(const viskores::cont::Field& absorption,
                     const viskores::Int32 numBins,
                     const viskores::cont::UnknownCellSet& cellSet,
                     const viskores::cont::CoordinateSystem& coords,
                     const viskores::cont::Field& emission);

  void SetBackgroundColor(const viskores::Vec4f_32& backgroundColor);
  void SetSampleDistance(const viskores::Float32& distance);
  void SetColorMap(const viskores::cont::ArrayHandle<viskores::Vec4f_32>& colorMap);

  MeshConnectivityContainer* GetMeshContainer() { return MeshContainer; }

  void Init();

  void SetDebugOn(bool on) { CountRayStatus = on; }

  void SetUnitScalar(const viskores::Float32 unitScalar) { UnitScalar = unitScalar; }
  void SetEpsilon(const viskores::Float64 epsilon) { BumpEpsilon = epsilon; }


  viskores::Id GetNumberOfMeshCells() const;

  void ResetTimers();
  void LogTimers();

  ///
  /// Traces rays fully through the mesh. Rays can exit and re-enter
  /// multiple times before leaving the domain. This is fast path for
  /// structured meshs or meshes that are not interlocking.
  /// Note: rays will be compacted
  ///
  template <typename FloatType>
  void FullTrace(Ray<FloatType>& rays);

  ///
  /// Integrates rays through the mesh. If rays leave the mesh and
  /// re-enter, then those become two separate partial composites.
  /// This is need to support domain decompositions that are like
  /// puzzle pieces. Note: rays will be compacted
  ///
  template <typename FloatType>
  std::vector<PartialComposite<FloatType>> PartialTrace(Ray<FloatType>& rays);

  ///
  /// Integrates the active rays though the mesh until all rays
  /// have exited.
  ///  Precondition: rays.HitIdx is set to a valid mesh cell
  ///
  template <typename FloatType>
  void IntegrateMeshSegment(Ray<FloatType>& rays);

  ///
  /// Find the entry point in the mesh
  ///
  template <typename FloatType>
  void FindMeshEntry(Ray<FloatType>& rays);

private:
  template <typename FloatType>
  void IntersectCell(Ray<FloatType>& rays, detail::RayTracking<FloatType>& tracker);

  template <typename FloatType>
  void AccumulatePathLengths(Ray<FloatType>& rays, detail::RayTracking<FloatType>& tracker);

  template <typename FloatType>
  void FindLostRays(Ray<FloatType>& rays, detail::RayTracking<FloatType>& tracker);

  template <typename FloatType>
  void SampleCells(Ray<FloatType>& rays, detail::RayTracking<FloatType>& tracker);

  template <typename FloatType>
  void IntegrateCells(Ray<FloatType>& rays, detail::RayTracking<FloatType>& tracker);

  template <typename FloatType>
  void OffsetMinDistances(Ray<FloatType>& rays);

  template <typename FloatType>
  void PrintRayStatus(Ray<FloatType>& rays);

protected:
  // Data set info
  viskores::cont::Field ScalarField;
  viskores::cont::Field EmissionField;
  viskores::cont::Field GhostField;
  viskores::cont::UnknownCellSet CellSet;
  viskores::cont::CoordinateSystem Coords;
  viskores::Range ScalarBounds;
  viskores::Float32 BoundingBox[6];

  viskores::cont::ArrayHandle<viskores::Vec4f_32> ColorMap;

  viskores::Vec4f_32 BackgroundColor;
  viskores::Float32 SampleDistance;
  viskores::Id RaysLost;
  IntegrationMode Integrator;

  MeshConnectivityContainer* MeshContainer;
  viskores::cont::CellLocatorGeneral Locator;
  viskores::Float64 BumpEpsilon;
  viskores::Float64 BumpDistance;
  //
  // flags
  bool CountRayStatus;
  bool MeshConnIsConstructed;
  bool DebugFiltersOn;
  bool ReEnterMesh; // Do not try to re-enter the mesh
  bool CreatePartialComposites;
  bool FieldAssocPoints;
  bool HasEmission; // Mode for integrating through energy bins

  // timers
  viskores::Float64 IntersectTime;
  viskores::Float64 IntegrateTime;
  viskores::Float64 SampleTime;
  viskores::Float64 LostRayTime;
  viskores::Float64 MeshEntryTime;
  viskores::Float32 UnitScalar;

}; // class ConnectivityTracer<CellType,ConnectivityType>
}
}
} // namespace viskores::rendering::raytracing
#endif
