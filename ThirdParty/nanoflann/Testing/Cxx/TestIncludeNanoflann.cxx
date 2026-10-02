// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtk_nanoflann.h"

#include <array>
#include <cstddef>
#include <cstdlib>
#include <vector>

namespace
{
// Minimal dataset adaptor over a list of 3D points.
struct PointCloud
{
  std::vector<std::array<double, 3>> Points;

  size_t kdtree_get_point_count() const { return this->Points.size(); }
  double kdtree_get_pt(const size_t idx, const size_t dim) const { return this->Points[idx][dim]; }
  template <class BBox>
  bool kdtree_get_bbox(BBox&) const
  {
    return false;
  }
};

using KdTree = nanoflann::KDTreeSingleIndexAdaptor<nanoflann::L2_Simple_Adaptor<double, PointCloud>,
  PointCloud, 3, size_t>;
}

int TestIncludeNanoflann(int /*argc*/, char* /*argv*/[])
{
  // Points on a 4x4x4 lattice with unit spacing.
  PointCloud cloud;
  for (int i = 0; i < 4; ++i)
  {
    for (int j = 0; j < 4; ++j)
    {
      for (int k = 0; k < 4; ++k)
      {
        cloud.Points.push_back(
          { static_cast<double>(i), static_cast<double>(j), static_cast<double>(k) });
      }
    }
  }

  KdTree tree(3, cloud, nanoflann::KDTreeSingleIndexAdaptorParams(10));

  // The closest point to a query near (1, 2, 3) is the lattice point (1, 2, 3).
  const double query[3] = { 1.1, 2.0, 2.9 };
  size_t closest = 0;
  double dist2 = 0.0;
  nanoflann::KNNResultSet<double> knn(1);
  knn.init(&closest, &dist2);
  tree.findNeighbors(knn, query);

  const auto& p = cloud.Points[closest];
  if (p[0] != 1.0 || p[1] != 2.0 || p[2] != 3.0)
  {
    return EXIT_FAILURE;
  }

  // The lattice points within a radius of 1.01 of (1, 1, 1) are itself and its 6 neighbors.
  const double center[3] = { 1.0, 1.0, 1.0 };
  std::vector<nanoflann::ResultItem<size_t, double>> matches;
  const size_t numMatches = tree.radiusSearch(center, 1.01 * 1.01, matches);

  return numMatches == 7 && matches.size() == 7 ? EXIT_SUCCESS : EXIT_FAILURE;
}
