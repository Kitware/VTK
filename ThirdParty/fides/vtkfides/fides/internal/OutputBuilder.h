//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef fides_OutputBuilder_H_
#define fides_OutputBuilder_H_

#include <fides/FidesTypes.h>

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include <fides/fides_export.h>

namespace fides
{

/// \brief Abstract interface for building output datasets from raw data.
///
/// OutputBuilder provides a backend-neutral interface for constructing
/// datasets from raw arrays read by the Fides data model objects.
/// Concrete implementations (ViskoresBuilder, VTKBuilder) translate
/// these calls into framework-specific data structures.
///
/// All methods that create objects return opaque size_t tokens.
/// These tokens are used to reference created objects in subsequent calls.
class FIDES_EXPORT OutputBuilder
{
public:
  /// Token value that indicates "no-op" or "handled externally".
  static constexpr size_t InvalidToken = 0;

  virtual ~OutputBuilder() = default;

  // --- Array creation (returns opaque token) ---

  /// Create an array from a RawArray buffer. Returns token.
  virtual size_t CreateArray(const RawArray& raw);

  /// Create uniform (image) coordinates. Returns token.
  ///
  /// \c start is the per-block start index in the global grid (or
  /// \c nullptr / all-zeros if not partitioned). The default
  /// implementation shifts the stored origin by \c spacing*start so
  /// downstream code that works in local coordinates (Viskores) sees
  /// the correct per-block origin. The VTKBuilder override stores
  /// \c start separately and uses it to set per-partition extents so
  /// siblings in a vtkPartitionedDataSet share origin/spacing.
  virtual size_t CreateUniformCoordinates(const int64_t dims[3],
                                          const double origin[3],
                                          const double spacing[3],
                                          const int64_t start[3] = nullptr);

  /// Create rectilinear (cartesian product) coordinates from three
  /// axis-aligned arrays. Returns token.
  virtual size_t CreateRectilinearCoordinates(size_t xArrayToken,
                                              size_t yArrayToken,
                                              size_t zArrayToken);

  /// Create composite (SOA Vec3) coordinates from three component arrays.
  /// Returns token.
  virtual size_t CreateCompositeCoordinates(size_t xArrayToken,
                                            size_t yArrayToken,
                                            size_t zArrayToken);

  // --- CellSet creation ---

  /// Create a structured cell set from point dimensions. Returns token.
  virtual size_t CreateStructuredCellSet(const int64_t dims[3]);

  /// Create a single-type unstructured cell set. Returns token.
  virtual size_t CreateSingleTypeCellSet(CellShape shape,
                                         int vertsPerCell,
                                         size_t connectivityArrayToken);

  /// Create an explicit (mixed-type) unstructured cell set. Returns token.
  virtual size_t CreateExplicitCellSet(size_t cellTypesArrayToken,
                                       size_t numVertsArrayToken,
                                       size_t connectivityArrayToken);

  /// Create a polydata cell set from up to four (offsets, connectivity)
  /// array-token pairs, one per vtkPolyData cell-array role. Any role
  /// may pass \c InvalidToken for both pair members, meaning "no cells of
  /// that kind". The on-disk layout mirrors vtkCellArray's modern
  /// offsets+connectivity format directly; cell-type semantics are
  /// implicit per role + vertex count, so no per-cell type byte is
  /// needed.
  virtual size_t CreatePolyDataCellSet(size_t vertsOffsetsToken,
                                       size_t vertsConnToken,
                                       size_t linesOffsetsToken,
                                       size_t linesConnToken,
                                       size_t polysOffsetsToken,
                                       size_t polysConnToken,
                                       size_t stripsOffsetsToken,
                                       size_t stripsConnToken);

  // --- Dataset assembly ---

  /// Create a new empty dataset. Returns token.
  virtual size_t CreateDataSet();

  /// Set the coordinate system for a dataset.
  virtual void SetCoordinateSystem(size_t dataSetToken, size_t coordToken);

  /// Set the cell set for a dataset.
  virtual void SetCellSet(size_t dataSetToken, size_t cellSetToken);

