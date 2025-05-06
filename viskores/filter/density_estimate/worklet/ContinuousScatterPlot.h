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

#ifndef viskores_worklet_ContinuousScatterPlot_h
#define viskores_worklet_ContinuousScatterPlot_h

#include <viskores/VectorAnalysis.h>
#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandleGroupVec.h>
#include <viskores/cont/ArrayHandleGroupVecVariable.h>
#include <viskores/cont/ConvertNumComponentsToOffsets.h>
#include <viskores/cont/Invoker.h>
#include <viskores/worklet/ScatterCounting.h>
#include <viskores/worklet/WorkletMapTopology.h>

namespace viskores
{
namespace worklet
{
class ContinuousScatterPlot
{
public:
  template <typename FieldType>
  struct ClassifyTetra : viskores::worklet::WorkletVisitCellsWithPoints
  {
    using ControlSignature = void(CellSetIn,
                                  FieldInPoint scalars,
                                  FieldOutCell pointsOrder,
                                  FieldOutCell numberOfPoints,
                                  FieldOutCell numberOfTris);
    using ExecutionSignature = void(_2 scalar, _3 pointsOrder, _4 numberOfPoints, _5 numberOfTris);
    using InputDomain = _1;

    VISKORES_EXEC FieldType ZCrossProduct(const viskores::Pair<FieldType, FieldType>& point1,
                                          const viskores::Pair<FieldType, FieldType>& point2,
                                          const viskores::Pair<FieldType, FieldType>& point3) const
    {
      return viskores::DifferenceOfProducts(point2.first - point1.first,
                                            point3.second - point1.second,
                                            point2.second - point1.second,
                                            point3.first - point1.first);
    }

    VISKORES_EXEC bool DifferentSign(const FieldType& value1, const FieldType& value2) const
    {
      return (viskores::IsNegative(value1) != viskores::IsNegative(value2));
    }

    VISKORES_EXEC bool AllSameSign(const FieldType& value1,
                                   const FieldType& value2,
                                   const FieldType& value3) const
    {
      return (!(DifferentSign(value1, value2) || DifferentSign(value2, value3)));
    }

    template <typename ScalarField, typename PointsOutOrder>
    VISKORES_EXEC void operator()(const ScalarField& scalar,
                                  PointsOutOrder& pointsOrder,
                                  viskores::IdComponent& numberOfPoints,
                                  viskores::IdComponent& numberOfTris) const
    {
      // To classify our tetras following their projection in the 2D scalar domain,
      // We consider them as quads, with their coordinates being their respective scalar values.

      // To identify the projection, we want to know if the polygon formed by the 4 points of the quad is convex.
      // For this, we compute the Z component of the cross product of the vectors of the polygon's edges.
      viskores::Vec<FieldType, 4> scalarCrossProduct{
        ZCrossProduct(scalar[0], scalar[1], scalar[2]),
        ZCrossProduct(scalar[1], scalar[2], scalar[3]),
        ZCrossProduct(scalar[2], scalar[3], scalar[0]),
        ZCrossProduct(scalar[3], scalar[0], scalar[1])
      };

      // If every cross product of consecutive edges of the quad is the same sign, it means that it is convex
      // In the case 2 of them are negative and 2 positive, the quad is self-intersecting.
      // If one or 3 of them are negative, we have found a non-convex quad, projecting 3 triangles.
      if ((DifferentSign(scalarCrossProduct[0], scalarCrossProduct[1]) !=
           DifferentSign(scalarCrossProduct[2], scalarCrossProduct[3])))
      {
        numberOfPoints = 4;
        numberOfTris = 3;

        // Here, one of the 4 points is in the triangle formed by the 3 other.
        // We can guess which one it is by analyzing which cross product has a different sign than the other 3.
        // Assign this specific point's id to element 3 of our order array.
        if (AllSameSign(scalarCrossProduct[1], scalarCrossProduct[2], scalarCrossProduct[3]))
        {
          pointsOrder[0] = 0;
          pointsOrder[1] = 2;
          pointsOrder[2] = 3;
          pointsOrder[3] = 1;
        }
        else if (AllSameSign(scalarCrossProduct[0], scalarCrossProduct[2], scalarCrossProduct[3]))
        {
          pointsOrder[0] = 0;
          pointsOrder[1] = 1;
          pointsOrder[2] = 3;
          pointsOrder[3] = 2;
        }
        else if (AllSameSign(scalarCrossProduct[0], scalarCrossProduct[1], scalarCrossProduct[3]))
        {
          pointsOrder[0] = 0;
          pointsOrder[1] = 1;
          pointsOrder[2] = 2;
          pointsOrder[3] = 3;
        }
        else
        {
          pointsOrder[0] = 1;
          pointsOrder[1] = 2;
          pointsOrder[2] = 3;
          pointsOrder[3] = 0;
        }
      }
      else
      {
        // This tetra projection yields 4 triangles,
        // And forms a convex quad in the data plane
        numberOfPoints = 5;
        numberOfTris = 4;

        // Find an order of points which forms a non self-intersecting quad,
        // Still using the Z cross-product signs
        if (DifferentSign(scalarCrossProduct[0], scalarCrossProduct[1]))
        {
          // Polygon Self-intersects on the second diagonal
          pointsOrder[0] = 0;
          pointsOrder[1] = 2;
          pointsOrder[2] = 3;
          pointsOrder[3] = 1;
        }
        else if (DifferentSign(scalarCrossProduct[1], scalarCrossProduct[2]))
        {
          // Self-intersect on the first diagonal
          pointsOrder[0] = 0;
          pointsOrder[1] = 2;
          pointsOrder[2] = 1;
          pointsOrder[3] = 3;
        }
        else
        {
          // Convex
          pointsOrder[0] = 0;
          pointsOrder[1] = 1;
          pointsOrder[2] = 2;
          pointsOrder[3] = 3;
        }
      }
    }
  };

