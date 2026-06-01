//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include <fides/internal/DataWrapHelper.h>

#if FIDES_USE_VISKORES
#include <viskores/cont/ArrayHandleRuntimeVec.h>
#include <viskores/cont/Field.h>
#include <viskores/cont/UnknownArrayHandle.h>
#endif

namespace fides
{
namespace internal
{

#if FIDES_USE_VISKORES

viskores::cont::Field::Association ConvertToViskoresFieldAssociation(fides::FieldAssociation assoc)
{
  switch (assoc)
  {
    case fides::FieldAssociation::Points:
      return viskores::cont::Field::Association::Points;
    case fides::FieldAssociation::Cells:
      return viskores::cont::Field::Association::Cells;
    case fides::FieldAssociation::WholeDataSet:
      return viskores::cont::Field::Association::WholeDataSet;
    case fides::FieldAssociation::CellGrid:
      // Cell-grid attributes are routed through the cellgrid attribute
      // pipeline, not through Viskores Field; map to WholeDataSet on the
      // off chance a caller asks for the conversion.
      return viskores::cont::Field::Association::WholeDataSet;
    default:
      throw std::runtime_error("Unknown FieldAssociation");
  }
}

fides::FieldAssociation ConvertFromViskoresFieldAssociation(
  viskores::cont::Field::Association assoc)
{
  switch (assoc)
  {
    case viskores::cont::Field::Association::Points:
      return fides::FieldAssociation::Points;
    case viskores::cont::Field::Association::Cells:
      return fides::FieldAssociation::Cells;
    case viskores::cont::Field::Association::WholeDataSet:
      return fides::FieldAssociation::WholeDataSet;
    default:
      throw std::runtime_error("Unknown viskores Field::Association");
  }
}

viskores::UInt8 ConvertCellShapeToViskores(fides::CellShape shape)
{
  // CellShape enum values match Viskores cell shape constants directly.
  return static_cast<viskores::UInt8>(shape);
}

fides::CellShape ConvertCellShapeFromViskores(viskores::UInt8 cellShapeType)
{
  // CellShape enum values match Viskores cell shape constants directly.
  return static_cast<fides::CellShape>(cellShapeType);
}

std::string ConvertViskoresCellTypeToFides(viskores::UInt8 cellShapeType)
{
  std::string cellName;

  if (cellShapeType == viskores::CELL_SHAPE_VERTEX)
  {
    cellName = "vertex";
  }
  else if (cellShapeType == viskores::CELL_SHAPE_LINE)
  {
    cellName = "line";
  }
  else if (cellShapeType == viskores::CELL_SHAPE_TRIANGLE)
  {
    cellName = "triangle";
  }
  else if (cellShapeType == viskores::CELL_SHAPE_QUAD)
  {
    cellName = "quad";
  }
  else if (cellShapeType == viskores::CELL_SHAPE_TETRA)
  {
    cellName = "tetrahedron";
  }
  else if (cellShapeType == viskores::CELL_SHAPE_HEXAHEDRON)
  {
    cellName = "hexahedron";
  }
  else if (cellShapeType == viskores::CELL_SHAPE_WEDGE)
  {
    cellName = "wedge";
  }
  else if (cellShapeType == viskores::CELL_SHAPE_PYRAMID)
  {
    cellName = "pyramid";
  }

  return cellName;
}

viskores::UInt8 ConvertFidesCellTypeToViskores(const std::string& cellShapeName)
{
  viskores::UInt8 cellType = viskores::CELL_SHAPE_EMPTY;

  if (cellShapeName == "vertex")
  {
    cellType = viskores::CELL_SHAPE_VERTEX;
  }
  else if (cellShapeName == "line")
  {
    cellType = viskores::CELL_SHAPE_LINE;
  }
  else if (cellShapeName == "triangle")
  {
    cellType = viskores::CELL_SHAPE_TRIANGLE;
  }
  else if (cellShapeName == "quad")
  {
    cellType = viskores::CELL_SHAPE_QUAD;
  }
  else if (cellShapeName == "tetrahedron")
  {
    cellType = viskores::CELL_SHAPE_TETRA;
  }
  else if (cellShapeName == "hexahedron")
  {
    cellType = viskores::CELL_SHAPE_HEXAHEDRON;
  }
  else if (cellShapeName == "wedge")
  {
    cellType = viskores::CELL_SHAPE_WEDGE;
  }
  else if (cellShapeName == "pyramid")
  {
    cellType = viskores::CELL_SHAPE_PYRAMID;
  }
  else
  {
    throw std::runtime_error("Unsupported fides cell type: " + cellShapeName);
  }

  return cellType;
}

viskores::cont::UnknownArrayHandle RawArrayToUnknownArrayHandle(const fides::RawArray& raw,
                                                                viskores::CopyFlag copy)
{
  // Local macro to generate the cases for each data type, handling
  // both flat arrays and multi-component vectors automatically.
#define FIDES_GENERATE_VISKORES_ARRAY(FIDES_TYPE, C_TYPE)                          \
  case fides::DataType::FIDES_TYPE:                                                \
  {                                                                                \
    if (raw.NumComponents == 1)                                                    \
    {                                                                              \
      return viskores::cont::make_ArrayHandle(                                     \
        raw.GetPointer<C_TYPE>(), static_cast<viskores::Id>(raw.NumValues), copy); \
    }                                                                              \
    else                                                                           \
    {                                                                              \
      return viskores::cont::make_ArrayHandleRuntimeVec(                           \
        raw.NumComponents,                                                         \
        viskores::cont::make_ArrayHandle(                                          \
          raw.GetPointer<C_TYPE>(),                                                \
          static_cast<viskores::Id>(raw.NumValues * raw.NumComponents),            \
          copy));                                                                  \
    }                                                                              \
  }

  switch (raw.Type)
  {
    FIDES_GENERATE_VISKORES_ARRAY(Int8, int8_t)
    FIDES_GENERATE_VISKORES_ARRAY(Int16, int16_t)
    FIDES_GENERATE_VISKORES_ARRAY(Int32, int32_t)
    FIDES_GENERATE_VISKORES_ARRAY(Int64, int64_t)
    FIDES_GENERATE_VISKORES_ARRAY(UInt8, uint8_t)
    FIDES_GENERATE_VISKORES_ARRAY(UInt16, uint16_t)
    FIDES_GENERATE_VISKORES_ARRAY(UInt32, uint32_t)
    FIDES_GENERATE_VISKORES_ARRAY(UInt64, uint64_t)
    FIDES_GENERATE_VISKORES_ARRAY(Float32, float)
    FIDES_GENERATE_VISKORES_ARRAY(Float64, double)

    default:
      throw std::runtime_error("RawArrayToUnknownArrayHandle: unsupported data type");
  }

#undef FIDES_GENERATE_VISKORES_ARRAY
}

#endif // FIDES_USE_VISKORES

}
}
