//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.md for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef lcl_Shapes_h
#define lcl_Shapes_h

#include <lcl/internal/Config.h>

#include <cstdint>

namespace lcl
{

enum ShapeId : IdShape
{
  // Linear cells
  EMPTY            = 0,
  VERTEX           = 1,
  //POLY_VERTEX      = 2,
  LINE             = 3,
  //POLY_LINE        = 4,
  TRIANGLE         = 5,
  //TRIANGLE_STRIP   = 6,
  POLYGON          = 7,
  PIXEL            = 8,
  QUAD             = 9,
  TETRA            = 10,
  VOXEL            = 11,
  HEXAHEDRON       = 12,
  WEDGE            = 13,
  PYRAMID          = 14,

  NUMBER_OF_CELL_SHAPES
};

class Cell
{
public:
  constexpr LCL_EXEC Cell() : Shape(ShapeId::EMPTY), NumberOfPoints(0) {}
  constexpr LCL_EXEC Cell(IdShape shapeId, IdComponent numPoints)
    : Shape(shapeId), NumberOfPoints(numPoints)
  {
  }

  constexpr LCL_EXEC IdShape shape() const noexcept { return this->Shape; }
  constexpr LCL_EXEC IdComponent numberOfPoints() const noexcept { return this->NumberOfPoints; }

protected:
  IdShape Shape;
  IdComponent NumberOfPoints;
};

/// \brief Check if a shape id is valid
/// \param[in]  shapeId  The id to check.
/// \return              true if id is a valid shape id.
///
constexpr inline LCL_EXEC bool isValidShape(IdShape shapeId)
{
  return (shapeId >= ShapeId::EMPTY) && (shapeId < ShapeId::NUMBER_OF_CELL_SHAPES);
}

/// \brief Returns the dimensionality of a cell
/// \param[in]  shapeId  The shape id of the cell.
/// \return              The dimensionality of the cell, -1 for invalid shapes.
///
inline LCL_EXEC int dimension(IdShape shapeId)
{
  switch (shapeId)
  {
    case VERTEX:
      return 0;
    case LINE:
      return 1;
    case TRIANGLE:
    case POLYGON:
    case PIXEL:
    case QUAD:
      return 2;
    case TETRA:
    case VOXEL:
    case HEXAHEDRON:
    case WEDGE:
    case PYRAMID:
      return 3;
    case EMPTY:
    default:
      return -1;
  }
}

/// \brief Returns the dimensionality of a cell
/// \param[in]  cell  The cell.
/// \return           The dimensionality of the cell, -1 for invalid shapes.
///
inline LCL_EXEC int dimension(Cell cell)
{
  return dimension(cell.shape());
}

// forward declare cell tags
class Vertex;
class Line;
class Triangle;
class Polygon;
class Pixel;
class Quad;
class Tetra;
class Voxel;
class Hexahedron;
class Wedge;
class Pyramid;

} //namespace lcl

#define lclGenericCellShapeMacroCase(cellId, cell, call)                                          \
  case cellId:                                                                                     \
  {                                                                                                \
    using CellTag = cell;                                                                          \
    call;                                                                                          \
  }                                                                                                \
  break

#define lclGenericCellShapeMacro(call)                                                            \
  lclGenericCellShapeMacroCase(lcl::ShapeId::VERTEX,     lcl::Vertex,     call);                \
  lclGenericCellShapeMacroCase(lcl::ShapeId::LINE,       lcl::Line,       call);                \
  lclGenericCellShapeMacroCase(lcl::ShapeId::TRIANGLE,   lcl::Triangle,   call);                \
  lclGenericCellShapeMacroCase(lcl::ShapeId::POLYGON,    lcl::Polygon,    call);                \
  lclGenericCellShapeMacroCase(lcl::ShapeId::PIXEL,      lcl::Pixel,      call);                \
  lclGenericCellShapeMacroCase(lcl::ShapeId::QUAD,       lcl::Quad,       call);                \
  lclGenericCellShapeMacroCase(lcl::ShapeId::TETRA,      lcl::Tetra,      call);                \
  lclGenericCellShapeMacroCase(lcl::ShapeId::VOXEL,      lcl::Voxel,      call);                \
  lclGenericCellShapeMacroCase(lcl::ShapeId::HEXAHEDRON, lcl::Hexahedron, call);                \
  lclGenericCellShapeMacroCase(lcl::ShapeId::WEDGE,      lcl::Wedge,      call);                \
  lclGenericCellShapeMacroCase(lcl::ShapeId::PYRAMID,    lcl::Pyramid,    call)

#endif //lcl_Shapes_h