  template <typename FieldType>
  struct VolumeMeasure : viskores::worklet::WorkletVisitCellsWithPoints
  {
    using ControlSignature = void(CellSetIn,
                                  FieldInPoint scalars,
                                  FieldInPoint coords,
                                  FieldInCell numberOfTris,
                                  FieldInCell ord,
                                  FieldOutCell newCoords,
                                  FieldOutCell density);
    using ExecutionSignature =
      void(_2 scalars, _3 coords, _4 numberOfTris, _5 ord, _6 newCoords, _7 density);
    using InputDomain = _1;
    using Vec3t = viskores::Vec<FieldType, 3>;

    template <typename VecType>
    VISKORES_EXEC Vec3t DiagonalIntersection(const VecType& point1,
                                             const VecType& point2,
                                             const VecType& point3,
                                             const VecType& point4) const
    {
      FieldType denominator = viskores::DifferenceOfProducts(
        point1[0] - point2[0], point3[1] - point4[1], point1[1] - point2[1], point3[0] - point4[0]);

      // In case the points are aligned, return arbitrarily the first point as intersection.
      // These points represent the diagonals of a convex polygon,
      // so they are either intersecting or lying on the same line
      // The surface area of the quad in the data domain will be null in this case anyways.
      if (denominator == 0)
      {
        return point1;
      }

      // Otherwise, compute the intersection point of the quad's diagonals, defined by 4 points.
      // This vector is the point we get when equating the line equations (point1,point2) and (point3,point4)
      return Vec3t{ viskores::DifferenceOfProducts(
                      point3[0] - point4[0],
                      viskores::DifferenceOfProducts(point1[0], point2[1], point1[1], point2[0]),
                      point1[0] - point2[0],
                      viskores::DifferenceOfProducts(point3[0], point4[1], point3[1], point4[0])) /
                      denominator,
                    viskores::DifferenceOfProducts(
                      point3[1] - point4[1],
                      viskores::DifferenceOfProducts(point1[0], point2[1], point1[1], point2[0]),
                      point1[1] - point2[1],
                      viskores::DifferenceOfProducts(point3[0], point4[1], point3[1], point4[0])) /
                      denominator,
                    0.0f };
    }

