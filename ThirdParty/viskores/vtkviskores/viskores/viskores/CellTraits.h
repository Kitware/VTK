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
#ifndef viskores_CellTraits_h
#define viskores_CellTraits_h

#include <viskores/CellShape.h>

namespace viskores
{

/// \c viskores::CellTraits::TopologyDimensionType is typedef to this with the
/// template parameter set to \c TOPOLOGICAL_DIMENSIONS. See \c
/// viskores::CellTraits for more information.
///
template <viskores::IdComponent dimension>
struct CellTopologicalDimensionsTag
{
};

/// \brief Tag for cell shapes with a fixed number of points.
///
struct CellTraitsTagSizeFixed
{
};

/// \brief Tag for cell shapes that can have a variable number of points.
///
struct CellTraitsTagSizeVariable
{
};

/// \brief Information about a cell based on its tag.
///
/// The templated CellTraits struct provides the basic high level information
/// about cells (like the number of vertices in the cell or its
/// dimensionality).
///
template <class CellTag>
struct CellTraits
#ifdef VISKORES_DOXYGEN_ONLY
{
  /// This defines the topological dimensions of the cell type. 3 for
  /// polyhedra, 2 for polygons, 1 for lines, 0 for points.
  ///
  static const viskores::IdComponent TOPOLOGICAL_DIMENSIONS = 3;

  /// This tag is typedef'ed to
  /// `viskores::CellTopologicalDimensionsTag<TOPOLOGICAL_DIMENSIONS>`. This provides
  /// a convenient way to overload a function based on topological dimensions
  /// (which is usually more efficient than conditionals).
  ///
  using TopologicalDimensionsTag = viskores::CellTopologicalDimensionsTag<TOPOLOGICAL_DIMENSIONS>;

  /// @brief A tag specifying whether the number of points is fixed.
  ///
  /// If set to `viskores::CellTraitsTagSizeFixed`, then `NUM_POINTS` is set. If set to
  /// `viskores::CellTraitsTagSizeVariable`, then the number of points is not known at
  /// compile time.
  ///
  using IsSizeFixed = viskores::CellTraitsTagSizeFixed;

  /// @brief Number of points in the cell.
  ///
  /// This is only defined for cell shapes of a fixed number of points (i.e.,
  /// `IsSizedFixed` is set to `viskores::CellTraitsTagSizeFixed`).
  ///
  static constexpr viskores::IdComponent NUM_POINTS = 3;
};
#else  // VISKORES_DOXYGEN_ONLY
  ;
#endif // VISKORES_DOXYGEN_ONLY

//-----------------------------------------------------------------------------

// Define traits for every cell type.

#define VISKORES_DEFINE_CELL_TRAITS(name, dimensions, numPoints)                \
  template <>                                                                   \
  struct CellTraits<viskores::CellShapeTag##name>                               \
  {                                                                             \
    static constexpr viskores::IdComponent TOPOLOGICAL_DIMENSIONS = dimensions; \
    using TopologicalDimensionsTag =                                            \
      viskores::CellTopologicalDimensionsTag<TOPOLOGICAL_DIMENSIONS>;           \
    using IsSizeFixed = viskores::CellTraitsTagSizeFixed;                       \
    static constexpr viskores::IdComponent NUM_POINTS = numPoints;              \
  }

#define VISKORES_DEFINE_CELL_TRAITS_VARIABLE(name, dimensions)                  \
  template <>                                                                   \
  struct CellTraits<viskores::CellShapeTag##name>                               \
  {                                                                             \
    static constexpr viskores::IdComponent TOPOLOGICAL_DIMENSIONS = dimensions; \
    using TopologicalDimensionsTag =                                            \
      viskores::CellTopologicalDimensionsTag<TOPOLOGICAL_DIMENSIONS>;           \
    using IsSizeFixed = viskores::CellTraitsTagSizeVariable;                    \
  }

VISKORES_DEFINE_CELL_TRAITS(Empty, 0, 0);
VISKORES_DEFINE_CELL_TRAITS(Vertex, 0, 1);
//VISKORES_DEFINE_CELL_TRAITS_VARIABLE(PolyVertex, 0);
VISKORES_DEFINE_CELL_TRAITS(Line, 1, 2);
VISKORES_DEFINE_CELL_TRAITS_VARIABLE(PolyLine, 1);
VISKORES_DEFINE_CELL_TRAITS(Triangle, 2, 3);
//VISKORES_DEFINE_CELL_TRAITS_VARIABLE(TriangleStrip, 2);
VISKORES_DEFINE_CELL_TRAITS_VARIABLE(Polygon, 2);
//VISKORES_DEFINE_CELL_TRAITS(Pixel, 2, 4);
VISKORES_DEFINE_CELL_TRAITS(Quad, 2, 4);
VISKORES_DEFINE_CELL_TRAITS(Tetra, 3, 4);
//VISKORES_DEFINE_CELL_TRAITS(Voxel, 3, 8);
VISKORES_DEFINE_CELL_TRAITS(Hexahedron, 3, 8);
VISKORES_DEFINE_CELL_TRAITS(Wedge, 3, 6);
VISKORES_DEFINE_CELL_TRAITS(Pyramid, 3, 5);

#undef VISKORES_DEFINE_CELL_TRAITS

} // namespace viskores

#endif //viskores_CellTraits_h
