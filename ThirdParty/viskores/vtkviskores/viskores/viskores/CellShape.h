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
#ifndef viskores_CellShape_h
#define viskores_CellShape_h

#include <viskores/StaticAssert.h>
#include <viskores/Types.h>

#include <lcl/Polygon.h>
#include <lcl/Shapes.h>

// Vtk-c does not have tags for Empty and PolyLine. Define dummy tags here to
// avoid compilation errors. These tags are not used anywhere.
namespace lcl
{
struct Empty;
struct PolyLine;
}

namespace viskores
{

/// CellShapeId identifies the type of each cell. Currently these are designed
/// to match up with VTK cell types.
///
enum CellShapeIdEnum
{
  // Linear cells
  /// Placeholder for empty or invalid cells.
  CELL_SHAPE_EMPTY = lcl::ShapeId::EMPTY,
  /// Vertex cells of a single point.
  CELL_SHAPE_VERTEX = lcl::ShapeId::VERTEX,
  //CELL_SHAPE_POLY_VERTEX      = 2,
  /// A line cell connecting two points.
  CELL_SHAPE_LINE = lcl::ShapeId::LINE,
  /// A sequence of line segments.
  /// A polyline has 2 or more points, and the points are connected in order
  /// by line segments forming a piecewise linear curve.
  CELL_SHAPE_POLY_LINE = 4,
  /// A triangle.
  CELL_SHAPE_TRIANGLE = lcl::ShapeId::TRIANGLE,
  //CELL_SHAPE_TRIANGLE_STRIP   = 6,
  /// A general polygon shape.
  /// All polygons have points ordered in counterclockwise order around the front side.
  /// Some operations may be invalid if the polygon is not a convex shape.
  CELL_SHAPE_POLYGON = lcl::ShapeId::POLYGON,
  //CELL_SHAPE_PIXEL            = 8,
  /// A four-sided polygon.
  CELL_SHAPE_QUAD = lcl::ShapeId::QUAD,
  /// A tetrahedron.
  /// A tetrahedron is a 3D polyhedron with 4 points and 4 triangular faces.
  CELL_SHAPE_TETRA = lcl::ShapeId::TETRA,
  //CELL_SHAPE_VOXEL            = 11,
  /// A hexahedron.
  CELL_SHAPE_HEXAHEDRON = lcl::ShapeId::HEXAHEDRON,
  /// A wedge.
  /// Wedges are simple prisms that can be formed by extruding a triangle.
  /// They have 2 triangular faces and 3 quadrilateral faces.
  CELL_SHAPE_WEDGE = lcl::ShapeId::WEDGE,
  /// A pyramid with a quadrilateral base and four triangular faces.0
  CELL_SHAPE_PYRAMID = lcl::ShapeId::PYRAMID,

  NUMBER_OF_CELL_SHAPES
};

// If you wish to add cell shapes to this list, in addition to adding an index
// to the enum above, you at a minimum need to define an associated tag with
// VISKORES_DEFINE_CELL_TAG and add a condition to the viskoresGenericCellShapeMacro.
// There are also many other cell-specific features that code might expect such
// as \c CellTraits and interpolations.

namespace internal
{

/// A class that can be used to determine if a class is a CellShapeTag or not.
/// The class will be either std::true_type or std::false_type.
///
template <typename T>
struct CellShapeTagCheck : std::false_type
{
};

/// Convert Viskores tag to VTK-c tag
template <typename ViskoresCellShapeTag>
struct CellShapeTagViskoresToVtkc;

} // namespace internal

/// Checks that the argument is a proper cell shape tag. This is a handy
/// concept check to make sure that a template argument is a proper cell shape
/// tag.
///
#define VISKORES_IS_CELL_SHAPE_TAG(tag)                                           \
  VISKORES_STATIC_ASSERT_MSG(::viskores::internal::CellShapeTagCheck<tag>::value, \
                             "Provided type is not a valid Viskores cell shape tag.")

/// A traits-like class to get an CellShapeId known at compile time to a tag.
///
template <viskores::IdComponent Id>
struct CellShapeIdToTag
{
  // If you get a compile error for this class about Id not being defined, that
  // probably means you are using an ID that does not have a defined cell
  // shape.