    VISKORES_EXEC FieldType TriangleArea(Vec3t point1, Vec3t point2, Vec3t point3) const
    {
      return 0.5f * viskores::Magnitude(viskores::Cross(point2 - point1, point3 - point1));
    }

    VISKORES_EXEC FieldType Distance(Vec3t point1, Vec3t point2) const
    {
      return viskores::Magnitude(point1 - point2);
    }


    template <typename ScalarField,
              typename CoordsType,
              typename NewCoordsType,
              typename PointsOutOrder,
              typename DensityField>
    VISKORES_EXEC void operator()(const ScalarField& scalar,
                                  const CoordsType& coords,
                                  const viskores::IdComponent& numberOfTris,
                                  PointsOutOrder& ord,
                                  NewCoordsType& newCoords,
                                  DensityField& density) const
    {
      // Write points coordinates in the 2D plane based on provided scalar values
      for (int i = 0; i < 4; i++)
      {
        newCoords[i] = Vec3t{ scalar[i].first, scalar[i].second, 0.0f };
      }

      if (numberOfTris == 4)
      {
        // If the projection has 4 triangles, the fifth imaginary point is defined as the intersection of the quad's diagonals.
        newCoords[4] = DiagonalIntersection(
          newCoords[ord[0]], newCoords[ord[2]], newCoords[ord[1]], newCoords[ord[3]]);
      }

      // Compute densities

      // The density on the borders of the data domain is always null.
      // For each tetra projection the only density > 0 is associated either to the point
      // located inside the triangle formed by the others (for 3 triangles projection),
      // Or to the imaginary point calculated above.

      density[ord[0]] = 0.0f;
      density[ord[1]] = 0.0f;
      density[ord[2]] = 0.0f;
      density[ord[3]] = 0.0f;

      // Pre-compute some vectors to reuse later
      viskores::Vec<Vec3t, 3> spatialVector = { coords[1] - coords[0],
                                                coords[2] - coords[0],
                                                coords[3] - coords[0] };
      viskores::Vec<Vec3t, 3> spatialCrossProducts = {
        viskores::Cross(spatialVector[1], spatialVector[0]),
        viskores::Cross(spatialVector[0], spatialVector[2]),
        viskores::Cross(spatialVector[2], spatialVector[1])
      };
      viskores::Vec<Vec3t, 2> scalarArray = { Vec3t{ scalar[1].first - scalar[0].first,
                                                     scalar[2].first - scalar[0].first,
                                                     scalar[3].first - scalar[0].first },
                                              Vec3t{
                                                Vec3t{ scalar[1].second - scalar[0].second,
                                                       scalar[2].second - scalar[0].second,
                                                       scalar[3].second - scalar[0].second } } };

      // We need to calculate the determinant in the spatial domain, using the triple product formula
      FieldType determinant = viskores::Dot(spatialVector[2], spatialCrossProducts[0]);

      // Calculate the spatial gradient for both scalar fields in the tetrahedron
      viskores::Vec<Vec3t, 2> scalar_gradient;

      // The determinant is null, therefore the volume is null
      if (determinant == 0.0f)
      {
        scalar_gradient = { Vec3t{ 0.0f, 0.0f, 0.0f }, Vec3t{ 0.0f, 0.0f, 0.0f } };
      }
      else
      {
        // This gradient formulation is derived from the calculation of the inverse Jacobian matrix,
        // dividing the adjugate matrix of the transformation by the determinant.

        // Each column of the matrix is then multiplied by the scalar difference
        // with the scalar value for point with index 0 and summed to get the gradient, for each scalar field.
        FieldType inv_determinant = 1.0f / determinant;
        scalar_gradient =
          viskores::Vec<Vec3t, 2>{ inv_determinant *
                                     (spatialCrossProducts[0] * scalarArray[0][2] +
                                      spatialCrossProducts[1] * scalarArray[0][1] +
                                      spatialCrossProducts[2] * scalarArray[0][0]),
                                   inv_determinant *
                                     (spatialCrossProducts[0] * scalarArray[1][2] +
                                      spatialCrossProducts[1] * scalarArray[1][1] +
                                      spatialCrossProducts[2] * scalarArray[1][0]) };
      }

      // We get the volume measure, defined as the magnitude of the cross product of the gradient
      // See formula (10) in the "continuous scatterplots" paper for the demonstration
      FieldType volume =
        viskores::Magnitude(viskores::Cross(scalar_gradient[0], scalar_gradient[1]));

      if (numberOfTris == 3)
      {
        // Calculate the area of the triangle on the backface of the projected tetra
        FieldType fullArea =
          this->TriangleArea(newCoords[ord[0]], newCoords[ord[1]], newCoords[ord[2]]);

        if (volume == 0.0f || fullArea == 0.0f)
        {
          // For a tetrahedra of null volume, the density is infinite
          density[ord[3]] = viskores::Infinity<FieldType>();
          return;
        }

        // The density for the central point is the distance to the backface divided by the volume
        // We interpolate the position of point [3] using the scalar values of the other points
        Vec3t contribs{
          this->TriangleArea(newCoords[ord[1]], newCoords[ord[2]], newCoords[ord[3]]) / fullArea,
          this->TriangleArea(newCoords[ord[0]], newCoords[ord[2]], newCoords[ord[3]]) / fullArea,
          this->TriangleArea(newCoords[ord[0]], newCoords[ord[1]], newCoords[ord[3]]) / fullArea
        };

        Vec3t backfaceProjectionInterpolated = contribs[0] * coords[ord[0]] +
          contribs[1] * coords[ord[1]] + contribs[2] * coords[ord[2]];

        density[ord[3]] = this->Distance(coords[ord[3]], backfaceProjectionInterpolated) / volume;
      }
      else
      {
        // 4 triangles projection

        FieldType distanceToIntersection1 = this->Distance(newCoords[4], newCoords[ord[0]]);
        FieldType diagonalLength1 = this->Distance(newCoords[ord[2]], newCoords[ord[0]]);

        FieldType distanceToIntersection2 = this->Distance(newCoords[4], newCoords[ord[1]]);
        FieldType diagonalLength2 = this->Distance(newCoords[ord[1]], newCoords[ord[3]]);

        // Spatial volume is null, or data domain surface is null
        if (volume == 0.0f || diagonalLength1 == 0.0f || diagonalLength2 == 0.0f)
        {
          density[4] = viskores::Infinity<FieldType>();
          return;
        }

        // Interpolate the intersection of diagonals to get its scalar values
        FieldType interpolateRatio1 = distanceToIntersection1 / diagonalLength1;
        FieldType interpolateRatio2 = distanceToIntersection2 / diagonalLength2;

        Vec3t interpolatedPos1 =
          coords[ord[0]] + (coords[ord[2]] - coords[ord[0]]) * interpolateRatio1;
        Vec3t interpolatedPos2 =
          coords[ord[1]] + (coords[ord[3]] - coords[ord[1]]) * interpolateRatio2;

        // Eventually, the density is calculated by dividing the scalar mass density by the volume of the cell
        density[4] = this->Distance(interpolatedPos1, interpolatedPos2) / volume;
      }
    }
  };