  /// Add a field to a dataset.
  virtual void AddField(size_t dataSetToken,
                        const std::string& name,
                        FieldAssociation assoc,
                        size_t arrayToken);

  /// Add a dataset as a partition to the output.
  virtual void AddPartition(size_t dataSetToken);

  // --- CellGrid (vtkCellGrid / DG finite element) creation ---

  /// Create an empty cell grid. Returns a token usable as a partition,
  /// passed to AddPartition() once fully populated. Cell grids only
  /// produce output through the VTK backend.
  virtual size_t CreateCellGrid();

  /// Register a cell type (e.g. "vtkDGHex") with shape (e.g. "hexahedron")
  /// on the cell grid identified by \c cgToken.
  virtual void AddCellTypeToCellGrid(size_t cgToken,
                                     const std::string& cellTypeName,
                                     const std::string& shapeName);

  /// Create a cell attribute (e.g. shape, scalar field) for a cell grid.
  /// Returns an attribute token used in subsequent SetCellAttributeForType
  /// and AddCellAttributeToCellGrid calls.
  virtual size_t CreateCellAttribute(const std::string& name,
                                     const std::string& space,
                                     int numComponents,
                                     bool isShape);

  /// Configure the structural metadata of a cell attribute on cells of
  /// one specific cell type. The arrays this (attribute, cell-type) pair
  /// carries are added separately via \c AddCellAttributeArrayForType,
  /// one call per role. \c dofSharing may be empty for discontinuous
  /// DOFs. \c offset and \c blanked feed the vtkDGCell source spec for
  /// the shape attribute (the geometric source); they are ignored for
  /// non-shape attributes and default to 0 / false.
  virtual void SetCellAttributeForType(size_t attrToken,
                                       const std::string& cellTypeName,
                                       const std::string& functionSpace,
                                       const std::string& basis,
                                       int order,
                                       const std::string& dofSharing,
                                       int64_t offset = 0,
                                       bool blanked = false);

  /// Attach one array, identified by its vtkCellGrid \c role (e.g.
  /// "values", "connectivity", "ghost"), to a previously-configured
  /// (attribute, cell-type) pair. The (groupName, arrayName) pair is how
  /// the array is organized inside the resulting vtkCellGrid;
  /// \c arrayToken may be \c InvalidToken when the role is named in the
  /// metadata but has no array data in this block.
  virtual void AddCellAttributeArrayForType(size_t attrToken,
                                            const std::string& cellTypeName,
                                            const std::string& role,
                                            const std::string& group,
                                            const std::string& arrayName,
                                            size_t arrayToken);

  /// Add a previously-created cell attribute to a cell grid.
  virtual void AddCellAttributeToCellGrid(size_t cgToken, size_t attrToken);

  // --- Query ---

  /// Get the number of values in an array identified by token.
  virtual size_t GetArrayNumberOfValues(size_t arrayToken) const;

  /// Get the number of components in an array identified by token.
  virtual int GetArrayNumberOfComponents(size_t arrayToken) const;

  /// Reset the builder, clearing all stored objects.
  virtual void Reset();

  /// Called after all deferred reads have completed (i.e., after DoAllReads
  /// or EndStep). Allows the builder to finalize construction of objects
  /// that require populated array data (e.g., cell sets that need ArrayCopy).
  virtual void Finalize() = 0;

protected:
  size_t NextToken = 1; // 0 is reserved as InvalidToken

  // --- Raw data storage (populated before Finalize) ---

  /// Stores the RawArray for each array token.
  std::unordered_map<size_t, fides::RawArray> StoredArrays;

  struct CoordEntry
  {
    enum class Type
    {
      Uniform,
      Rectilinear,
      Composite,
      Array
    };
    Type EntryType = Type::Array;
    // Uniform:
    int64_t Dims[3] = { 0, 0, 0 };
    double Origin[3] = { 0, 0, 0 };
    double Spacing[3] = { 1, 1, 1 };
    // Per-block start index in the global grid. Builders that produce
    // partitioned-with-shared-origin output (VTKBuilder) use this to
    // compute partition extents; builders that fold the start into the
    // origin (the default) leave it at zero.
    int64_t Start[3] = { 0, 0, 0 };
    // Rectilinear and Composite:
    size_t XToken = 0;
    size_t YToken = 0;
    size_t ZToken = 0;
    // Array (plain array token for explicit coords):
    size_t ArrayToken = 0;
  };
  std::unordered_map<size_t, CoordEntry> StoredCoords;

