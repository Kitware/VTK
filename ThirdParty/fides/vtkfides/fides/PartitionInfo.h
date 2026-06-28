//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef fides_PartitionInfo_H_
#define fides_PartitionInfo_H_

#include <fides/FidesTypes.h>

#include <map>
#include <string>
#include <vector>

#include <fides/fides_export.h>

namespace fides
{

/// Describes a single field in a partition.
struct FIDES_EXPORT FieldInfo
{
  std::string Name;
  FieldAssociation Association = FieldAssociation::Points;
  RawArray Data;
};

/// Framework-independent description of one partition of a dataset.
/// Used as the intermediate representation for the writer layer.
struct FIDES_EXPORT PartitionInfo
{
  /// Coordinate system type.
  enum class CoordType
  {
    Uniform,
    Rectilinear,
    Explicit
  };

  /// Cell set type.
  enum class CellType
  {
    Structured,
    SingleType,
    Explicit,
    PolyData
  };

  CoordType Coordinates = CoordType::Uniform;
  CellType Cells = CellType::Structured;

  // --- Uniform coordinates ---
  size_t Dims[3] = { 0, 0, 0 };
  double Origin[3] = { 0, 0, 0 };
  double Spacing[3] = { 1, 1, 1 };

  // --- Rectilinear coordinates ---
  RawArray XCoords;
  RawArray YCoords;
  RawArray ZCoords;

  // --- Explicit coordinates (Nx3 interleaved) ---
  RawArray ExplicitCoords;

  // --- Structured cells: uses Dims above ---

  // --- SingleType cells ---
  CellShape SingleCellShape = CellShape::Triangle;
  int64_t VertsPerCell = 0;
  RawArray SingleTypeConnectivity;

  // --- Explicit cells ---
  RawArray ExplicitCellTypes;
  RawArray ExplicitNumVerts;
  RawArray ExplicitConnectivity;

  // --- PolyData cells ---
  // Mirrors vtkPolyData's four independent cell arrays. Each role is an
  // (offsets, connectivity) pair using vtkCellArray's modern layout
  // (Offsets has nCells+1 entries; Connectivity is the concatenated
  // index buffer). Any role's pair may be empty (no cells of that
  // kind). Cell-type semantics are implicit per role + vertex count
  // and don't need a separate type byte.
  RawArray PolyDataVertsOffsets;
  RawArray PolyDataVertsConnectivity;
  RawArray PolyDataLinesOffsets;
  RawArray PolyDataLinesConnectivity;
  RawArray PolyDataPolysOffsets;
  RawArray PolyDataPolysConnectivity;
  RawArray PolyDataStripsOffsets;
  RawArray PolyDataStripsConnectivity;

  // --- Fields ---
  std::vector<FieldInfo> Fields;

  /// Returns the number of points in this partition.
  size_t NumberOfPoints() const
  {
    switch (Coordinates)
    {
      case CoordType::Uniform:
        return Dims[0] * Dims[1] * Dims[2];
      case CoordType::Rectilinear:
        return XCoords.NumValues * YCoords.NumValues * ZCoords.NumValues;
      case CoordType::Explicit:
        return ExplicitCoords.NumValues;
      default:
        return 0;
    }
  }

  /// Returns the number of cells in this partition.
  size_t NumberOfCells() const
  {
    switch (Cells)
    {
      case CellType::Structured:
      {
        size_t d[3];
        if (Coordinates == CoordType::Uniform)
        {
          d[0] = Dims[0];
          d[1] = Dims[1];
          d[2] = Dims[2];
        }
        else if (Coordinates == CoordType::Rectilinear)
        {
          d[0] = XCoords.NumValues;
          d[1] = YCoords.NumValues;
          d[2] = ZCoords.NumValues;
        }
        else
        {
          d[0] = Dims[0];
          d[1] = Dims[1];
          d[2] = Dims[2];
        }
        size_t n = 1;
        for (int i = 0; i < 3; i++)
        {
          if (d[i] > 1)
          {
            n *= (d[i] - 1);
          }
        }
        return n;
      }
      case CellType::SingleType:
        if (VertsPerCell > 0)
        {
          return SingleTypeConnectivity.NumValues / static_cast<size_t>(VertsPerCell);
        }
        return 0;
      case CellType::Explicit:
        return ExplicitCellTypes.NumValues;
      case CellType::PolyData:
      {
        // Offsets has nCells+1 entries per role; subtract 1 per non-empty
        // role. Empty roles contribute zero.
        auto count = [](const RawArray& off) -> size_t {
          return off.NumValues > 0 ? off.NumValues - 1 : 0;
        };
        return count(PolyDataVertsOffsets) + count(PolyDataLinesOffsets) +
          count(PolyDataPolysOffsets) + count(PolyDataStripsOffsets);
      }
      default:
        return 0;
    }
  }