  struct ComputeTriangles : viskores::worklet::WorkletVisitCellsWithPoints
  {
    using ControlSignature = void(CellSetIn,
                                  FieldInCell pointsOrder,
                                  FieldInCell numberOfTris,
                                  FieldInCell offsets,
                                  FieldOutCell connectivity);
    using ExecutionSignature =
      void(_2 pointsOrder, _3 numberOfTris, _4 offsets, _5 connectivity, VisitIndex, InputIndex);
    using InputDomain = _1;

    using ScatterType = viskores::worklet::ScatterCounting;

    template <typename OrderType, typename ConnectivityType>
    VISKORES_EXEC void operator()(const OrderType& order,
                                  const viskores::IdComponent& numberOfTris,
                                  const viskores::Id& offsets,
                                  ConnectivityType& connectivity,
                                  const viskores::Id& visitIndex,
                                  const viskores::Id& cellId) const
    {
      viskores::Id secondPoint, thirdPoint;

      if (numberOfTris == 3)
      {
        secondPoint = order[(visitIndex + 1) % 3]; // 1,2,0
        // The one point in the triangle formed by the 3 other is the common vertex to all 3 visible triangles,
        // And has index 3 in the order array
        thirdPoint = order[3];
      }
      else
      {
        secondPoint = order[(visitIndex + 1) % 4]; // 1,2,3,0
        thirdPoint =
          4; // Imaginary point at the intersection of diagonals, connected to every triangle
      }

      connectivity[0] = cellId + offsets + order[static_cast<viskores::IdComponent>(visitIndex)];
      connectivity[1] = cellId + offsets + secondPoint;
      connectivity[2] = cellId + offsets + thirdPoint;
    }
  };