  using valid = std::false_type;
};

// Define a tag for each cell shape as well as the support structs to go
// between tags and ids. The following macro is only valid here.

#define VISKORES_DEFINE_CELL_TAG(name, idname)                                                \
  struct CellShapeTag##name                                                                   \
  {                                                                                           \
    static constexpr viskores::UInt8 Id = viskores::idname;                                   \
  };                                                                                          \
  namespace internal                                                                          \
  {                                                                                           \
  template <>                                                                                 \
  struct CellShapeTagCheck<viskores::CellShapeTag##name> : std::true_type                     \
  {                                                                                           \
  };                                                                                          \
  template <>                                                                                 \
  struct CellShapeTagViskoresToVtkc<viskores::CellShapeTag##name>                             \
  {                                                                                           \
    using Type = lcl::name;                                                                   \
  };                                                                                          \
  }                                                                                           \
  static inline VISKORES_EXEC_CONT const char* GetCellShapeName(viskores::CellShapeTag##name) \
  {                                                                                           \
    return #name;                                                                             \
  }                                                                                           \
  template <>                                                                                 \
  struct CellShapeIdToTag<viskores::idname>                                                   \
  {                                                                                           \
    using valid = std::true_type;                                                             \
    using Tag = viskores::CellShapeTag##name;                                                 \
  }

VISKORES_DEFINE_CELL_TAG(Empty, CELL_SHAPE_EMPTY);
VISKORES_DEFINE_CELL_TAG(Vertex, CELL_SHAPE_VERTEX);
//VISKORES_DEFINE_CELL_TAG(PolyVertex, CELL_SHAPE_POLY_VERTEX);
VISKORES_DEFINE_CELL_TAG(Line, CELL_SHAPE_LINE);
VISKORES_DEFINE_CELL_TAG(PolyLine, CELL_SHAPE_POLY_LINE);
VISKORES_DEFINE_CELL_TAG(Triangle, CELL_SHAPE_TRIANGLE);
//VISKORES_DEFINE_CELL_TAG(TriangleStrip, CELL_SHAPE_TRIANGLE_STRIP);
VISKORES_DEFINE_CELL_TAG(Polygon, CELL_SHAPE_POLYGON);
//VISKORES_DEFINE_CELL_TAG(Pixel, CELL_SHAPE_PIXEL);
VISKORES_DEFINE_CELL_TAG(Quad, CELL_SHAPE_QUAD);
VISKORES_DEFINE_CELL_TAG(Tetra, CELL_SHAPE_TETRA);
//VISKORES_DEFINE_CELL_TAG(Voxel, CELL_SHAPE_VOXEL);
VISKORES_DEFINE_CELL_TAG(Hexahedron, CELL_SHAPE_HEXAHEDRON);
VISKORES_DEFINE_CELL_TAG(Wedge, CELL_SHAPE_WEDGE);
VISKORES_DEFINE_CELL_TAG(Pyramid, CELL_SHAPE_PYRAMID);

#undef VISKORES_DEFINE_CELL_TAG

/// A special cell shape tag that holds a cell shape that is not known at
/// compile time. Unlike other cell set tags, the Id field is set at runtime
/// so its value cannot be used in template parameters. You need to use
/// \c viskoresGenericCellShapeMacro to specialize on the cell type.
///
struct CellShapeTagGeneric
{
  VISKORES_EXEC_CONT
  CellShapeTagGeneric(viskores::UInt8 shape)
    : Id(shape)
  {
  }

  /// An identifier that corresponds to one of the `CELL_SHAPE_*` identifiers.
  /// This value is used to detect the proper shape at runtime.
  viskores::UInt8 Id;
};

namespace internal
{

template <typename ViskoresCellShapeTag>
VISKORES_EXEC_CONT inline typename CellShapeTagViskoresToVtkc<ViskoresCellShapeTag>::Type
make_LclCellShapeTag(const ViskoresCellShapeTag&, viskores::IdComponent numPoints = 0)
{
  using VtkcCellShapeTag = typename CellShapeTagViskoresToVtkc<ViskoresCellShapeTag>::Type;
  static_cast<void>(numPoints); // unused
  return VtkcCellShapeTag{};
}

VISKORES_EXEC_CONT
inline lcl::Polygon make_LclCellShapeTag(const viskores::CellShapeTagPolygon&,
                                         viskores::IdComponent numPoints = 0)
{
  return lcl::Polygon(numPoints);
}

VISKORES_EXEC_CONT
inline lcl::Cell make_LclCellShapeTag(const viskores::CellShapeTagGeneric& tag,
                                      viskores::IdComponent numPoints = 0)
{
  return lcl::Cell(static_cast<std::int8_t>(tag.Id), numPoints);
}

} // namespace internal

#define viskoresGenericCellShapeMacroCase(cellShapeId, call)                     \
  case viskores::cellShapeId:                                                    \
  {                                                                              \
    using CellShapeTag = viskores::CellShapeIdToTag<viskores::cellShapeId>::Tag; \
    call;                                                                        \
  }                                                                              \
  break

/// \brief A macro used in a \c switch statement to determine cell shape.
///
/// The \c viskoresGenericCellShapeMacro is a series of case statements for all
/// of the cell shapes supported by Viskores. This macro is intended to be used
/// inside of a switch statement on a cell type. For each cell shape condition,
/// a \c CellShapeTag typedef is created and the given \c call is executed.
///
/// A typical use case of this class is to create the specialization of a
/// function overloaded on a cell shape tag for the generic cell shape like as
/// following.
///
/// \code{.cpp}
/// template<typename WorkletType>
/// VISKORES_EXEC
/// void MyCellOperation(viskores::CellShapeTagGeneric cellShape,
///                      const viskores::exec::FunctorBase &worklet)
/// {
///   switch(cellShape.CellShapeId)
///   {
///     viskoresGenericCellShapeMacro(
///       MyCellOperation(CellShapeTag())
///       );
///     default: worklet.RaiseError("Encountered unknown cell shape."); break
///   }
/// }
/// \endcode
///
/// Note that \c viskoresGenericCellShapeMacro does not have a default case. You
/// should consider adding one that gives a
///
#define viskoresGenericCellShapeMacro(call)                       \
  viskoresGenericCellShapeMacroCase(CELL_SHAPE_EMPTY, call);      \
  viskoresGenericCellShapeMacroCase(CELL_SHAPE_VERTEX, call);     \
  viskoresGenericCellShapeMacroCase(CELL_SHAPE_LINE, call);       \
  viskoresGenericCellShapeMacroCase(CELL_SHAPE_POLY_LINE, call);  \
  viskoresGenericCellShapeMacroCase(CELL_SHAPE_TRIANGLE, call);   \
  viskoresGenericCellShapeMacroCase(CELL_SHAPE_POLYGON, call);    \
  viskoresGenericCellShapeMacroCase(CELL_SHAPE_QUAD, call);       \
  viskoresGenericCellShapeMacroCase(CELL_SHAPE_TETRA, call);      \
  viskoresGenericCellShapeMacroCase(CELL_SHAPE_HEXAHEDRON, call); \
  viskoresGenericCellShapeMacroCase(CELL_SHAPE_WEDGE, call);      \
  viskoresGenericCellShapeMacroCase(CELL_SHAPE_PYRAMID, call)

} // namespace viskores

#endif //viskores_CellShape_h
