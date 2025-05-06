//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.md for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef lcl_internal_Common_h
#define lcl_internal_Common_h

#include <lcl/FieldAccessor.h>
#include <lcl/internal/Math.h>

#include <cstdint>
#include <type_traits>

#define LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(PCoordType)                   \
  static_assert(std::is_floating_point<ComponentType<PCoordType>>::value,      \
                "parametric coordinates should be of floating point type");

namespace lcl
{
namespace internal
{

///=========================================================================
template<typename CoordType>
LCL_EXEC inline ComponentType<CoordType> findParametricDistance(
  const CoordType& pvals, IdComponent numVals) noexcept
{
  using T = ComponentType<CoordType>;

  T pDistMax = 0.0f;
  for (IdComponent i = 0; i < numVals; ++i)
  {
    ComponentType<CoordType> pDist = 0.0f;
    if (component(pvals, i) < T(0))
    {
      pDist = -1.0f * component(pvals, i);
    }
    else if (component(pvals, i) > T(1))
    {
      pDist = component(pvals, i) - 1.0f;
    }
    if (pDist > pDistMax)
    {
      pDistMax = pDist;
    }
  }

  return pDistMax;
}

///=========================================================================
/// Forward declaration
#define FORWARD_DECLAR_PARAMETRIC_DERIVATIVE(tag)                                                  \
template <typename Values, typename CoordType, typename Result>                                    \
LCL_EXEC inline void parametricDerivative(                                                        \
  lcl::tag, const Values& values, IdComponent comp, const CoordType&, Result&& result) noexcept

FORWARD_DECLAR_PARAMETRIC_DERIVATIVE(Triangle);
FORWARD_DECLAR_PARAMETRIC_DERIVATIVE(Quad);
FORWARD_DECLAR_PARAMETRIC_DERIVATIVE(Tetra);
FORWARD_DECLAR_PARAMETRIC_DERIVATIVE(Hexahedron);
FORWARD_DECLAR_PARAMETRIC_DERIVATIVE(Wedge);
FORWARD_DECLAR_PARAMETRIC_DERIVATIVE(Pyramid);

#undef FORWARD_DECLAR_PARAMETRIC_DERIVATIVE

///=========================================================================
template <typename T>
class Space2D
{
public:
  explicit LCL_EXEC Space2D(const Vector<T, 3>& origin, const Vector<T, 3>& p1, const Vector<T, 3>& p2) noexcept
  {
    this->Origin = origin;
    this->XAxis = p1 - origin;
    auto normal = internal::cross(this->XAxis, p2 - origin);
    this->YAxis = internal::cross(normal, this->XAxis);

    internal::normalize(this->XAxis);
    internal::normalize(this->YAxis);
  }

  LCL_EXEC Vector<T, 2> to2DPoint(Vector<T, 3> pt) const noexcept
  {
    pt -= this->Origin;
    return Vector<T, 2>{ internal::dot(pt, this->XAxis), internal::dot(pt, this->YAxis) };
  }

  LCL_EXEC Vector<T, 3> to3DVec(const Vector<T, 2>& vec) const noexcept
  {
    return (this->XAxis * vec[0]) + (this->YAxis * vec[1]);
  }

private:
  Vector<T, 3> Origin;
  Vector<T, 3> XAxis, YAxis;
};

template <typename CellTag, typename Points, typename PCoords, typename T>
LCL_EXEC inline void jacobian2D(
  CellTag tag, const Points& points, const PCoords& pcoords, Matrix<T, 2, 2>& jacobian) noexcept
{
  T pd[2];
  parametricDerivative(tag, points, 0, pcoords, pd);
  jacobian(0, 0) = pd[0];
  jacobian(0, 1) = pd[1];
  parametricDerivative(tag, points, 1, pcoords, pd);
  jacobian(1, 0) = pd[0];
  jacobian(1, 1) = pd[1];
}

template <typename CellTag, typename Points, typename Values, typename CoordType, typename Result>
LCL_EXEC inline lcl::ErrorCode derivative2D(
  CellTag tag,
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

  constexpr IdComponent numPoints = CellTag{}.numberOfPoints();

  Vector<ProcessingType, 3> pts[numPoints];
  for (int i = 0; i < numPoints; ++i)
  {
    points.getTuple(i, pts[i]);
  }

  // 2-D coordinate system on the cell's plane
  Space2D<ProcessingType> planeSpace(pts[0], pts[1], pts[numPoints - 1]);
  Vector<ProcessingType, 2> pts2d[numPoints];
  for (int i = 0; i < numPoints; ++i)
  {
    pts2d[i] = planeSpace.to2DPoint(pts[i]);
  }

  Matrix<ProcessingType, 2, 2> jacobian;
  jacobian2D(tag, makeFieldAccessorNestedSOA(pts2d, 2), pcoords, jacobian);
  Matrix<ProcessingType, 2, 2> invJacobian;
  LCL_RETURN_ON_ERROR(matrixInverse(jacobian, invJacobian))

  for (IdComponent c = 0; c < values.getNumberOfComponents(); ++c)
  {
    Vector<ProcessingType, 2> dvdp;
    parametricDerivative(tag, values, c, pcoords, dvdp);
    auto d2D = matrixMultiply(dvdp, invJacobian);
    auto d3D = planeSpace.to3DVec(d2D);

    component(dx, c) = static_cast<ResultCompType>(d3D[0]);
    component(dy, c) = static_cast<ResultCompType>(d3D[1]);
    component(dz, c) = static_cast<ResultCompType>(d3D[2]);
  }

  return ErrorCode::SUCCESS;
}

template <typename CellTag, typename Points, typename WCoordType, typename PCoordType>
LCL_EXEC inline lcl::ErrorCode worldToParametric2D(
  CellTag tag,
  const Points& points,
  const WCoordType& wcoords,
  PCoordType&& pcoords) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(PCoordType);