  template <typename CoordsComType,
            typename CoordsComTypeOut,
            typename CoordsInStorageType,
            typename OutputCellSetType,
            typename CoordsOutStorageType,
            typename FieldType>
  void Run(
    const viskores::cont::CellSetSingleType<>& inputCellSet,
    const viskores::cont::ArrayHandle<viskores::Vec<CoordsComType, 3>, CoordsInStorageType>& coords,
    viskores::cont::ArrayHandle<viskores::Vec<CoordsComTypeOut, 3>, CoordsOutStorageType>&
      newCoords,
    viskores::cont::ArrayHandle<FieldType>& density,
    const viskores::cont::ArrayHandle<FieldType>& field1,
    const viskores::cont::ArrayHandle<FieldType>& field2,
    OutputCellSetType& outputCellset)
  {
    viskores::cont::Invoker invoke;

    // Use zip to pass both scalar fields to worklets as a single argument
    auto scalars = viskores::cont::make_ArrayHandleZip(field1, field2);

    // We want to project every tetrahedron in the 2-dimensional data domain using its scalar values,
    // following the tetra projection algorithm
    // (see "A polygonal approximation to direct scalar volume rendering" by Shirley and Tuchman)

    // Minus degenerate cases, this projection makes 3 or 4 triangles in the 2D plane
    // This first worklet generates the number of points and triangles needed to project a tetrahedron,
    // And the order in which to take them to build the cells
    viskores::cont::ArrayHandle<viskores::Vec<viskores::IdComponent, 4>> pointsOrder;
    viskores::cont::ArrayHandle<viskores::IdComponent> numberOfPoints;
    viskores::cont::ArrayHandle<viskores::IdComponent> numberOfTris;
    invoke(
      ClassifyTetra<FieldType>{}, inputCellSet, scalars, pointsOrder, numberOfPoints, numberOfTris);

    viskores::Id totalPoints;
    viskores::cont::ArrayHandle<viskores::Id> offsets =
      viskores::cont::ConvertNumComponentsToOffsets(numberOfPoints, totalPoints);

    // Now, compute the tetra's coordinates in the data plane,
    // and the density of each projected point
    viskores::cont::ArrayHandle<FieldType> volumeMeasure;
    newCoords.Allocate(totalPoints);
    density.Allocate(totalPoints);
    invoke(VolumeMeasure<FieldType>{},
           inputCellSet,
           scalars,
           coords,
           numberOfTris,
           pointsOrder,
           viskores::cont::make_ArrayHandleGroupVecVariable(newCoords, offsets),
           viskores::cont::make_ArrayHandleGroupVecVariable(density, offsets));

    // Finally, write triangle connectivity in the data domain
    viskores::worklet::ScatterCounting scatter(numberOfTris, true);
    viskores::cont::ArrayHandle<viskores::Id> offsets_connectivity = scatter.GetInputToOutputMap();
    viskores::cont::ArrayHandle<viskores::Id> outConnectivity;
    invoke(ComputeTriangles{},
           scatter,
           inputCellSet,
           pointsOrder,
           numberOfTris,
           offsets_connectivity,
           viskores::cont::make_ArrayHandleGroupVec<3>(outConnectivity));

    // Create the new dataset
    outputCellset.Fill(totalPoints, viskores::CellShapeTagTriangle::Id, 3, outConnectivity);
  }
};
} // namespace worklet
} // namespace viskores

#endif // viskores_worklet_ContinuousScatterPlot_h
