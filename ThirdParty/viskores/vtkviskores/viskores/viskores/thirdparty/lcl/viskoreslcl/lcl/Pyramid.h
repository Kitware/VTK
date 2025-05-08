//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.md for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef lcl_Pyramid_h
#define lcl_Pyramid_h

#include <lcl/ErrorCode.h>
#include <lcl/Shapes.h>

#include <lcl/internal/Common.h>

namespace lcl
{

class Pyramid : public Cell
{
public:
  constexpr LCL_EXEC Pyramid() : Cell(ShapeId::PYRAMID, 5) {}
  constexpr LCL_EXEC explicit Pyramid(const Cell& cell) : Cell(cell) {}
};

LCL_EXEC inline lcl::ErrorCode validate(Pyramid tag) noexcept
{
  if (tag.shape() != ShapeId::PYRAMID)
  {
    return ErrorCode::WRONG_SHAPE_ID_FOR_TAG_TYPE;
  }
  if (tag.numberOfPoints() != 5)
  {
    return ErrorCode::INVALID_NUMBER_OF_POINTS;
  }

  return ErrorCode::SUCCESS;
}

template<typename CoordType>
LCL_EXEC inline lcl::ErrorCode parametricCenter(Pyramid, CoordType&& pcoords) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  component(pcoords, 0) = 0.5f;
  component(pcoords, 1) = 0.5f;
  component(pcoords, 2) = 0.2f;
  return ErrorCode::SUCCESS;
}

template<typename CoordType>
LCL_EXEC inline lcl::ErrorCode parametricPoint(
  Pyramid, IdComponent pointId, CoordType&& pcoords) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  switch (pointId)
  {
    case 0:
      component(pcoords, 0) = 0.0f;
      component(pcoords, 1) = 0.0f;
      component(pcoords, 2) = 0.0f;
      break;
    case 1:
      component(pcoords, 0) = 1.0f;
      component(pcoords, 1) = 0.0f;
      component(pcoords, 2) = 0.0f;
      break;
    case 2:
      component(pcoords, 0) = 1.0f;
      component(pcoords, 1) = 1.0f;
      component(pcoords, 2) = 0.0f;
      break;
    case 3:
      component(pcoords, 0) = 0.0f;
      component(pcoords, 1) = 1.0f;
      component(pcoords, 2) = 0.0f;
      break;
    case 4:
      component(pcoords, 0) = 0.5f;
      component(pcoords, 1) = 0.5f;
      component(pcoords, 2) = 1.0f;
      break;
    default:
      return ErrorCode::INVALID_POINT_ID;
  }

  return ErrorCode::SUCCESS;
}

template<typename CoordType>
LCL_EXEC inline ComponentType<CoordType> parametricDistance(Pyramid, const CoordType& pcoords) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);
  return internal::findParametricDistance(pcoords, 3);
}

template<typename CoordType>
LCL_EXEC inline bool cellInside(Pyramid, const CoordType& pcoords) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  using T = ComponentType<CoordType>;

  constexpr T eps = 0.001f;
  return component(pcoords, 0) >= -eps && component(pcoords, 0) <= (T{1} + eps) &&
         component(pcoords, 1) >= -eps && component(pcoords, 1) <= (T{1} + eps) &&
         component(pcoords, 2) >= -eps && component(pcoords, 2) <= (T{1} + eps);
}

template <typename Values, typename CoordType, typename Result>
LCL_EXEC inline lcl::ErrorCode interpolate(
  Pyramid,
  const Values& values,
  const CoordType& pcoords,
  Result&& result) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  using T = internal::ClosestFloatType<typename Values::ValueType>;

  for (IdComponent c = 0; c < values.getNumberOfComponents(); ++c)
  {
    auto baseV0 = internal::lerp(static_cast<T>(values.getValue(0, c)),
                                 static_cast<T>(values.getValue(1, c)),
                                 static_cast<T>(component(pcoords, 0)));
    auto baseV1 = internal::lerp(static_cast<T>(values.getValue(3, c)),
                                 static_cast<T>(values.getValue(2, c)),
                                 static_cast<T>(component(pcoords, 0)));

    auto baseV = internal::lerp(baseV0, baseV1, static_cast<T>(component(pcoords, 1)));

    auto v = internal::lerp(baseV,
                            static_cast<T>(values.getValue(4, c)),
                            static_cast<T>(component(pcoords, 2)));

    component(result, c) = static_cast<ComponentType<Result>>(v);
  }

  return ErrorCode::SUCCESS;
}

namespace internal
{

template <typename Values, typename CoordType, typename Result>
LCL_EXEC inline void parametricDerivative(Pyramid,
                                           const Values& values,
                                           IdComponent comp,
                                           const CoordType& pcoords,
                                           Result&& result) noexcept
{
  using T = internal::ClosestFloatType<typename Values::ValueType>;
  T p0 = static_cast<T>(component(pcoords, 0));
  T p1 = static_cast<T>(component(pcoords, 1));
  T p2 = static_cast<T>(component(pcoords, 2));
  T rm = T{1} - p0;
  T sm = T{1} - p1;
  T tm = T{1} - p2;

  T dr = (static_cast<T>(values.getValue(0, comp)) * -sm * tm) +
         (static_cast<T>(values.getValue(1, comp)) *  sm * tm) +
         (static_cast<T>(values.getValue(2, comp)) *  p1 * tm) +
         (static_cast<T>(values.getValue(3, comp)) * -p1 * tm);

  T ds = (static_cast<T>(values.getValue(0, comp)) * -rm * tm) +
         (static_cast<T>(values.getValue(1, comp)) * -p0 * tm) +
         (static_cast<T>(values.getValue(2, comp)) *  p0 * tm) +
         (static_cast<T>(values.getValue(3, comp)) *  rm * tm);

  T dt = (static_cast<T>(values.getValue(0, comp)) * -rm * sm) +
         (static_cast<T>(values.getValue(1, comp)) * -p0 * sm) +
         (static_cast<T>(values.getValue(2, comp)) * -p0 * p1) +
         (static_cast<T>(values.getValue(3, comp)) * -rm * p1) +
         (static_cast<T>(values.getValue(4, comp)));

  component(result, 0) = static_cast<ComponentType<Result>>(dr);
  component(result, 1) = static_cast<ComponentType<Result>>(ds);
  component(result, 2) = static_cast<ComponentType<Result>>(dt);
}

} // internal

