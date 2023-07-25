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

#include "vtkCompiler.h" // for VTK_COMPILER_MSVC
#include "vtkDataObject.h"
#include "vtkSmartPointer.h" // For ivars.
#include "vtkStringToken.h"  // For ivars.
#include "vtkTypeName.h"     // For vtk::TypeName<>().

#include <array>         // For ivars.
#include <unordered_map> // For ivars.

VTK_ABI_NAMESPACE_BEGIN
class vtkCellAttribute;
class vtkCellGridQuery;
class vtkCellMetadata;
class vtkDataSetAttributes;

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
  int GetDataObjectType() override { return VTK_CELL_GRID; }

  /**
   * Return the actual size of the data in kibibytes (1024 bytes). This number
   * is valid only after the pipeline has updated. The memory size
   * returned is guaranteed to be greater than or equal to the
   * memory required to represent the data (e.g., extra space in
   * arrays, etc. are not included in the return value).
   */
  unsigned long GetActualMemorySize() override;

  ///@{
  /**
   * Shallow and Deep copy. These copy the data, but not any pipeline connections.
   */
  void ShallowCopy(vtkDataObject* baseSrc) override;
  void DeepCopy(vtkDataObject* baseSrc) override;
  ///@}

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
  const std::unordered_map<int, vtkSmartPointer<vtkDataSetAttributes>>& GetArrayGroups() const
  {
    return this->ArrayGroups;
  }
  ///@}

  /**
   * Returns the ghost arrays of the data object of the specified
   * attribute type. This may return a null pointer.
   */
  vtkUnsignedCharArray* GetGhostArray(int type) override;

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
        result = ok.first.GetPointer();
      }
    }
    return result;
  }

  vtkCellMetadata* AddCellMetadata(vtkCellMetadata* cellType);
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
    return static_cast<const CellType*>(it->second);
  }
  template <typename CellType>
  CellType* GetCellsOfType()
  {
    auto it = this->Cells.find(vtkStringToken(vtk::TypeName<CellType>()).GetId());
    if (it == this->Cells.end())
    {
      return nullptr;
    }
    return static_cast<CellType*>(it->second);
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
  std::vector<vtkStringToken> CellTypeArray() const
  {
#if defined(_MSC_VER) && _MSC_VER >= 1930 && _MSC_VER < 1940 /*17.4+*/
    // MSVC 2022 bombs when an exported method uses thread_local in its implementation.
    // See https://github.com/pytorch/pytorch/issues/87957 for more. We omit the
    // thread_local here, which makes this method non-threadsafe on Windows, which
    // should be OK in most cases.
    static std::vector<vtkStringToken> cellTypes;
#else
    static thread_local std::vector<vtkStringToken> cellTypes;
#endif
    this->CellTypes(cellTypes);
    return cellTypes;
  }
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

  ///@{
  /**
   * Retrieve an instance of this class from an information object.
   */
  static vtkCellGrid* GetData(vtkInformation* info);
  static vtkCellGrid* GetData(vtkInformationVector* v, int i = 0);
  ///@}

protected:
  vtkCellGrid();
  ~vtkCellGrid() override;

  bool ComputeBoundsInternal();

  std::unordered_map<int, vtkSmartPointer<vtkDataSetAttributes>> ArrayGroups;
  std::unordered_map<CellTypeId, vtkSmartPointer<vtkCellMetadata>> Cells;
  std::unordered_map<vtkStringToken::Hash, vtkSmartPointer<vtkCellAttribute>> Attributes;
  int NextAttribute = 0;
  vtkStringToken ShapeAttribute;
  bool HaveShape{ false };

  mutable std::array<double, 6> CachedBounds;
  mutable vtkTimeStamp CachedBoundsTime;

private:
  vtkCellGrid(const vtkCellGrid&) = delete;
  void operator=(const vtkCellGrid&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
