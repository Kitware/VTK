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
#ifndef viskores_rendering_raytracing_CellSampler_h
#define viskores_rendering_raytracing_CellSampler_h

#include <viskores/VecVariable.h>
#include <viskores/exec/CellInterpolate.h>
#include <viskores/exec/ParametricCoordinates.h>

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
template <typename CellTag>
VISKORES_EXEC_CONT inline viskores::Int32 GetNumberOfPoints(CellTag tag);

template <>
VISKORES_EXEC_CONT inline viskores::Int32 GetNumberOfPoints<viskores::CellShapeTagHexahedron>(
  viskores::CellShapeTagHexahedron viskoresNotUsed(tag))
{
  return 8;
}

template <>
VISKORES_EXEC_CONT inline viskores::Int32 GetNumberOfPoints<viskores::CellShapeTagTetra>(
  viskores::CellShapeTagTetra viskoresNotUsed(tag))
{
  return 4;
}

template <>
VISKORES_EXEC_CONT inline viskores::Int32 GetNumberOfPoints<viskores::CellShapeTagWedge>(
  viskores::CellShapeTagWedge viskoresNotUsed(tag))
{
  return 6;
}

template <>
VISKORES_EXEC_CONT inline viskores::Int32 GetNumberOfPoints<viskores::CellShapeTagPyramid>(
  viskores::CellShapeTagPyramid viskoresNotUsed(tag))
{
  return 5;
}

template <typename P, typename S, typename CellShapeTagType>
VISKORES_EXEC_CONT inline bool Sample(const viskores::Vec<viskores::Vec<P, 3>, 8>& points,
                                      const viskores::Vec<S, 8>& scalars,
                                      const viskores::Vec<P, 3>& sampleLocation,
                                      S& lerpedScalar,
                                      const CellShapeTagType& shapeTag)
{

  bool validSample = true;
  viskores::VecVariable<viskores::Vec<P, 3>, 8> pointsVec;
  viskores::VecVariable<S, 8> scalarVec;
  for (viskores::Int32 i = 0; i < GetNumberOfPoints(shapeTag); ++i)
  {
    pointsVec.Append(points[i]);
    scalarVec.Append(scalars[i]);
  }
  viskores::Vec<P, 3> pcoords;
  viskores::exec::WorldCoordinatesToParametricCoordinates(
    pointsVec, sampleLocation, shapeTag, pcoords);
  P pmin, pmax;
  pmin = viskores::Min(viskores::Min(pcoords[0], pcoords[1]), pcoords[2]);
  pmax = viskores::Max(viskores::Max(pcoords[0], pcoords[1]), pcoords[2]);
  if (pmin < 0.f || pmax > 1.f)
  {
    validSample = false;
  }
  viskores::exec::CellInterpolate(scalarVec, pcoords, shapeTag, lerpedScalar);
  return validSample;
}

template <typename S, typename P, typename CellShapeTagType>
VISKORES_EXEC_CONT inline bool Sample(const viskores::VecAxisAlignedPointCoordinates<3>& points,
                                      const viskores::Vec<S, 8>& scalars,
                                      const viskores::Vec<P, 3>& sampleLocation,
                                      S& lerpedScalar,
                                      const CellShapeTagType& viskoresNotUsed(shapeTag))
{

  bool validSample = true;
  viskores::Vec<P, 3> pcoords;
  viskores::exec::WorldCoordinatesToParametricCoordinates(
    points, sampleLocation, viskores::CellShapeTagHexahedron(), pcoords);
  P pmin, pmax;
  pmin = viskores::Min(viskores::Min(pcoords[0], pcoords[1]), pcoords[2]);
  pmax = viskores::Max(viskores::Max(pcoords[0], pcoords[1]), pcoords[2]);
  if (pmin < 0.f || pmax > 1.f)
  {
    validSample = false;
  }
  viskores::exec::CellInterpolate(
    scalars, pcoords, viskores::CellShapeTagHexahedron(), lerpedScalar);
  return validSample;
}
} // namespace detail

//
//  General Template: returns false if sample location is outside the cell
//
template <int CellType>
class CellSampler
{
public:
  template <typename P, typename S>
  VISKORES_EXEC_CONT inline bool SampleCell(
    const viskores::Vec<viskores::Vec<P, 3>, 8>& viskoresNotUsed(points),
    const viskores::Vec<S, 8>& viskoresNotUsed(scalars),
    const viskores::Vec<P, 3>& viskoresNotUsed(sampleLocation),
    S& viskoresNotUsed(lerpedScalar),
    const viskores::Int32& viskoresNotUsed(cellShape = CellType)) const
  {
    static_assert(CellType != CELL_SHAPE_ZOO && CellType != CELL_SHAPE_STRUCTURED &&
                    CellType != CELL_SHAPE_HEXAHEDRON && CellType != CELL_SHAPE_TETRA &&
                    CellType != CELL_SHAPE_WEDGE && CellType != CELL_SHAPE_PYRAMID,
                  "Cell Sampler: Default template. This should not happen.\n");
    return false;
  }
};

