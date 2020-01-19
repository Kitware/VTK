// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCellGrid
 * @brief   Visualization data composed of cells of arbitrary type.
 *
 * vtkCellGrid inherits vtkDataObject in order to introduce the concept
 * of cells that, instead of relying on spatial points to specify their
 * shape, rely on degrees of freedom (which may or may not be embedded
 * in a world coordinate system).
 *
 * The degrees of freedom that define cells and the functions using those
 * cells as their domain are provided in data arrays.
 * The arrays are partitioned into groups (vtkDataSetAttributes) by the
 * registered cell types. Each array in a group has the same number of tuples.
 *
 * @sa vtkDataObject vtkDataSetAttributes
 */

#ifndef vtkCellGrid_h
#define vtkCellGrid_h

#include "vtkCellGridRangeQuery.h" // For RangeCache ivar.
#include "vtkCompiler.h"           // for VTK_COMPILER_MSVC
#include "vtkDataObject.h"
#include "vtkSmartPointer.h" // For ivars.
#include "vtkStringToken.h"  // For ivars.
#include "vtkTypeName.h"     // For vtk::TypeName<>().

#include <array>         // For ivars.
#include <set>           // For ivars.
#include <unordered_map> // For ivars.

VTK_ABI_NAMESPACE_BEGIN
class vtkCellAttribute;
class vtkCellGridQuery;
class vtkCellGridCopyQuery;
class vtkCellMetadata;
class vtkDataSetAttributes;
class vtkInformationIntegerVectorKey;

class VTKCOMMONDATAMODEL_EXPORT vtkCellGrid : public vtkDataObject
{
public:
  using CellTypeId = vtkStringToken;

  static vtkCellGrid* New();