template <typename Points, typename Values, typename CoordType, typename Result>
LCL_EXEC inline lcl::ErrorCode derivative(
  Pyramid,
  const Points& points,
  const Values& values,
  const CoordType& pcoords,
  Result&& dx,
  Result&& dy,
  Result&& dz) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  using ProcessingType = internal::ClosestFloatType<typename Values::ValueType>;
  using ResultCompType = ComponentType<Result>;

  if (component(pcoords, 2) > ComponentType<CoordType>(.999f))
  {
    // If we are at the apex of the pyramid we need to do something special.
    // As we approach the apex, the derivatives of the parametric shape
    // functions in x and y go to 0 while the inverse of the Jacobian
    // also goes to 0.  This results in 0/0 but using l'Hopital's rule
    // we could actually compute the value of the limit, if we had a
    // functional expression to compute the gradient.  We're on a computer
    // so we don't but we can cheat and do a linear extrapolation of the
    // derivatives which really ends up as the same thing.
    internal::Matrix<ProcessingType, 3, 3> j, ij1, ij2;

    const ComponentType<CoordType> pc1[3] = {0.5f, 0.5f, (2.0f * 0.998f) - component(pcoords, 2)};
    internal::jacobian3D(Pyramid{}, points, pc1, j);
    LCL_RETURN_ON_ERROR(internal::matrixInverse(j, ij1))

    const ComponentType<CoordType> pc2[3] = {0.5f, 0.5f, 0.998f};
    internal::jacobian3D(Pyramid{}, points, pc2, j);
    LCL_RETURN_ON_ERROR(internal::matrixInverse(j, ij2))

    for (IdComponent c = 0; c < values.getNumberOfComponents(); ++c)
    {
      internal::Vector<ProcessingType, 3> dvdp;

      internal::parametricDerivative(Pyramid{}, values, c, pc1, dvdp);
      auto d1 = matrixMultiply(dvdp, ij1);

      internal::parametricDerivative(Pyramid{}, values, c, pc2, dvdp);
      auto d2 = matrixMultiply(dvdp, ij2);

      component(dx, c) = static_cast<ResultCompType>((d2[0] * 2.0f) - d1[0]);
      component(dy, c) = static_cast<ResultCompType>((d2[1] * 2.0f) - d1[1]);
      component(dz, c) = static_cast<ResultCompType>((d2[2] * 2.0f) - d1[2]);
    }
  }
  else
  {
    return internal::derivative3D(Pyramid{},
                                  points,
                                  values,
                                  pcoords,
                                  std::forward<Result>(dx),
                                  std::forward<Result>(dy),
                                  std::forward<Result>(dz));
  }

  return ErrorCode::SUCCESS;
}

template <typename Points, typename PCoordType, typename WCoordType>
LCL_EXEC inline lcl::ErrorCode parametricToWorld(
  Pyramid,
  const Points& points,
  const PCoordType& pcoords,
  WCoordType&& wcoords) noexcept
{
  return interpolate(Pyramid{}, points, pcoords, std::forward<WCoordType>(wcoords));
}

template <typename Points, typename WCoordType, typename PCoordType>
LCL_EXEC inline lcl::ErrorCode worldToParametric(
  Pyramid,
  const Points& points,
  const WCoordType& wcoords,
  PCoordType&& pcoords) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(PCoordType);

  using TIn = typename Points::ValueType;
  using TOut = ComponentType<PCoordType>;

  internal::Vector<TIn, 3> wcVec{component(wcoords, 0), component(wcoords, 1), component(wcoords, 2)};

  // Newton's method fails if the wcoord is too close to the apex. Just return the pcoords at the
  // apex for those cases.
  internal::Vector<TOut, 3> pcBaseCenter(0.5f, 0.5f, 0.0f);
  internal::Vector<TIn, 3> apex, wcBaseCenter;
  points.getTuple(4, apex);
  LCL_RETURN_ON_ERROR(parametricToWorld(Pyramid{}, points, pcBaseCenter, wcBaseCenter))
  auto apexToBase = wcBaseCenter - apex;
  auto apexToWc = wcVec - apex;
  auto dist2ApexToBase = internal::dot(apexToBase, apexToBase);
  auto dist2ApexToWC = internal::dot(apexToWc, apexToWc);
  if (dist2ApexToWC <= (1e-6f * dist2ApexToBase))
  {
    return parametricPoint(Pyramid{}, 4, pcoords);
  }

  return internal::worldToParametric3D(
    Pyramid{}, points, wcoords, std::forward<PCoordType>(pcoords));
}

} // lcl

#endif // lcl_Pyramid_h
