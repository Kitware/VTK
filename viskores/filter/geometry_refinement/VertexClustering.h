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

#ifndef viskores_filter_geometry_refinement_VertexClustering_h
#define viskores_filter_geometry_refinement_VertexClustering_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/geometry_refinement/viskores_filter_geometry_refinement_export.h>

namespace viskores
{
namespace filter
{
namespace geometry_refinement
{
/// \brief Reduce the number of triangles in a mesh.
///
/// `VertexClustering` is a filter to reduce the number of triangles in a
/// triangle mesh, forming a good approximation to the original geometry. The
/// input must be a `viskores::cont::DataSet` that contains only triangles.
///
/// The general approach of the algorithm is to cluster vertices in a uniform
/// binning of space, accumulating to an average point within each bin. In
/// more detail, the algorithm first gets the bounds of the input poly data.
/// It then breaks this bounding volume into a user-specified number of
/// spatial bins.  It then reads each triangle from the input and hashes its
/// vertices into these bins. Then, if 2 or more vertices of
/// the triangle fall in the same bin, the triangle is dicarded.  If the
/// triangle is not discarded, it adds the triangle to the list of output
/// triangles as a list of vertex identifiers.  (There is one vertex id per
/// bin.)  After all the triangles have been read, the representative vertex
/// for each bin is computed.  This determines the spatial location of the
/// vertices of each of the triangles in the output.
///
/// To use this filter, specify the divisions defining the spatial subdivision
/// in the x, y, and z directions. Compared to algorithms such as
/// vtkQuadricClustering, a significantly higher bin count is recommended as it
/// doesn't increase the computation or memory of the algorithm and will produce
/// significantly better results.
///
class VISKORES_FILTER_GEOMETRY_REFINEMENT_EXPORT VertexClustering : public viskores::filter::Filter
{
public:
  /// @brief Specifies the dimensions of the uniform grid that establishes the bins used for clustering.
  ///
  /// Setting smaller numbers of dimensions produces a smaller output, but with a coarser
  /// representation of the surface.
  VISKORES_CONT void SetNumberOfDivisions(const viskores::Id3& num)
  {
    this->NumberOfDivisions = num;
  }

  /// @copydoc SetNumberOfDivisions
  VISKORES_CONT const viskores::Id3& GetNumberOfDivisions() const
  {
    return this->NumberOfDivisions;
  }

private:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;

  viskores::Id3 NumberOfDivisions = { 256, 256, 256 };
};
} // namespace geometry_refinement
} // namespace filter
} // namespace viskores

#endif // viskores_filter_geometry_refinement_VertexClustering_h