  vtkTypeMacro(vtkCellGrid, vtkDataObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Restore data object to initial state,
   */
  void Initialize() override;

  /**
   * Return class name of data type.
   * THIS METHOD IS THREAD SAFE
   */
  int GetDataObjectType() VTK_FUTURE_CONST override { return VTK_CELL_GRID; }

  /**
   * Return the actual size of the data in kibibytes (1024 bytes). This number
   * is valid only after the pipeline has updated. The memory size
   * returned is guaranteed to be greater than or equal to the
   * memory required to represent the data (e.g., extra space in
   * arrays, etc. are not included in the return value).
   */
  unsigned long GetActualMemorySize() override;

  ///@name Copying  Copying
  ///@{
  /// Copy \a baseSrc by reference (which must be a vtkCellGrid) into this object.
  ///
  /// This copies cell metadata (i.e., topology) and cell-attributes (including
  /// the cell-attribute holding the geometric shape).
  void ShallowCopy(vtkDataObject* baseSrc) override;

  /// Copy \a baseSrc by value (which must be a vtkCellGrid) into this object.
  ///
  /// This copies cell metadata (i.e., topology) and cell-attributes (including
  /// the cell-attribute holding the geometric shape).
  void DeepCopy(vtkDataObject* baseSrc) override;

  /// Copy the geometric and topological data from \a other, but not any attributes.
  ///
  /// If \a byReference is true, references to source arrays are used directly.
  /// Otherwise, deep copies of the \a other cell-grid's arrays are created.
  bool CopyStructure(vtkCellGrid* other, bool byReference = true);
  ///@}

  ///@name ArrayGroups Array-groups.
  ///
  /// A cell grid can have any number of *groups* of arrays.
  /// Each group must have the same number of tuples.
  /// The groups are related to a particular \a type (generally
  /// they are functional groups related to a particular type of
  /// cell or its sides).
  ///@{
  /**
   * Fetch a partition of DOF arrays.
   * The GetAttributes method will create an empty one if no arrays of that type exist;
   * the GetArrayGroups method returns the map from types to existing partitions.
   * The FindAttributes method will return a null pointer if no arrays of that type exist.
   */
  vtkDataSetAttributes* GetAttributes(int type) override;
  vtkDataSetAttributes* GetAttributes(vtkStringToken type);
  vtkDataSetAttributes* FindAttributes(int type) const;
  vtkDataSetAttributes* FindAttributes(vtkStringToken type) const;
  const std::unordered_map<int, vtkSmartPointer<vtkDataSetAttributes>>& GetArrayGroups() const
  {
    return this->ArrayGroups;
  }

  /// This method populates the map you pass with pointers to all arrays
  /// in this cell-grid's vtkDataSetAttributes instances.
  ///
  /// Various queries and responders need ways to refer to arrays by name
  /// rather than by pointer; this provides a way to produce names based
  /// on the array-group that the array belongs to.
  void MapArrayLocations(
    std::unordered_map<vtkAbstractArray*, vtkStringToken>& arrayLocations) const;
  ///@}

  /**
   * Returns the ghost arrays of the data object of the specified
   * attribute type. This may return a null pointer.
   */
  vtkUnsignedCharArray* GetGhostArray(int type) override;

  /**
   * Returns true if type is CELL, false otherwise
   */
  bool SupportsGhostArray(int type) override;

  /**
   * Retrieves the attribute type that an array came from.
   * This is useful for obtaining which attribute type a input array
   * to an algorithm came from (retrieved from GetInputAbstractArrayToProcesss).
   */
  int GetAttributeTypeForArray(vtkAbstractArray* arr) override;

  /**
   * Get the number of elements for a specific attribute type (POINT, CELL, etc.).
   */
  vtkIdType GetNumberOfElements(int type) override;

  /// Return the number of cells (of all types).
  vtkIdType GetNumberOfCells();

  /**
   * Fill the provided bounding box with the bounds of all the cells present in the grid.
   *
   * If no cells are present, the bounding box will be reset to uninitialized bounds.
   * It is up to each cell type to implement a specialization of the BoundsQuery operation.
   *
   * The bounds are ordered { -x, +x, -y, +y, -z, +z }.
   */
  void GetBounds(double bounds[6]);

  ///@{
  /** Insert a cell type, if possible.
   *
   * If this instance of vtkCellGrid contains the necessary attribute types
   * (i.e., if FindAttributes returns a non-empty object for all the attribute IDs
   * specified by the cell type), then insert an instance into this->Cells and
   * return it. Otherwise, this method will return a null pointer.
   *
   * If the cell type already exists, this will simply return the existing
   * metadata object.
   */
  template <typename CellType>
  CellType* AddCellMetadata()
  {
    CellType* result = this->GetCellsOfType<CellType>();
    if (result)
    {
      return result;
    }
    auto metadata = vtkSmartPointer<CellType>::New();
    if (metadata->SetCellGrid(this))
    {
      auto ok = this->Cells.insert(
        std::make_pair(vtkStringToken(vtk::TypeName<CellType>()).GetId(), metadata));
      if (ok.second)
      {
        result = dynamic_cast<CellType*>(ok.first->second.GetPointer());
      }
    }
    return result;
  }

  vtkCellMetadata* AddCellMetadata(vtkCellMetadata* cellType);

  vtkCellMetadata* AddCellMetadata(vtkStringToken cellTypeName);
  ///@}

  ///@{
  /**\brief Add every registered cell type to this grid.
   *
   * This is useful for queries that need to iterate over registered
   * cell types in order to determine which type of cell to construct.
   * Afterward, your VTK filter/source can call \a RemoveUnusedCellMetadata()
   * to remove metadata for which no cells exist.
   *
   * The number of metadata entries added is returned.
   */
  int AddAllCellMetadata();
  ///@}

  ///@{
  /// Remove all cells of the given type from this cell grid.
  ///
  /// This returns true if cell metadata of the given type was present.
  template <typename CellType>
  bool RemoveCellMetadata()
  {
    CellType* meta = this->GetCellsOfType<CellType>();
    if (!meta)
    {
      return false;
    }
    return this->RemoveCellMetadata(meta);
  }

  bool RemoveCellMetadata(vtkCellMetadata* meta);
  ///@}

  ///@{
  /**\brief Remove every registered cell type in this grid which has no cells.
   *
   * The number of metadata entries removed is returned.
   *
   * \sa vtkCellGrid::AddAllCellMetadata
   */
  int RemoveUnusedCellMetadata();
  ///@}

  ///@{
  /**
   * Get a cell metadata object of the given type.
   */
  template <typename CellType>
  const CellType* GetCellsOfType() const
  {
    auto it = this->Cells.find(vtkStringToken(vtk::TypeName<CellType>()).GetId());
    if (it == this->Cells.end())
    {
      return nullptr;
    }
    return static_cast<const CellType*>(it->second.GetPointer());
  }
  template <typename CellType>
  CellType* GetCellsOfType()
  {
    auto it = this->Cells.find(vtkStringToken(vtk::TypeName<CellType>()).GetId());
    if (it == this->Cells.end())
    {
      return nullptr;
    }
    return static_cast<CellType*>(it->second.GetPointer());
  }
  ///@}

  ///@{
  /**
   * Fill a container with all the cell types (as string tokens).
   */
  template <typename Container>
  void CellTypes(Container& cellTypes) const
  {
    for (const auto& entry : this->Cells)
    {
      cellTypes.insert(cellTypes.end(), entry.first);
    }
  }
  template <typename Container>
  Container CellTypes() const
  {
    Container cellTypes;
    this->CellTypes(cellTypes);
    return cellTypes;
  }
  std::vector<vtkStringToken> CellTypeArray() const;
  std::vector<std::string> GetCellTypes() const;
  ///@}

  ///@{
  /**
   * Return an object that can operate on this vtkCellGrid's cells of the given type.
   */
  const vtkCellMetadata* GetCellType(vtkStringToken cellTypeName) const;
  vtkCellMetadata* GetCellType(vtkStringToken cellTypeName);
  ///@}

  ///@{
  /**
   * Add a cell-attribute to the dataset.
   * A cell-attribute is an object representing a consistent
   * collection of arrays that specify a function over the
   * entire vtkCellGrid's domain (i.e., all cells of all types
   * present in the vtkCellGrid), with custom storage available
   * to each cell type to facilitate interpolation, rendering,
   * and other basic visualization operations.
   */
  virtual bool AddCellAttribute(vtkCellAttribute* attribute);
  ///@}

  ///@{
  /**
   * Remove a cell-attribute from the dataset.
   *
   * This returns true if the cell-attribute was removed.
   * Note that you cannot remove the shape attribute.
   */
  virtual bool RemoveCellAttribute(vtkCellAttribute* attribute);
  ///@}

  /// Return information on the range of values taken on by a component of an attribute.
  ///
  /// This method also accommodates computing the range of the L₁ or L₂ norm
  /// of the entire tuple by passing -1 or -2, respectively, for the
  /// \a componentIndex.
  ///
  /// When called with \a finiteRange equal to false (the default), either
  /// component of \a range may be NaN (not-a-number) or +/-Inf (∞).
  /// When \a finiteRange is true, the returned \a range values will always
  /// be finite.
  ///
  /// Note that if this cell-attribute has no values, the \a range will be
  /// marked invalid (i.e., range[1] < range[0]).
  ///
  /// Note that \a attribute must be an attribute owned by this cell-grid.
  virtual bool GetCellAttributeRange(vtkCellAttribute* attribute, int componentIndex,
    double range[2], bool finiteRange = false) const;

  /// Return the cache of cell-attribute range data.
  /// Responders to vtkCellGridRangeQuery are expected to update this.
  vtkCellGridRangeQuery::CacheMap& GetRangeCache() const { return this->RangeCache; }

  /// Clear the cache of cell-attribute range data.
  ///
  /// If \a attributeName is empty, the entire cache is cleared.
  /// Otherwise, attributes with the given name are cleared from the cache.
  /// This method exists for python scripts.
  void ClearRangeCache(const std::string& attributeName = std::string());

  /// Return the set of cell attribute IDs.
  ///
  /// Values in this set can be passed to GetCellAttributeById().
  std::set<int> GetCellAttributeIds() const;
  std::vector<int> GetUnorderedCellAttributeIds() const;

  /// Return a vector of all this grid's cell-attributes.
  ///
  /// This is a convenience for scripting.
  std::vector<vtkSmartPointer<vtkCellAttribute>> GetCellAttributeList() const;

  ///@{
  /**
   * Return an attribute given its hash.
   *
   * This method is fast (O(1)) and preferred compared to the
   * GetCellAttributeById and GetCellAttributeByName methods below.
   */
  vtkCellAttribute* GetCellAttribute(vtkStringToken::Hash hash);
  ///@}

  ///@{
  /**
   * Return an attribute given its name or identifier.
   *
   * This is currently an O(n) process, but additional indices
   * could be added internally if needed.
   *
   * These methods may return a null pointer if no such attribute exists.
   *
   * Multiple attributes with the same name are possible. The first match
   * will be returned.
   */
  vtkCellAttribute* GetCellAttributeById(int attributeId);
  vtkCellAttribute* GetCellAttributeByName(const std::string& name);
  ///@}

  ///@{
  /**
   * Set/get the "shape attribute" (i.e., a vector-valued cell-attribute
   * that maps from reference to world coordinates).
   *
   * If there is no shape attribute, then a vtkCellGrid cannot be rendered.
   *
   * A shape attribute must have between 1 and 4 components (inclusive).
   *
   * If you call SetShapeAttribute with an attribute that does not satisfy
   * this constraint, this method will return false and have no effect.
   * If you wish to "remove" a grid's shape, call SetShapeAttribute(nullptr).
   */
  vtkCellAttribute* GetShapeAttribute();
  bool SetShapeAttribute(vtkCellAttribute* shape);
  ///@}

  ///@{
  /**
   * Perform a query on all the cells in this instance.
   *
   * The return value indicates success (true when all cells respond
   * to the query) or failure (false when some cell type is unable to
   * handle the query).
   */
  bool Query(vtkCellGridQuery* query);
  ///@}

  /// Set the schema name and version number that generated this object.
  virtual void SetSchema(vtkStringToken name, vtkTypeUInt32 version);
  vtkGetStringTokenMacro(SchemaName);
  vtkGetMacro(SchemaVersion, vtkTypeUInt32);

  /// Set the version number of the object's contents.
  ///
  /// This is not intended to be incremented with each change in memory
  /// (as a vtkTimeStamp would) but when the object is serialized to
  /// disk.
  vtkSetMacro(ContentVersion, vtkTypeUInt32);
  vtkGetMacro(ContentVersion, vtkTypeUInt32)

  ///@{
  /**
   * Retrieve an instance of this class from an information object.
   */
  static vtkCellGrid* GetData(vtkInformation* info);
  static vtkCellGrid* GetData(vtkInformationVector* v, int i = 0);
  ///@}

  /// Identify a correspondence between arrays in two cell grid objects.
  ///
  /// Given two cell-grids and an array held by the former, return the
  /// corresponding array of the latter (i..e, one of the same name held
  /// in an array-group (vtkDataSetAttributes) instance of the same name.
  ///
  /// If no match is found, this returns a null pointer.
  static vtkDataArray* CorrespondingArray(
    vtkCellGrid* gridA, vtkDataArray* arrayA, vtkCellGrid* gridB);

  /// This information key is used to mark arrays with the string token(s)
  /// of their owning vtkDataSetAttributes instance.
  ///
  /// If this key exists on an array, it indicates that calling
  /// vtkCellGrid::GetAttributes() with its value will return a
  /// vtkDataSetAttributes instance that holds the array.
  /// It is used to accelerate the CorrespondingArray() method.
  static vtkInformationIntegerVectorKey* ARRAY_GROUP_IDS();

protected:
  // Provide write access to this->NextAttribute:
  friend class vtkCellGridCopyQuery;

  vtkCellGrid();
  ~vtkCellGrid() override;

  bool ComputeBoundsInternal();
  bool ComputeRangeInternal(vtkCellAttribute* attribute, int component, bool finiteRange) const;

  std::unordered_map<int, vtkSmartPointer<vtkDataSetAttributes>> ArrayGroups;
  std::unordered_map<CellTypeId, vtkSmartPointer<vtkCellMetadata>> Cells;
  std::unordered_map<vtkStringToken::Hash, vtkSmartPointer<vtkCellAttribute>> Attributes;
  int NextAttribute = 0;
  vtkStringToken ShapeAttribute;
  bool HaveShape{ false };

  /// A string specifying the schema which generated this cell-grid.
  vtkStringToken SchemaName;
  /// The monotonically-increasing version number associated with \a SchemaName.
  ///
  /// The number 0 is reserved to indicate no version has been set.
  vtkTypeUInt32 SchemaVersion{ 0 };
  /// A user-provided version number for the grid's data.
  /// This number should be incremented each time the data is serialized.
  ///
  /// The number 0 is reserved to indicate no version has been set.
  vtkTypeUInt32 ContentVersion{ 0 };

  mutable std::array<double, 6> CachedBounds;
  mutable vtkTimeStamp CachedBoundsTime;

  /// Cache for cell attribute component ranges.
  mutable vtkCellGridRangeQuery::CacheMap RangeCache;

private:
  vtkCellGrid(const vtkCellGrid&) = delete;
  void operator=(const vtkCellGrid&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