  using TIn = typename Points::ValueType;
  using TOut = ComponentType<PCoordType>;

  constexpr IdComponent numPoints = CellTag{}.numberOfPoints();

  Vector<TIn, 3> pts[numPoints];
  for (int i = 0; i < numPoints; ++i)
  {
    points.getTuple(i, pts[i]);
  }

  // 2-D coordinate system on the cell's plane
  Space2D<TIn> planeSpace(pts[0], pts[1], pts[numPoints - 1]);
  Vector<TIn, 2> pts2d[numPoints];
  for (int i = 0; i < numPoints; ++i)
  {
    pts2d[i] = planeSpace.to2DPoint(pts[i]);
  }

  auto jacobianEvaluator = [&pts2d](const Vector<TOut, 2>& pc, Matrix<TIn, 2, 2>& jacobian) {
    jacobian2D(CellTag{}, makeFieldAccessorNestedSOA(pts2d, 2), pc, jacobian);
    return ErrorCode::SUCCESS;
  };

  auto functionEvaluator = [&points, &planeSpace](const Vector<TOut, 2>& pc, Vector<TIn, 2>& wc) {
    Vector<TIn, 3> wc3(0);
    LCL_RETURN_ON_ERROR(parametricToWorld(CellTag{}, points, pc, wc3))
    wc = planeSpace.to2DPoint(wc3);

    return ErrorCode::SUCCESS;
  };

  Vector<TIn, 3> wcVec{component(wcoords, 0), component(wcoords, 1), component(wcoords, 2)};
  Vector<TOut, 2> pcVec;
  LCL_RETURN_ON_ERROR(parametricCenter(tag, pcVec))
  auto status = newtonsMethod(
    jacobianEvaluator, functionEvaluator, planeSpace.to2DPoint(wcVec), pcVec);

  if (status == ErrorCode::SUCCESS || status == ErrorCode::SOLUTION_DID_NOT_CONVERGE)
  {
    component(pcoords, 0) = pcVec[0];
    component(pcoords, 1) = pcVec[1];
  }

  return status;
}

///=========================================================================
template <typename CellTag, typename Points, typename PCoords, typename T>
LCL_EXEC inline void jacobian3D(
  CellTag tag, const Points& points, const PCoords& pcoords, Matrix<T, 3, 3>& jacobian) noexcept
{
  T pd[3];
  parametricDerivative(tag, points, 0, pcoords, pd);
  jacobian(0, 0) = pd[0];
  jacobian(0, 1) = pd[1];
  jacobian(0, 2) = pd[2];
  parametricDerivative(tag, points, 1, pcoords, pd);
  jacobian(1, 0) = pd[0];
  jacobian(1, 1) = pd[1];
  jacobian(1, 2) = pd[2];
  parametricDerivative(tag, points, 2, pcoords, pd);
  jacobian(2, 0) = pd[0];
  jacobian(2, 1) = pd[1];
  jacobian(2, 2) = pd[2];
}

template <typename CellTag, typename Points, typename Values, typename CoordType, typename Result>
LCL_EXEC inline lcl::ErrorCode derivative3D(
  CellTag tag,
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

  Matrix<ProcessingType, 3, 3> jacobian;
  jacobian3D(tag, points, pcoords, jacobian);
  Matrix<ProcessingType, 3, 3> invJacobian;
  LCL_RETURN_ON_ERROR(matrixInverse(jacobian, invJacobian))

  for (IdComponent c = 0; c < values.getNumberOfComponents(); ++c)
  {
    Vector<ProcessingType, 3> dvdp;
    parametricDerivative(tag, values, c, pcoords, dvdp);
    auto deriv = matrixMultiply(dvdp, invJacobian);
    component(dx, c) = static_cast<ResultCompType>(deriv[0]);
    component(dy, c) = static_cast<ResultCompType>(deriv[1]);
    component(dz, c) = static_cast<ResultCompType>(deriv[2]);
  }

  return ErrorCode::SUCCESS;
}

template <typename CellTag, typename Points, typename WCoordType, typename PCoordType>
LCL_EXEC inline lcl::ErrorCode worldToParametric3D(
  CellTag tag,
  const Points& points,
  const WCoordType& wcoords,
  PCoordType&& pcoords) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(PCoordType);

  using TIn = typename Points::ValueType;
  using TOut = ComponentType<PCoordType>;

  auto jacobianEvaluator = [tag, &points](const Vector<TOut, 3>& pc, Matrix<TIn, 3, 3>& jacobian) {
    jacobian3D(tag, points, pc, jacobian);
    return ErrorCode::SUCCESS;
  };

  auto functionEvaluator = [tag, &points](const Vector<TOut, 3>& pc, Vector<TIn, 3>& wc) {
    return parametricToWorld(tag, points, pc, wc);
  };

  internal::Vector<TIn, 3> wcVec{component(wcoords, 0), component(wcoords, 1), component(wcoords, 2)};
  internal::Vector<TOut, 3> pcVec;
  LCL_RETURN_ON_ERROR(parametricCenter(tag, pcVec))
  auto status = newtonsMethod(jacobianEvaluator, functionEvaluator, wcVec, pcVec);

  if (status == ErrorCode::SUCCESS || status == ErrorCode::SOLUTION_DID_NOT_CONVERGE)
  {
    component(pcoords, 0) = pcVec[0];
    component(pcoords, 1) = pcVec[1];
    component(pcoords, 2) = pcVec[2];
  }

  return status;
}

}
} // lcl::internal

#endif //lcl_internal_Common_h