  struct CellSetEntry
  {
    enum class Type
    {
      Structured,
      SingleType,
      Explicit,
      PolyData
    };
    Type EntryType;
    // Structured:
    int64_t Dims[3] = { 0, 0, 0 };
    // SingleType:
    CellShape Shape = CellShape::Triangle;
    int VertsPerCell = 0;
    size_t ConnToken = 0;
    // Explicit:
    size_t TypesToken = 0;
    size_t NVertsToken = 0;
    size_t ConnTokenExplicit = 0;
    // PolyData: four (offsets, connectivity) array-token pairs. Either
    // token of a pair set to \c InvalidToken means "no cells of that
    // kind" for this partition. Roles mirror vtkPolyData::SetVerts /
    // SetLines / SetPolys / SetStrips.
    size_t PolyVertsOffsetsToken = 0;
    size_t PolyVertsConnToken = 0;
    size_t PolyLinesOffsetsToken = 0;
    size_t PolyLinesConnToken = 0;
    size_t PolyPolysOffsetsToken = 0;
    size_t PolyPolysConnToken = 0;
    size_t PolyStripsOffsetsToken = 0;
    size_t PolyStripsConnToken = 0;
  };
  std::unordered_map<size_t, CellSetEntry> StoredCellSets;

  // --- Deferred operations (recorded before Finalize, applied during) ---

  struct DeferredCoordSystem
  {
    size_t DataSetToken;
    size_t CoordToken;
  };
  std::vector<DeferredCoordSystem> DeferredCoordSystems;

  struct DeferredCellSet
  {
    size_t DataSetToken;
    size_t CellSetToken;
  };
  std::vector<DeferredCellSet> DeferredCellSets;

  struct DeferredField
  {
    size_t DataSetToken;
    std::string Name;
    FieldAssociation Assoc;
    size_t ArrayToken;
  };
  std::vector<DeferredField> DeferredFields;

  std::vector<size_t> DataSetTokens; // ordered list of dataset tokens
  std::vector<size_t> PartitionTokens;

  std::unordered_map<size_t, size_t> DataSetTokenMap; // token -> index in DataSetsVec

  // --- CellGrid storage ---

  struct CellTypeRegistration
  {
    std::string CellTypeName;
    std::string ShapeName;
  };

  struct CellGridEntry
  {
    std::vector<CellTypeRegistration> CellTypes;
    std::vector<size_t> AttributeTokens; // ordered attribute tokens
  };
  std::unordered_map<size_t, CellGridEntry> StoredCellGrids;

  struct CellAttributeArrayRef
  {
    std::string Group;
    std::string ArrayName;
    size_t Token = InvalidToken;
  };

  struct CellAttributePerTypeEntry
  {
    std::string CellTypeName;
    std::string FunctionSpace;
    std::string Basis;
    int Order = 0;
    std::string DOFSharing; // empty if discontinuous
    // Arrays keyed by their vtkCellGrid role ("values", "connectivity",
    // "ghost-node", ...). A cell attribute may carry any number of roles;
    // the set is discovered from the data source, not hardcoded.
    std::map<std::string, CellAttributeArrayRef> Roles;
    // vtkDGCell source-spec scalars (meaningful only for the shape
    // attribute, which drives the cell's geometric source).
    int64_t Offset = 0;
    bool Blanked = false;
  };

  struct CellAttributeEntry
  {
    std::string Name;
    std::string Space;
    int Components = 0;
    bool IsShape = false;
    std::vector<CellAttributePerTypeEntry> PerCellType;
  };
  std::unordered_map<size_t, CellAttributeEntry> StoredCellAttributes;

  size_t AllocToken() { return NextToken++; }
};

} // namespace fides

#endif // fides_OutputBuilder_H_