//
// Zoo Sampler
//
template <>
class CellSampler<255>
{
public:
  template <typename P, typename S>
  VISKORES_EXEC_CONT inline bool SampleCell(const viskores::Vec<viskores::Vec<P, 3>, 8>& points,
                                            const viskores::Vec<S, 8>& scalars,
                                            const viskores::Vec<P, 3>& sampleLocation,
                                            S& lerpedScalar,
                                            const viskores::Int32& cellShape) const
  {
    bool valid = false;
    if (cellShape == CELL_SHAPE_HEXAHEDRON)
    {
      valid = detail::Sample(
        points, scalars, sampleLocation, lerpedScalar, viskores::CellShapeTagHexahedron());
    }

    if (cellShape == CELL_SHAPE_TETRA)
    {
      valid = detail::Sample(
        points, scalars, sampleLocation, lerpedScalar, viskores::CellShapeTagTetra());
    }

    if (cellShape == CELL_SHAPE_WEDGE)
    {
      valid = detail::Sample(
        points, scalars, sampleLocation, lerpedScalar, viskores::CellShapeTagWedge());
    }
    if (cellShape == CELL_SHAPE_PYRAMID)
    {
      valid = detail::Sample(
        points, scalars, sampleLocation, lerpedScalar, viskores::CellShapeTagPyramid());
    }
    return valid;
  }
};

//
//  Single type hex
//
template <>
class CellSampler<CELL_SHAPE_HEXAHEDRON>
{
public:
  template <typename P, typename S>
  VISKORES_EXEC_CONT inline bool SampleCell(
    const viskores::Vec<viskores::Vec<P, 3>, 8>& points,
    const viskores::Vec<S, 8>& scalars,
    const viskores::Vec<P, 3>& sampleLocation,
    S& lerpedScalar,
    const viskores::Int32& viskoresNotUsed(cellShape = CELL_SHAPE_HEXAHEDRON)) const
  {
    return detail::Sample(
      points, scalars, sampleLocation, lerpedScalar, viskores::CellShapeTagHexahedron());
  }
};

//
//  Single type hex uniform and rectilinear
//  calls fast path for sampling
//
template <>
class CellSampler<CELL_SHAPE_STRUCTURED>
{
public:
  template <typename P, typename S>
  VISKORES_EXEC_CONT inline bool SampleCell(
    const viskores::Vec<viskores::Vec<P, 3>, 8>& points,
    const viskores::Vec<S, 8>& scalars,
    const viskores::Vec<P, 3>& sampleLocation,
    S& lerpedScalar,
    const viskores::Int32& viskoresNotUsed(cellShape = CELL_SHAPE_HEXAHEDRON)) const
  {
    viskores::VecAxisAlignedPointCoordinates<3> rPoints(points[0], points[6] - points[0]);
    return detail::Sample(
      rPoints, scalars, sampleLocation, lerpedScalar, viskores::CellShapeTagHexahedron());
  }
};

//
//  Single type pyramid
//
template <>
class CellSampler<CELL_SHAPE_PYRAMID>
{
public:
  template <typename P, typename S>
  VISKORES_EXEC_CONT inline bool SampleCell(
    const viskores::Vec<viskores::Vec<P, 3>, 8>& points,
    const viskores::Vec<S, 8>& scalars,
    const viskores::Vec<P, 3>& sampleLocation,
    S& lerpedScalar,
    const viskores::Int32& viskoresNotUsed(cellShape = CELL_SHAPE_PYRAMID)) const
  {
    return detail::Sample(
      points, scalars, sampleLocation, lerpedScalar, viskores::CellShapeTagPyramid());
  }
};


//
//  Single type Tet
//
template <>
class CellSampler<CELL_SHAPE_TETRA>
{
public:
  template <typename P, typename S>
  VISKORES_EXEC_CONT inline bool SampleCell(
    const viskores::Vec<viskores::Vec<P, 3>, 8>& points,
    const viskores::Vec<S, 8>& scalars,
    const viskores::Vec<P, 3>& sampleLocation,
    S& lerpedScalar,
    const viskores::Int32& viskoresNotUsed(cellShape = CELL_SHAPE_TETRA)) const
  {
    return detail::Sample(
      points, scalars, sampleLocation, lerpedScalar, viskores::CellShapeTagTetra());
  }
};

//
//  Single type Wedge
//
template <>
class CellSampler<CELL_SHAPE_WEDGE>
{
public:
  template <typename P, typename S>
  VISKORES_EXEC_CONT inline bool SampleCell(
    const viskores::Vec<viskores::Vec<P, 3>, 8>& points,
    const viskores::Vec<S, 8>& scalars,
    const viskores::Vec<P, 3>& sampleLocation,
    S& lerpedScalar,
    const viskores::Int32& viskoresNotUsed(cellShape = CELL_SHAPE_WEDGE)) const
  {
    return detail::Sample(
      points, scalars, sampleLocation, lerpedScalar, viskores::CellShapeTagWedge());
  }
};
}
}
} // namespace viskores::rendering::raytracing
#endif