  /// Returns the data model type string for schema generation.
  std::string GetDataModelTypeString() const
  {
    switch (Coordinates)
    {
      case CoordType::Uniform:
        return "uniform";
      case CoordType::Rectilinear:
        return "rectilinear";
      case CoordType::Explicit:
        if (Cells == CellType::Structured)
        {
          return "structured";
        }
        if (Cells == CellType::SingleType)
        {
          return "unstructured_single";
        }
        if (Cells == CellType::PolyData)
        {
          return "polydata";
        }
        return "unstructured";
      default:
        return "unsupported";
    }
  }
};

/// Framework-independent description of one vtkCellGrid partition of a
/// dataset. Mirrors the structure that the reader's CellGridModel
/// produces on the read side: a list of cell types and a list of
/// attributes, each attribute carrying per-cell-type DOF metadata plus
/// values / connectivity RawArrays. The on-disk ADIOS2 layout is
/// enumerated by FidesWriter when serializing.
struct FIDES_EXPORT CellGridPartitionInfo
{
  /// One registered cell type (e.g. "vtkDGHex") and the geometric shape
  /// it maps to (e.g. "hexahedron"). Order is preserved when emitting
  /// the ADIOS2 \c cell_types attribute.
  struct CellTypeEntry
  {
    std::string CellTypeName;
    std::string ShapeName;
  };

  /// One array on a (attribute, cell-type) pair, identified by its
  /// vtkCellGrid role ("values", "connectivity", "ghost-node", ...).
  /// \c Data is the zero-copy array to write; the matching
  /// \c <attr>/<ct>/<role> ADIOS2 attribute holds the full variable name
  /// "group/arrayName" assembled from \c Group + \c ArrayName.
  struct RoleArray
  {
    std::string Group;
    std::string ArrayName;
    RawArray Data;
  };

  /// One (attribute, cell-type) DOF block. A cell attribute may carry any
  /// number of array roles; the set is read off the vtkCellGrid rather
  /// than assuming a fixed values/connectivity pair. \c Offset and
  /// \c Blanked are vtkDGCell source-spec scalars, meaningful only for the
  /// shape attribute (the geometric source).
  struct AttributePerType
  {
    std::string CellTypeName;
    std::string FunctionSpace;
    std::string Basis;
    int Order = 1;
    std::string DOFSharing;
    std::map<std::string, RoleArray> Roles;
    int64_t Offset = 0;
    bool Blanked = false;
  };

  /// One cell attribute (e.g. the shape attribute, or a scalar field
  /// defined across cell types). PerCellType only lists cell types where
  /// the attribute is defined.
  struct Attribute
  {
    std::string Name;
    std::string Space;
    int Components = 1;
    bool IsShape = false;
    std::vector<AttributePerType> PerCellType;
  };

  std::vector<CellTypeEntry> CellTypes;
  std::vector<Attribute> Attributes;
};

/// One assembly (vtkDataAssembly) tree node for a multi-dataset write.
/// \c Datasets references collection items by name. Mirrors the read-side
/// OutputBuilder::AssemblyNode; kept separate so the writer stays
/// independent of the read IR.
struct FIDES_EXPORT AssemblyNode
{
  std::string Name;
  std::vector<AssemblyNode> Children;
  std::vector<std::string> Datasets;
};

/// One named item of a multi-dataset (PDC) write: the item name (which
/// becomes the ADIOS variable group prefix) and its partitions.
struct FIDES_EXPORT CollectionItem
{
  std::string Name;
  std::vector<PartitionInfo> Partitions;
};

} // namespace fides

#endif // fides_PartitionInfo_H_
