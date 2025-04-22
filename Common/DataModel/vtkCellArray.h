// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCellArray
 * @brief   object to represent cell connectivity
 *
 * vtkCellArray stores dataset topologies as an explicit connectivity table
 * listing the point ids that make up each cell.
 *
 * Internally, the connectivity table is represented as two arrays: Offsets and
 * Connectivity.
 *
 * Offsets is an array of [numCells+1] values indicating the index in the
 * Connectivity array where each cell's points start. The last value is always
 * the length of the Connectivity array.
 *
 * The Connectivity array stores the lists of point ids for each cell.
 *
 * Thus, for a dataset consisting of 2 triangles, a quad, and a line, the
 * internal arrays will appear as follows:
 *
 * ```
 * Topology:
 * ---------
 * Cell 0: Triangle | point ids: {0, 1, 2}
 * Cell 1: Triangle | point ids: {5, 7, 2}
 * Cell 2: Quad     | point ids: {3, 4, 6, 7}
 * Cell 3: Line     | point ids: {5, 8}
 *
 * vtkCellArray (current):
 * -----------------------
 * Offsets:      {0, 3, 6, 10, 12}
 * Connectivity: {0, 1, 2, 5, 7, 2, 3, 4, 6, 7, 5, 8}
 * ```
 *
 * While this class provides traversal methods (the legacy InitTraversal(),
 * GetNextCell() methods, and the newer method GetCellAtId()) these are in
 * general not thread-safe. Whenever possible it is preferable to use a
 * local thread-safe, vtkCellArrayIterator object, which can be obtained via:
 *
 * ```
 * auto iter = vtk::TakeSmartPointer(cellArray->NewIterator());
 * for (iter->GoToFirstCell(); !iter->IsDoneWithTraversal(); iter->GoToNextCell())
 * {
 *   // do work with iter
 * }
 * ```
 * (Note however that depending on the type and structure of internal
 * storage, a cell array iterator may be significantly slower than direct
 * traversal over the cell array due to extra data copying. Factors of 3-4X
 * are not uncommon. See vtkCellArrayIterator for more information. Also note
 * that an iterator may become invalid if the internal vtkCellArray storage
 * is modified.)
 *
 * Other methods are also available for allocation and memory-related
 * management; insertion of new cells into the vtkCellArray; and limited
 * editing operations such as replacing one cell with a new cell of the
 * same size.
 *
 * The internal arrays may store either 32- or 64-bit values, though most of
 * the API will prefer to use vtkIdType to refer to items in these
 * arrays. This enables significant memory savings when vtkIdType is 64-bit,
 * but 32 bits are sufficient to store all of the values in the connectivity
 * table. Using 64-bit storage with a 32-bit vtkIdType is permitted, but
 * values too large to fit in a 32-bit signed integer will be truncated when
 * accessed through the API. (The particular internal storage type has
 * implications on performance depending on vtkIdType. If the internal
 * storage is equivalent to vtkIdType, then methods that return pointers to
 * arrays of point ids can share the internal storage; otherwise a copy of
 * internal memory must be performed.)
 *
 * Methods for managing the storage type are:
 *
 * - `bool IsStorage64Bit()`
 * - `bool IsStorageShareable() // Can pointers to internal storage be shared`
 * - `void Use32BitStorage()`
 * - `void Use64BitStorage()`
 * - `void UseDefaultStorage() // Depends on vtkIdType`
 * - `bool CanConvertTo32BitStorage()`
 * - `bool CanConvertTo64BitStorage()`
 * - `bool CanConvertToDefaultStorage() // Depends on vtkIdType`
 * - `bool ConvertTo32BitStorage()`
 * - `bool ConvertTo64BitStorage()`
 * - `bool ConvertToDefaultStorage() // Depends on vtkIdType`
 * - `bool ConvertToSmallestStorage() // Depends on current values in arrays`
 *
 * Note that some legacy methods are still available that reflect the
 * previous storage format of this data, which embedded the cell sizes into
 * the Connectivity array:
 *
 * ```
 * vtkCellArray (legacy):
 * ----------------------
 * Connectivity: {3, 0, 1, 2, 3, 5, 7, 2, 4, 3, 4, 6, 7, 2, 5, 8}
 *                |--Cell 0--||--Cell 1--||----Cell 2---||--C3-|
 * ```
 *
 * The methods require an external lookup table to allow random access, which
 * was historically stored in the vtkCellTypes object. The following methods in
 * vtkCellArray still support this style of indexing for compatibility
 * purposes, but these are slow as they must perform some complex computations
 * to convert the old "location" into the new "offset" and should be avoided.
 * These methods (and their modern equivalents) are:
 *
 * - GetCell (Prefer GetCellAtId)
 * - GetInsertLocation (Prefer GetNumberOfCells)
 * - GetTraversalLocation (Prefer GetTraversalCellId, or better, NewIterator)
 * - SetTraversalLocation (Prefer SetTraversalLocation, or better, NewIterator)
 * - ReverseCell (Prefer ReverseCellAtId)
 * - ReplaceCell (Prefer ReplaceCellAtId)
 * - SetCells (Use ImportLegacyFormat, or SetData)
 * - GetData (Use ExportLegacyFormat, or Get[Offsets|Connectivity]Array[|32|64])
 *
 * Some other legacy methods were completely removed, such as GetPointer() /
 * WritePointer(), since they are cannot be effectively emulated under the
 * current design. If external code needs to support both the old and new
 * version of the vtkCellArray API, the VTK_CELL_ARRAY_V2 preprocessor
 * definition may be used to detect which API is being compiled against.
 *
 * @sa vtkAbstractCellArray vtkStructuredCellArray vtkCellTypes vtkCellLinks
 */

#ifndef vtkCellArray_h
#define vtkCellArray_h

#include "vtkAbstractCellArray.h"
#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkWrappingHints.h"         // For VTK_MARSHALMANUAL

#include "vtkAOSDataArrayTemplate.h" // Needed for inline methods
#include "vtkCell.h"                 // Needed for inline methods
#include "vtkDataArrayRange.h"       // Needed for inline methods
#include "vtkFeatures.h"             // for VTK_USE_MEMKIND
#include "vtkSmartPointer.h"         // For vtkSmartPointer
#include "vtkTypeInt32Array.h"       // Needed for inline methods
#include "vtkTypeInt64Array.h"       // Needed for inline methods
#include "vtkTypeList.h"             // Needed for ArrayList definition

#include <cassert>          // for assert
#include <initializer_list> // for API
#include <type_traits>      // for std::is_same
#include <utility>          // for std::forward

/**
 * @def VTK_CELL_ARRAY_V2
 * @brief This preprocessor definition indicates that the updated vtkCellArray
 * is being used. It may be used to conditionally switch between old and new
 * API when both must be supported.
 *
 * For example:
 *
 * ```
 * vtkIdType npts;
 *
 * #ifdef VTK_CELL_ARRAY_V2
 * const vtkIdType *pts;
 * #else // VTK_CELL_ARRAY_V2
 * vtkIdType *pts'
 * #endif // VTK_CELL_ARRAY_V2
 *
 * cellArray->GetCell(legacyLocation, npts, pts);
 * ```
 */
#define VTK_CELL_ARRAY_V2

VTK_ABI_NAMESPACE_BEGIN
class vtkCellArrayIterator;
class vtkIdTypeArray;

class VTKCOMMONDATAMODEL_EXPORT VTK_MARSHALMANUAL vtkCellArray : public vtkAbstractCellArray
{
public:
  using ArrayType32 = vtkTypeInt32Array;
  using ArrayType64 = vtkTypeInt64Array;

  ///@{
  /**
   * Standard methods for instantiation, type information, and
   * printing.
   */
  static vtkCellArray* New();
  vtkTypeMacro(vtkCellArray, vtkAbstractCellArray);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  void PrintDebug(ostream& os);
  ///@}

  /**
   * List of possible array types used for storage. May be used with
   * vtkArrayDispatch::Dispatch[2]ByArray to process internal arrays.
   * Both the Connectivity and Offset arrays are guaranteed to have the same
   * type.
   *
   * @sa vtkCellArray::Visit() for a simpler mechanism.
   */
  using StorageArrayList = vtkTypeList::Create<ArrayType32, ArrayType64>;

  /**
   * List of possible ArrayTypes that are compatible with internal storage.
   * Single component AOS-layout arrays holding one of these types may be
   * passed to the method SetData to setup the cell array state.
   *
   * This can be used with vtkArrayDispatch::DispatchByArray, etc to
   * check input arrays before assigning them to a cell array.
   */
  using InputArrayList =
    typename vtkTypeList::Unique<vtkTypeList::Create<vtkAOSDataArrayTemplate<int>,
      vtkAOSDataArrayTemplate<long>, vtkAOSDataArrayTemplate<long long>>>::Result;

  /**
   * Allocate memory.
   *
   * This currently allocates both the offsets and connectivity arrays to @a sz.
   *
   * @note It is preferable to use AllocateEstimate(numCells, maxCellSize)
   * or AllocateExact(numCells, connectivitySize) instead.
   */
  vtkTypeBool Allocate(vtkIdType sz, vtkIdType vtkNotUsed(ext) = 1000)
  {
    return this->AllocateExact(sz, sz) ? 1 : 0;
  }

  /**
   * @brief Pre-allocate memory in internal data structures. Does not change
   * the number of cells, only the array capacities. Existing data is NOT
   * preserved.
   * @param numCells The number of expected cells in the dataset.
   * @param maxCellSize The number of points per cell to allocate memory for.
   * @return True if allocation succeeds.
   * @sa Squeeze AllocateExact AllocateCopy
   */
  bool AllocateEstimate(vtkIdType numCells, vtkIdType maxCellSize)
  {
    return this->AllocateExact(numCells, numCells * maxCellSize);
  }

  /**
   * @brief Pre-allocate memory in internal data structures. Does not change
   * the number of cells, only the array capacities. Existing data is NOT
   * preserved.
   * @param numCells The number of expected cells in the dataset.
   * @param connectivitySize The total number of pointIds stored for all cells.
   * @return True if allocation succeeds.
   * @sa Squeeze AllocateEstimate AllocateCopy
   */
  bool AllocateExact(vtkIdType numCells, vtkIdType connectivitySize);

  /**
   * @brief Pre-allocate memory in internal data structures to match the used
   * size of the input vtkCellArray. Does not change
   * the number of cells, only the array capacities. Existing data is NOT
   * preserved.
   * @param other The vtkCellArray to use as a reference.
   * @return True if allocation succeeds.
   * @sa Squeeze AllocateEstimate AllocateExact
   */
  bool AllocateCopy(vtkCellArray* other)
  {
    return this->AllocateExact(other->GetNumberOfCells(), other->GetNumberOfConnectivityIds());
  }

  /**
   * @brief ResizeExact() resizes the internal structures to hold @a numCells
   * total cell offsets and @a connectivitySize total pointIds. Old data is
   * preserved, and newly-available memory is not initialized.
   *
   * @warning For advanced use only. You probably want an Allocate method.
   *
   * @return True if allocation succeeds.
   */
  bool ResizeExact(vtkIdType numCells, vtkIdType connectivitySize);

  /**
   * Free any memory and reset to an empty state.
   */
  void Initialize() override;

  /**
   * Reuse list. Reset to initial state without freeing memory.
   */
  void Reset();

  /**
   * Reclaim any extra memory while preserving data.
   *
   * @sa ConvertToSmallestStorage
   */
  void Squeeze();

  /**
   * Check that internal storage is consistent and in a valid state.
   *
   * Specifically, this function returns true if and only if:
   * - The offset and connectivity arrays have exactly one component.
   * - The offset array has at least one value and starts at 0.
   * - The offset array values never decrease.
   * - The connectivity array has as many entries as the last value in the
   *   offset array.
   */
  bool IsValid();

  /**
   * Get the number of cells in the array.
   */
  vtkIdType GetNumberOfCells() const override
  {
    if (this->Storage.Is64Bit())
    {
      return this->Storage.GetArrays64().Offsets->GetNumberOfValues() - 1;
    }
    else
    {
      return this->Storage.GetArrays32().Offsets->GetNumberOfValues() - 1;
    }
  }

  /**
   * Get the number of elements in the offsets array. This will be the number of
   * cells + 1.
   */
  vtkIdType GetNumberOfOffsets() const override
  {
    if (this->Storage.Is64Bit())
    {
      return this->Storage.GetArrays64().Offsets->GetNumberOfValues();
    }
    else
    {
      return this->Storage.GetArrays32().Offsets->GetNumberOfValues();
    }
  }

  /**
   * Get the offset (into the connectivity) for a specified cell id.
   */
  vtkIdType GetOffset(vtkIdType cellId) override
  {
    if (this->Storage.Is64Bit())
    {
      return this->Storage.GetArrays64().Offsets->GetValue(cellId);
    }
    else
    {
      return this->Storage.GetArrays32().Offsets->GetValue(cellId);
    }
  }

  /**
   * Set the offset (into the connectivity) for a specified cell id.
   */
  void SetOffset(vtkIdType cellId, vtkIdType offset)
  {
    if (this->Storage.Is64Bit())
    {
      this->Storage.GetArrays64().Offsets->SetValue(cellId, offset);
    }
    else
    {
      this->Storage.GetArrays32().Offsets->SetValue(cellId, offset);
    }
  }

  /**
   * Get the size of the connectivity array that stores the point ids.
   * @note Do not confuse this with the deprecated
   * GetNumberOfConnectivityEntries(), which refers to the legacy memory
   * layout.
   */
  vtkIdType GetNumberOfConnectivityIds() const override
  {
    if (this->Storage.Is64Bit())
    {
      return this->Storage.GetArrays64().Connectivity->GetNumberOfValues();
    }
    else
    {
      return this->Storage.GetArrays32().Connectivity->GetNumberOfValues();
    }
  }

  /**
   * @brief NewIterator returns a new instance of vtkCellArrayIterator that
   * is initialized to point at the first cell's data. The caller is responsible
   * for Delete()'ing the object.
   */
  VTK_NEWINSTANCE vtkCellArrayIterator* NewIterator();

  /**
   * Set the internal data arrays to the supplied offsets and connectivity
   * arrays.
   *
   * Note that the input arrays may be copied and not used directly. To avoid
   * copying, use vtkIdTypeArray, vtkCellArray::ArrayType32, or
   * vtkCellArray::ArrayType64.
   *
   * @{
   */
  void SetData(vtkIdTypeArray* offsets, vtkIdTypeArray* connectivity);
  void SetData(vtkAOSDataArrayTemplate<int>* offsets, vtkAOSDataArrayTemplate<int>* connectivity);
  void SetData(vtkAOSDataArrayTemplate<long>* offsets, vtkAOSDataArrayTemplate<long>* connectivity);
  void SetData(
    vtkAOSDataArrayTemplate<long long>* offsets, vtkAOSDataArrayTemplate<long long>* connectivity);
  void SetData(vtkTypeInt32Array* offsets, vtkTypeInt32Array* connectivity);
  void SetData(vtkTypeInt64Array* offsets, vtkTypeInt64Array* connectivity);
  /**@}*/

  /**
   * Sets the internal arrays to the supplied offsets and connectivity arrays.
   *
   * This is a convenience method, and may fail if the following conditions
   * are not met:
   *
   * - Both arrays must be of the same type.
   * - The array type must be one of the types in InputArrayList.
   *
   * If invalid arrays are passed in, an error is logged and the function
   * will return false.
   */
  bool SetData(vtkDataArray* offsets, vtkDataArray* connectivity);

  /**
   * Sets the internal arrays to the supported connectivity array with an
   * offsets array automatically generated given the fixed cells size.
   *
   * This is a convenience method, and may fail if the following conditions
   * are not met:
   *
   * - The `connectivity` array must be one of the types in InputArrayList.
   * - The `connectivity` array size must be a multiple of `cellSize`.
   *
   * If invalid arrays are passed in, an error is logged and the function
   * will return false.
   */
  bool SetData(vtkIdType cellSize, vtkDataArray* connectivity);

  /**
   * @return True if the internal storage is using 64 bit arrays. If false,
   * the storage is using 32 bit arrays.
   */
  bool IsStorage64Bit() const { return this->Storage.Is64Bit(); }

  /**
   * @return True if the internal storage can be shared as a
   * pointer to vtkIdType, i.e., the type and organization of internal
   * storage is such that copying of data can be avoided, and instead
   * a pointer to vtkIdType can be used.
   */
  bool IsStorageShareable() const override
  {
    if (this->Storage.Is64Bit())
    {
      return VisitState<ArrayType64>::ValueTypeIsSameAsIdType;
    }
    else
    {
      return VisitState<ArrayType32>::ValueTypeIsSameAsIdType;
    }
  }

  /**
   * Initialize internal data structures to use 32- or 64-bit storage.
   * If selecting default storage, the storage depends on the VTK_USE_64BIT_IDS
   * setting.
   *
   * All existing data is erased.
   * @{
   */
  void Use32BitStorage();
  void Use64BitStorage();
  void UseDefaultStorage();
  /**@}*/

  /**
   * Check if the existing data can safely be converted to use 32- or 64- bit
   * storage. Ensures that all values can be converted to the target storage
   * without truncating.
   * If selecting default storage, the storage depends on the VTK_USE_64BIT_IDS
   * setting.
   * @{
   */
  bool CanConvertTo32BitStorage() const;
  bool CanConvertTo64BitStorage() const;
  bool CanConvertToDefaultStorage() const;
  /**@}*/

  /**
   * Convert internal data structures to use 32- or 64-bit storage.
   *
   * If selecting default storage, the storage depends on the VTK_USE_64BIT_IDS
   * setting.
   *
   * If selecting smallest storage, the data is checked to see what the smallest
   * safe storage for the existing data is, and then converts to it.
   *
   * Existing data is preserved.
   *
   * @return True on success, false on failure. If this algorithm fails, the
   * cell array will be in an unspecified state.
   *
   * @{
   */
  bool ConvertTo32BitStorage();
  bool ConvertTo64BitStorage();
  bool ConvertToDefaultStorage();
  bool ConvertToSmallestStorage();
  /**@}*/

  /**
   * Return the array used to store cell offsets. The 32/64 variants are only
   * valid when IsStorage64Bit() returns the appropriate value.
   * @{
   */
  vtkDataArray* GetOffsetsArray()
  {
    if (this->Storage.Is64Bit())
    {
      return this->GetOffsetsArray64();
    }
    else
    {
      return this->GetOffsetsArray32();
    }
  }
  ArrayType32* GetOffsetsArray32() { return this->Storage.GetArrays32().Offsets; }
  ArrayType64* GetOffsetsArray64() { return this->Storage.GetArrays64().Offsets; }
  /**@}*/

  /**
   * Return the array used to store the point ids that define the cells'
   * connectivity. The 32/64 variants are only valid when IsStorage64Bit()
   * returns the appropriate value.
   * @{
   */
  vtkDataArray* GetConnectivityArray()
  {
    if (this->Storage.Is64Bit())
    {
      return this->GetConnectivityArray64();
    }
    else
    {
      return this->GetConnectivityArray32();
    }
  }
  ArrayType32* GetConnectivityArray32() { return this->Storage.GetArrays32().Connectivity; }
  ArrayType64* GetConnectivityArray64() { return this->Storage.GetArrays64().Connectivity; }
  /**@}*/

  /**
   * Check if all cells have the same number of vertices.
   *
   * The return value is coded as:
   * * -1 = heterogeneous
   * * 0 = Cell array empty
   * * n (positive integer) = homogeneous array of cell size n
   */
  vtkIdType IsHomogeneous() override;

  /**
   * @warning This method is not thread-safe. Consider using the NewIterator()
   * iterator instead.
   *
   * InitTraversal() initializes the traversal of the list of cells.
   *
   * @note This method is not thread-safe and has tricky syntax to use
   * correctly. Prefer the use of vtkCellArrayIterator (see NewIterator()).
   */
  void InitTraversal();

  /**
   * @warning This method is not thread-safe. Consider using the NewIterator()
   * iterator instead.
   *
   * GetNextCell() gets the next cell in the list. If end of list
   * is encountered, 0 is returned. A value of 1 is returned whenever
   * npts and pts have been updated without error.
   *
   * Do not modify the returned @a pts pointer, as it may point to shared
   * memory.
   *
   * @note This method is not thread-safe and has tricky syntax to use
   * correctly. Prefer the use of vtkCellArrayIterator (see NewIterator()).
   */
  int GetNextCell(vtkIdType& npts, vtkIdType const*& pts) VTK_SIZEHINT(pts, npts);

  /**
   * @warning This method is not thread-safe. Consider using the NewIterator()
   * iterator instead.
   *
   * GetNextCell() gets the next cell in the list. If end of list is
   * encountered, 0 is returned.
   *
   * @note This method is not thread-safe and has tricky syntax to use
   * correctly. Prefer the use of vtkCellArrayIterator (see NewIterator()).
   */
  int GetNextCell(vtkIdList* pts);

  /**
   * Return the point ids for the cell at @a cellId.
   *
   * Subsequent calls to this method may invalidate previous call
   * results if the internal storage type is not the same as vtkIdType and
   * cannot be shared through the @a cellPoints pointer. If that occurs,
   * the method will use ptIds, which is an object that is created by each thread,
   * to guarantee thread safety.
   */
  using vtkAbstractCellArray::GetCellAtId;
  void GetCellAtId(vtkIdType cellId, vtkIdType& cellSize, vtkIdType const*& cellPoints,
    vtkIdList* ptIds) VTK_SIZEHINT(cellPoints, cellSize)
    VTK_EXPECTS(0 <= cellId && cellId < GetNumberOfCells()) override;

  /**
   * Return the point ids for the cell at @a cellId. This always copies
   * the cell ids (i.e., the list of points @a pts into the supplied
   * vtkIdList). This method is thread safe.
   */
  void GetCellAtId(vtkIdType cellId, vtkIdList* pts)
    VTK_EXPECTS(0 <= cellId && cellId < GetNumberOfCells()) override;

  /**
   * Return the point ids for the cell at @a cellId. This always copies
   * the cell ids into cellSize and cellPoints. This method is thread safe.
   *
   * Note: the cellPoints need to have the correct size already allocated otherwise memory
   * issues can occur.
   */
  void GetCellAtId(vtkIdType cellId, vtkIdType& cellSize, vtkIdType* cellPoints) VTK_SIZEHINT(
    cellPoints, cellSize) VTK_EXPECTS(0 <= cellId && cellId < GetNumberOfCells()) override;

  /**
   * Return the point id at @a cellPointIndex for the cell at @a cellId.
   */
  vtkIdType GetCellPointAtId(vtkIdType cellId, vtkIdType cellPointIndex) const
    VTK_EXPECTS(0 <= cellId && cellId < GetNumberOfCells())
      VTK_EXPECTS(0 <= cellPointIndex && cellPointIndex < this->GetCellSize(cellId));

  /**
   * Return the size of the cell at @a cellId.
   */
  vtkIdType GetCellSize(vtkIdType cellId) const override;

  /**
   * Insert a cell object. Return the cell id of the cell.
   */
  vtkIdType InsertNextCell(vtkCell* cell);

  /**
   * Create a cell by specifying the number of points and an array of point
   * id's.  Return the cell id of the cell.
   */
  vtkIdType InsertNextCell(vtkIdType npts, const vtkIdType* pts) VTK_SIZEHINT(pts, npts);

  /**
   * Create a cell by specifying a list of point ids. Return the cell id of
   * the cell.
   */
  vtkIdType InsertNextCell(vtkIdList* pts);

  /**
   * Overload that allows `InsertNextCell({0, 1, 2})` syntax.
   *
   * @warning This approach is useful for testing, but beware that trying to
   * pass a single value (eg. `InsertNextCell({3})`) will call the
   * `InsertNextCell(int)` overload instead.
   */
  vtkIdType InsertNextCell(const std::initializer_list<vtkIdType>& cell)
  {
    return this->InsertNextCell(static_cast<vtkIdType>(cell.size()), cell.begin());
  }

  /**
   * Create cells by specifying a count of total points to be inserted, and
   * then adding points one at a time using method InsertCellPoint(). If you
   * don't know the count initially, use the method UpdateCellCount() to
   * complete the cell. Return the cell id of the cell.
   */
  vtkIdType InsertNextCell(int npts);

  /**
   * Used in conjunction with InsertNextCell(npts) to add another point
   * to the list of cells.
   */
  void InsertCellPoint(vtkIdType id);

  /**
   * Used in conjunction with InsertNextCell(int npts) and InsertCellPoint() to
   * update the number of points defining the cell.
   */
  void UpdateCellCount(int npts);

  /**
   * Get/Set the current cellId for traversal.
   *
   * @note This method is not thread-safe and has tricky syntax to use
   * correctly. Prefer the use of vtkCellArrayIterator (see NewIterator()).
   * @{
   */
  vtkIdType GetTraversalCellId();
  void SetTraversalCellId(vtkIdType cellId);
  /**@}*/

  /**
   * Reverses the order of the point ids for the specified cell.
   */
  void ReverseCellAtId(vtkIdType cellId) VTK_EXPECTS(0 <= cellId && cellId < GetNumberOfCells());

  /**
   * Replaces the point ids for the specified cell with the supplied list.
   *
   * @warning This can ONLY replace the cell if the size does not change.
   * Attempting to change cell size through this method will have undefined
   * results.
   * @{
   */
  void ReplaceCellAtId(vtkIdType cellId, vtkIdList* list);
  void ReplaceCellAtId(vtkIdType cellId, vtkIdType cellSize, const vtkIdType* cellPoints)
    VTK_EXPECTS(0 <= cellId && cellId < GetNumberOfCells()) VTK_SIZEHINT(cellPoints, cellSize);
  /**@}*/

  /**
   * Replaces the pointId at cellPointIndex of a cell with newPointId.
   *
   * @warning This can ONLY replace the cell if the size does not change.
   * Attempting to change cell size through this method will have undefined
   * results.
   */
  void ReplaceCellPointAtId(vtkIdType cellId, vtkIdType cellPointIndex, vtkIdType newPointId);

  /**
   * Overload that allows `ReplaceCellAtId(cellId, {0, 1, 2})` syntax.
   *
   * @warning This can ONLY replace the cell if the size does not change.
   * Attempting to change cell size through this method will have undefined
   * results.
   */
  void ReplaceCellAtId(vtkIdType cellId, const std::initializer_list<vtkIdType>& cell)
  {
    this->ReplaceCellAtId(cellId, static_cast<vtkIdType>(cell.size()), cell.begin());
  }

  /**
   * Returns the size of the largest cell. The size is the number of points
   * defining the cell.
   */
  int GetMaxCellSize() override;

  /**
   * Perform a deep copy (no reference counting) of the given cell array.
   */
  void DeepCopy(vtkAbstractCellArray* ca) override;

  /**
   * Shallow copy @a ca into this cell array.
   */
  void ShallowCopy(vtkAbstractCellArray* ca) override;

  /**
   * Append cells from src into this. Point ids are offset by @a pointOffset.
   */
  void Append(vtkCellArray* src, vtkIdType pointOffset = 0);

  /**
   * Fill @a data with the old-style vtkCellArray data layout, e.g.
   *
   * ```
   * { n0, p0_0, p0_1, ..., p0_n, n1, p1_0, p1_1, ..., p1_n, ... }
   * ```
   *
   * where `n0` is the number of points in cell 0, and `pX_Y` is the Y'th point
   * in cell X.
   */
  void ExportLegacyFormat(vtkIdTypeArray* data);

  /**
   * Import an array of data with the legacy vtkCellArray layout, e.g.:
   *
   * ```
   * { n0, p0_0, p0_1, ..., p0_n, n1, p1_0, p1_1, ..., p1_n, ... }
   * ```
   *
   * where `n0` is the number of points in cell 0, and `pX_Y` is the Y'th point
   * in cell X.
   * @{
   */
  void ImportLegacyFormat(vtkIdTypeArray* data);
  void ImportLegacyFormat(const vtkIdType* data, vtkIdType len) VTK_SIZEHINT(data, len);
  /** @} */

  /**
   * Append an array of data with the legacy vtkCellArray layout, e.g.:
   *
   * ```
   * { n0, p0_0, p0_1, ..., p0_n, n1, p1_0, p1_1, ..., p1_n, ... }
   * ```
   *
   * where `n0` is the number of points in cell 0, and `pX_Y` is the Y'th point
   * in cell X.
   * @{
   */
  void AppendLegacyFormat(vtkIdTypeArray* data, vtkIdType ptOffset = 0);
  void AppendLegacyFormat(const vtkIdType* data, vtkIdType len, vtkIdType ptOffset = 0)
    VTK_SIZEHINT(data, len);
  /** @} */

  /**
   * Return the memory in kibibytes (1024 bytes) consumed by this cell array. Used to
   * support streaming and reading/writing data. The value returned is
   * guaranteed to be greater than or equal to the memory required to
   * actually represent the data represented by this object. The
   * information returned is valid only after the pipeline has
   * been updated.
   */
  unsigned long GetActualMemorySize() const;

  // The following code is used to support

  // The wrappers get understandably confused by some of the template code below
#ifndef __VTK_WRAP__

  // Holds connectivity and offset arrays of the given ArrayType.
  template <typename ArrayT>
  struct VisitState
  {
    using ArrayType = ArrayT;
    using ValueType = typename ArrayType::ValueType;
    using CellRangeType = decltype(vtk::DataArrayValueRange<1>(std::declval<ArrayType>()));

    // We can't just use is_same here, since binary compatible representations
    // (e.g. int and long) are distinct types. Instead, ensure that ValueType
    // is a signed integer the same size as vtkIdType.
    // If this value is true, ValueType pointers may be safely converted to
    // vtkIdType pointers via reinterpret cast.
    static constexpr bool ValueTypeIsSameAsIdType = std::is_integral<ValueType>::value &&
      std::is_signed<ValueType>::value && (sizeof(ValueType) == sizeof(vtkIdType));

    ArrayType* GetOffsets() { return this->Offsets; }
    const ArrayType* GetOffsets() const { return this->Offsets; }

    ArrayType* GetConnectivity() { return this->Connectivity; }
    const ArrayType* GetConnectivity() const { return this->Connectivity; }

    vtkIdType GetNumberOfCells() const;

    vtkIdType GetBeginOffset(vtkIdType cellId) const;

    vtkIdType GetEndOffset(vtkIdType cellId) const;

    vtkIdType GetCellSize(vtkIdType cellId) const;

    CellRangeType GetCellRange(vtkIdType cellId);

    friend class vtkCellArray;

  protected:
    VisitState()
    {
      this->Connectivity = vtkSmartPointer<ArrayType>::New();
      this->Offsets = vtkSmartPointer<ArrayType>::New();
      this->Offsets->InsertNextValue(0);
      if (vtkObjectBase::GetUsingMemkind())
      {
        this->IsInMemkind = true;
      }
    }
    ~VisitState() = default;
    void* operator new(size_t nSize)
    {
      void* r;
#ifdef VTK_USE_MEMKIND
      r = vtkObjectBase::GetCurrentMallocFunction()(nSize);
#else
      r = malloc(nSize);
#endif
      return r;
    }
    void operator delete(void* p)
    {
#ifdef VTK_USE_MEMKIND
      VisitState* a = static_cast<VisitState*>(p);
      if (a->IsInMemkind)
      {
        vtkObjectBase::GetAlternateFreeFunction()(p);
      }
      else
      {
        free(p);
      }
#else
      free(p);
#endif
    }

    vtkSmartPointer<ArrayType> Connectivity;
    vtkSmartPointer<ArrayType> Offsets;

  private:
    VisitState(const VisitState&) = delete;
    VisitState& operator=(const VisitState&) = delete;
    bool IsInMemkind = false;
  };

private: // Helpers that allow Visit to return a value:
  template <typename Functor, typename... Args>
  using GetReturnType = decltype(std::declval<Functor>()(
    std::declval<VisitState<ArrayType32>&>(), std::declval<Args>()...));

  template <typename Functor, typename... Args>
  struct ReturnsVoid : std::is_same<GetReturnType<Functor, Args...>, void>
  {
  };

public:
  /**
   * @warning Advanced use only.
   *
   * The Visit methods allow efficient bulk modification of the vtkCellArray
   * internal arrays by dispatching a functor with the current storage arrays.
   * The simplest functor is of the form:
   *
   * ```
   * // Functor definition:
   * struct Worker
   * {
   *   template <typename CellStateT>
   *   void operator()(CellStateT &state)
   *   {
   *     // Do work on state object
   *   }
   * };
   *
   * // Functor usage:
   * vtkCellArray *cellArray = ...;
   * cellArray->Visit(Worker{});
   * ```
   *
   * where `state` is an instance of the vtkCellArray::VisitState<ArrayT> class,
   * instantiated for the current storage type of the cell array. See that
   * class for usage details.
   *
   * The functor may also:
   * - Return a value from `operator()`
   * - Pass additional arguments to `operator()`
   * - Hold state.
   *
   * A more advanced functor that does these things is shown below, along
   * with its usage. This functor scans a range of cells and returns the largest
   * cell's id:
   *
   * ```
   * struct FindLargestCellInRange
   * {
   *   template <typename CellStateT>
   *   vtkIdType operator()(CellStateT &state,
   *                        vtkIdType rangeBegin,
   *                        vtkIdType rangeEnd)
   *   {
   *     vtkIdType largest = rangeBegin;
   *     vtkIdType largestSize = state.GetCellSize(rangeBegin);
   *     ++rangeBegin;
   *     for (; rangeBegin < rangeEnd; ++rangeBegin)
   *     {
   *       const vtkIdType curSize = state.GetCellSize(rangeBegin);
   *       if (curSize > largestSize)
   *       {
   *         largest = rangeBegin;
   *         largestSize = curSize;
   *       }
   *     }
   *
   *     return largest;
   *   }
   * };
   *
   * // Usage:
   * // Scan cells in range [128, 1024) and return the id of the largest.
   * vtkCellArray cellArray = ...;
   * vtkIdType largest = cellArray->Visit(FindLargestCellInRange{},
   *                                      128, 1024);
   * ```
   * @{
   */
  template <typename Functor, typename... Args,
    typename = typename std::enable_if<ReturnsVoid<Functor, Args...>::value>::type>
  void Visit(Functor&& functor, Args&&... args)
  {
    if (this->Storage.Is64Bit())
    {
      // If you get an error on the next line, a call to Visit(functor, Args...)
      // is being called with arguments that do not match the functor's call
      // signature. See the Visit documentation for details.
      functor(this->Storage.GetArrays64(), std::forward<Args>(args)...);
    }
    else
    {
      // If you get an error on the next line, a call to Visit(functor, Args...)
      // is being called with arguments that do not match the functor's call
      // signature. See the Visit documentation for details.
      functor(this->Storage.GetArrays32(), std::forward<Args>(args)...);
    }
  }

  template <typename Functor, typename... Args,
    typename = typename std::enable_if<ReturnsVoid<Functor, Args...>::value>::type>
  void Visit(Functor&& functor, Args&&... args) const
  {
    if (this->Storage.Is64Bit())
    {
      // If you get an error on the next line, a call to Visit(functor, Args...)
      // is being called with arguments that do not match the functor's call
      // signature. See the Visit documentation for details.
      functor(this->Storage.GetArrays64(), std::forward<Args>(args)...);
    }
    else
    {
      // If you get an error on the next line, a call to Visit(functor, Args...)
      // is being called with arguments that do not match the functor's call
      // signature. See the Visit documentation for details.
      functor(this->Storage.GetArrays32(), std::forward<Args>(args)...);
    }
  }

  template <typename Functor, typename... Args,
    typename = typename std::enable_if<!ReturnsVoid<Functor, Args...>::value>::type>
  GetReturnType<Functor, Args...> Visit(Functor&& functor, Args&&... args)
  {
    if (this->Storage.Is64Bit())
    {
      // If you get an error on the next line, a call to Visit(functor, Args...)
      // is being called with arguments that do not match the functor's call
      // signature. See the Visit documentation for details.
      return functor(this->Storage.GetArrays64(), std::forward<Args>(args)...);
    }
    else
    {
      // If you get an error on the next line, a call to Visit(functor, Args...)
      // is being called with arguments that do not match the functor's call
      // signature. See the Visit documentation for details.
      return functor(this->Storage.GetArrays32(), std::forward<Args>(args)...);
    }
  }
  template <typename Functor, typename... Args,
    typename = typename std::enable_if<!ReturnsVoid<Functor, Args...>::value>::type>
  GetReturnType<Functor, Args...> Visit(Functor&& functor, Args&&... args) const
  {
    if (this->Storage.Is64Bit())
    {
      // If you get an error on the next line, a call to Visit(functor, Args...)
      // is being called with arguments that do not match the functor's call
      // signature. See the Visit documentation for details.
      return functor(this->Storage.GetArrays64(), std::forward<Args>(args)...);
    }
    else
    {
      // If you get an error on the next line, a call to Visit(functor, Args...)
      // is being called with arguments that do not match the functor's call
      // signature. See the Visit documentation for details.
      return functor(this->Storage.GetArrays32(), std::forward<Args>(args)...);
    }
  }

#endif // __VTK_WRAP__

  /** @} */

  /**
   * Control the default internal storage size. Useful for saving memory when
   * most cases can be handled with 32bit indices, but large models may require
   * a run-time switch to 64bit indices.
   * @{
   */
  static bool GetDefaultStorageIs64Bit() { return vtkCellArray::DefaultStorageIs64Bit; }
  static void SetDefaultStorageIs64Bit(bool val) { vtkCellArray::DefaultStorageIs64Bit = val; }
  /** @} */

  //=================== Begin Legacy Methods ===================================
  // These should be deprecated at some point as they are confusing or very slow

  /**
   * Set the number of cells in the array.
   * DO NOT do any kind of allocation, advanced use only.
   *
   * @note This call has no effect.
   */
  virtual void SetNumberOfCells(vtkIdType);

  /**
   * Utility routines help manage memory of cell array. EstimateSize()
   * returns a value used to initialize and allocate memory for array based
   * on number of cells and maximum number of points making up cell.  If
   * every cell is the same size (in terms of number of points), then the
   * memory estimate is guaranteed exact. (If not exact, use Squeeze() to
   * reclaim any extra memory.)
   *
   * @note This method was often misused (e.g. called alone and then
   * discarding the result). Use AllocateEstimate directly instead.
   */
  vtkIdType EstimateSize(vtkIdType numCells, int maxPtsPerCell);

  /**
   * Get the size of the allocated connectivity array.
   *
   * @warning This returns the allocated capacity of the internal arrays as a
   * number of elements, NOT the number of elements in use.
   *
   * @note Method incompatible with current internal storage.
   */
  vtkIdType GetSize();

  /**
   * Return the size of the array that would be returned from
   * ExportLegacyFormat().
   *
   * @note Method incompatible with current internal storage.
   */
  vtkIdType GetNumberOfConnectivityEntries();

  /**
   * Internal method used to retrieve a cell given a legacy offset location.
   *
   * @warning Subsequent calls to this method may invalidate previous call
   * results.
   *
   * @note The location-based API is now a super-slow compatibility layer.
   * Prefer GetCellAtId.
   */
  void GetCell(vtkIdType loc, vtkIdType& npts, const vtkIdType*& pts)
    VTK_EXPECTS(0 <= loc && loc < GetNumberOfConnectivityEntries()) VTK_SIZEHINT(pts, npts);

  /**
   * Internal method used to retrieve a cell given a legacy offset location.
   *
   * @note The location-based API is now a super-slow compatibility layer.
   * Prefer GetCellAtId.
   */
  void GetCell(vtkIdType loc, vtkIdList* pts)
    VTK_EXPECTS(0 <= loc && loc < GetNumberOfConnectivityEntries());

  /**
   * Computes the current legacy insertion location within the internal array.
   * Used in conjunction with GetCell(int loc,...).
   *
   * @note The location-based API is now a super-slow compatibility layer.
   */
  vtkIdType GetInsertLocation(int npts);

  /**
   * Get/Set the current traversal legacy location.
   *
   * @note The location-based API is now a super-slow compatibility layer.
   * Prefer Get/SetTraversalCellId.
   * @{
   */
  vtkIdType GetTraversalLocation();
  vtkIdType GetTraversalLocation(vtkIdType npts);
  void SetTraversalLocation(vtkIdType loc);
  /**@}*/

  /**
   * Special method inverts ordering of cell at the specified legacy location.
   * Must be called carefully or the cell topology may be corrupted.
   *
   * @note The location-based API is now a super-slow compatibility layer.
   * Prefer ReverseCellAtId;
   */
  void ReverseCell(vtkIdType loc) VTK_EXPECTS(0 <= loc && loc < GetNumberOfConnectivityEntries());

  /**
   * Replace the point ids of the cell at the legacy location with a different
   * list of point ids. Calling this method does not mark the vtkCellArray as
   * modified. This is the responsibility of the caller and may be done after
   * multiple calls to ReplaceCell. This call does not support changing the
   * number of points in the cell -- the caller must ensure that the target
   * cell has npts points.
   *
   * @note The location-based API is now a super-slow compatibility layer.
   * Prefer ReplaceCellAtId.
   */
  void ReplaceCell(vtkIdType loc, int npts, const vtkIdType pts[])
    VTK_EXPECTS(0 <= loc && loc < GetNumberOfConnectivityEntries()) VTK_SIZEHINT(pts, npts);

  /**
   * Define multiple cells by providing a connectivity list. The list is in
   * the form (npts,p0,p1,...p(npts-1), repeated for each cell). Be careful
   * using this method because it discards the old cells, and anything
   * referring these cells becomes invalid (for example, if BuildCells() has
   * been called see vtkPolyData).  The traversal location is reset to the
   * beginning of the list; the insertion location is set to the end of the
   * list.
   *
   * @warning The vtkCellArray will not hold a reference to `cells`. This
   * function merely calls ImportLegacyFormat.
   *
   * @note Use ImportLegacyFormat or SetData instead.
   */
  void SetCells(vtkIdType ncells, vtkIdTypeArray* cells);

  /**
   * Return the underlying data as a data array.
   *
   * @warning The returned array is not the actual internal representation used
   * by vtkCellArray. Modifications to the returned array will not change the
   * vtkCellArray's topology.
   *
   * @note Use ExportLegacyFormat, or GetOffsetsArray/GetConnectivityArray
   * instead.
   */
  vtkIdTypeArray* GetData();

  //=================== End Legacy Methods =====================================

  friend class vtkCellArrayIterator;

protected:
  vtkCellArray();
  ~vtkCellArray() override;

  // Encapsulates storage of the internal arrays as a discriminated union
  // between 32-bit and 64-bit storage.
  struct Storage
  {
    // Union type that switches 32 and 64 bit array storage
    union ArraySwitch
    {
      ArraySwitch() = default;  // handled by Storage
      ~ArraySwitch() = default; // handle by Storage
      VisitState<ArrayType32>* Int32;
      VisitState<ArrayType64>* Int64;
    };

    Storage()
    {
#ifdef VTK_USE_MEMKIND
      this->Arrays =
        static_cast<ArraySwitch*>(vtkObjectBase::GetCurrentMallocFunction()(sizeof(ArraySwitch)));
#else
      this->Arrays = new ArraySwitch;
#endif

      // Default can be changed, to save memory
      if (vtkCellArray::GetDefaultStorageIs64Bit())
      {
        this->Arrays->Int64 = new VisitState<ArrayType64>;
        this->StorageIs64Bit = true;
      }
      else
      {
        this->Arrays->Int32 = new VisitState<ArrayType32>;
        this->StorageIs64Bit = false;
      }

#ifdef VTK_USE_MEMKIND
      if (vtkObjectBase::GetUsingMemkind())
      {
        this->IsInMemkind = true;
      }
#else
      (void)this->IsInMemkind; // comp warning workaround
#endif
    }

    ~Storage()
    {
      if (this->StorageIs64Bit)
      {
        this->Arrays->Int64->~VisitState();
        delete this->Arrays->Int64;
      }
      else
      {
        this->Arrays->Int32->~VisitState();
        delete this->Arrays->Int32;
      }
#ifdef VTK_USE_MEMKIND
      if (this->IsInMemkind)
      {
        vtkObjectBase::GetAlternateFreeFunction()(this->Arrays);
      }
      else
      {
        free(this->Arrays);
      }
#else
      delete this->Arrays;
#endif
    }

    // Switch the internal arrays to be 32-bit. Any old data is lost. Returns
    // true if the storage changes.
    bool Use32BitStorage()
    {
      if (!this->StorageIs64Bit)
      {
        return false;
      }

      this->Arrays->Int64->~VisitState();
      delete this->Arrays->Int64;
      this->Arrays->Int32 = new VisitState<ArrayType32>;
      this->StorageIs64Bit = false;

      return true;
    }

    // Switch the internal arrays to be 64-bit. Any old data is lost. Returns
    // true if the storage changes.
    bool Use64BitStorage()
    {
      if (this->StorageIs64Bit)
      {
        return false;
      }

      this->Arrays->Int32->~VisitState();
      delete this->Arrays->Int32;
      this->Arrays->Int64 = new VisitState<ArrayType64>;
      this->StorageIs64Bit = true;

      return true;
    }

    // Returns true if the storage is currently configured to be 64 bit.
    bool Is64Bit() const { return this->StorageIs64Bit; }

    // Get the VisitState for 32-bit arrays
    VisitState<ArrayType32>& GetArrays32()
    {
      assert(!this->StorageIs64Bit);
      return *this->Arrays->Int32;
    }

    const VisitState<ArrayType32>& GetArrays32() const
    {
      assert(!this->StorageIs64Bit);
      return *this->Arrays->Int32;
    }

    // Get the VisitState for 64-bit arrays
    VisitState<ArrayType64>& GetArrays64()
    {
      assert(this->StorageIs64Bit);
      return *this->Arrays->Int64;
    }

    const VisitState<ArrayType64>& GetArrays64() const
    {
      assert(this->StorageIs64Bit);
      return *this->Arrays->Int64;
    }

  private:
    // Access restricted to ensure proper union construction/destruction thru
    // API.
    ArraySwitch* Arrays;
    bool StorageIs64Bit;
    bool IsInMemkind = false;
  };

  Storage Storage;
  vtkIdType TraversalCellId{ 0 };

  vtkNew<vtkIdTypeArray> LegacyData; // For GetData().

  static bool DefaultStorageIs64Bit;

private:
  vtkCellArray(const vtkCellArray&) = delete;
  void operator=(const vtkCellArray&) = delete;
};

template <typename ArrayT>
vtkIdType vtkCellArray::VisitState<ArrayT>::GetNumberOfCells() const
{
  return this->Offsets->GetNumberOfValues() - 1;
}

template <typename ArrayT>
vtkIdType vtkCellArray::VisitState<ArrayT>::GetBeginOffset(vtkIdType cellId) const
{
  return static_cast<vtkIdType>(this->Offsets->GetValue(cellId));
}

template <typename ArrayT>
vtkIdType vtkCellArray::VisitState<ArrayT>::GetEndOffset(vtkIdType cellId) const
{
  return static_cast<vtkIdType>(this->Offsets->GetValue(cellId + 1));
}

template <typename ArrayT>
vtkIdType vtkCellArray::VisitState<ArrayT>::GetCellSize(vtkIdType cellId) const
{
  return this->GetEndOffset(cellId) - this->GetBeginOffset(cellId);
}

template <typename ArrayT>
typename vtkCellArray::VisitState<ArrayT>::CellRangeType
vtkCellArray::VisitState<ArrayT>::GetCellRange(vtkIdType cellId)
{
  return vtk::DataArrayValueRange<1>(
    this->GetConnectivity(), this->GetBeginOffset(cellId), this->GetEndOffset(cellId));
}
VTK_ABI_NAMESPACE_END

namespace vtkCellArray_detail
{
VTK_ABI_NAMESPACE_BEGIN

struct InsertNextCellImpl
{
  // Insert full cell
  template <typename CellStateT>
  vtkIdType operator()(CellStateT& state, const vtkIdType npts, const vtkIdType pts[])
  {
    using ValueType = typename CellStateT::ValueType;
    auto* conn = state.GetConnectivity();
    auto* offsets = state.GetOffsets();

    const vtkIdType cellId = offsets->GetNumberOfValues() - 1;

    offsets->InsertNextValue(static_cast<ValueType>(conn->GetNumberOfValues() + npts));

    for (vtkIdType i = 0; i < npts; ++i)
    {
      conn->InsertNextValue(static_cast<ValueType>(pts[i]));
    }

    return cellId;
  }

  // Just update offset table (for incremental API)
  template <typename CellStateT>
  vtkIdType operator()(CellStateT& state, const vtkIdType npts)
  {
    using ValueType = typename CellStateT::ValueType;
    auto* conn = state.GetConnectivity();
    auto* offsets = state.GetOffsets();

    const vtkIdType cellId = offsets->GetNumberOfValues() - 1;

    offsets->InsertNextValue(static_cast<ValueType>(conn->GetNumberOfValues() + npts));

    return cellId;
  }
};

// for incremental API:
struct UpdateCellCountImpl
{
  template <typename CellStateT>
  void operator()(CellStateT& state, const vtkIdType npts)
  {
    using ValueType = typename CellStateT::ValueType;

    auto* offsets = state.GetOffsets();
    const ValueType cellBegin = offsets->GetValue(offsets->GetMaxId() - 1);
    offsets->SetValue(offsets->GetMaxId(), static_cast<ValueType>(cellBegin + npts));
  }
};

struct GetCellSizeImpl
{
  template <typename CellStateT>
  vtkIdType operator()(CellStateT& state, vtkIdType cellId)
  {
    return state.GetCellSize(cellId);
  }
};

struct GetCellAtIdImpl
{
  template <typename CellStateT>
  void operator()(CellStateT& state, const vtkIdType cellId, vtkIdList* ids)
  {
    using ValueType = typename CellStateT::ValueType;

    const vtkIdType beginOffset = state.GetBeginOffset(cellId);
    const vtkIdType endOffset = state.GetEndOffset(cellId);
    const vtkIdType cellSize = endOffset - beginOffset;
    const auto cellConnectivity = state.GetConnectivity()->GetPointer(beginOffset);

    // ValueType differs from vtkIdType, so we have to copy into a temporary buffer:
    ids->SetNumberOfIds(cellSize);
    vtkIdType* idPtr = ids->GetPointer(0);
    for (ValueType i = 0; i < cellSize; ++i)
    {
      idPtr[i] = static_cast<vtkIdType>(cellConnectivity[i]);
    }
  }

  template <typename CellStateT>
  void operator()(
    CellStateT& state, const vtkIdType cellId, vtkIdType& cellSize, vtkIdType* cellPoints)
  {
    using ValueType = typename CellStateT::ValueType;

    const vtkIdType beginOffset = state.GetBeginOffset(cellId);
    const vtkIdType endOffset = state.GetEndOffset(cellId);
    cellSize = endOffset - beginOffset;
    const ValueType* cellConnectivity = state.GetConnectivity()->GetPointer(beginOffset);

    // ValueType differs from vtkIdType, so we have to copy into a temporary buffer:
    for (vtkIdType i = 0; i < cellSize; ++i)
    {
      cellPoints[i] = static_cast<vtkIdType>(cellConnectivity[i]);
    }
  }

  // SFINAE helper to check if a VisitState's connectivity array's memory
  // can be used as a vtkIdType*.
  template <typename CellStateT>
  struct CanShareConnPtr
  {
  private:
    using ValueType = typename CellStateT::ValueType;
    using ArrayType = typename CellStateT::ArrayType;
    using AOSArrayType = vtkAOSDataArrayTemplate<ValueType>;
    static constexpr bool ValueTypeCompat = CellStateT::ValueTypeIsSameAsIdType;
    static constexpr bool ArrayTypeCompat = std::is_base_of<AOSArrayType, ArrayType>::value;

  public:
    static constexpr bool value = ValueTypeCompat && ArrayTypeCompat;
  };

  template <typename CellStateT>
  typename std::enable_if<CanShareConnPtr<CellStateT>::value, void>::type operator()(
    CellStateT& state, const vtkIdType cellId, vtkIdType& cellSize, vtkIdType const*& cellPoints,
    vtkIdList* vtkNotUsed(temp))
  {
    const vtkIdType beginOffset = state.GetBeginOffset(cellId);
    const vtkIdType endOffset = state.GetEndOffset(cellId);
    cellSize = endOffset - beginOffset;
    // This is safe, see CanShareConnPtr helper above.
    cellPoints = reinterpret_cast<vtkIdType*>(state.GetConnectivity()->GetPointer(beginOffset));
  }

  template <typename CellStateT>
  typename std::enable_if<!CanShareConnPtr<CellStateT>::value, void>::type operator()(
    CellStateT& state, const vtkIdType cellId, vtkIdType& cellSize, vtkIdType const*& cellPoints,
    vtkIdList* temp)
  {
    using ValueType = typename CellStateT::ValueType;

    const vtkIdType beginOffset = state.GetBeginOffset(cellId);
    const vtkIdType endOffset = state.GetEndOffset(cellId);
    cellSize = endOffset - beginOffset;
    const ValueType* cellConnectivity = state.GetConnectivity()->GetPointer(beginOffset);

    // ValueType differs from vtkIdType, so we have to copy into a temporary buffer:
    temp->SetNumberOfIds(cellSize);
    vtkIdType* tempPtr = temp->GetPointer(0);
    for (vtkIdType i = 0; i < cellSize; ++i)
    {
      tempPtr[i] = static_cast<vtkIdType>(cellConnectivity[i]);
    }

    cellPoints = temp->GetPointer(0);
  }
};

struct CellPointAtIdImpl
{
  template <typename CellStateT>
  vtkIdType operator()(CellStateT& cells, vtkIdType cellId, vtkIdType cellPointIndex) const
  {
    return static_cast<vtkIdType>(
      cells.GetConnectivity()->GetValue(cells.GetBeginOffset(cellId) + cellPointIndex));
  }
};

struct ResetImpl
{
  template <typename CellStateT>
  void operator()(CellStateT& state)
  {
    state.GetOffsets()->Reset();
    state.GetConnectivity()->Reset();
    state.GetOffsets()->InsertNextValue(0);
  }
};

VTK_ABI_NAMESPACE_END
} // end namespace vtkCellArray_detail

VTK_ABI_NAMESPACE_BEGIN
//----------------------------------------------------------------------------
inline void vtkCellArray::InitTraversal()
{
  this->TraversalCellId = 0;
}

//----------------------------------------------------------------------------
inline int vtkCellArray::GetNextCell(vtkIdType& npts, vtkIdType const*& pts) VTK_SIZEHINT(pts, npts)
{
  if (this->TraversalCellId < this->GetNumberOfCells())
  {
    this->GetCellAtId(this->TraversalCellId, npts, pts);
    ++this->TraversalCellId;
    return 1;
  }

  npts = 0;
  pts = nullptr;
  return 0;
}

//----------------------------------------------------------------------------
inline int vtkCellArray::GetNextCell(vtkIdList* pts)
{
  if (this->TraversalCellId < this->GetNumberOfCells())
  {
    this->GetCellAtId(this->TraversalCellId, pts);
    ++this->TraversalCellId;
    return 1;
  }

  pts->Reset();
  return 0;
}
//----------------------------------------------------------------------------
inline vtkIdType vtkCellArray::GetCellSize(const vtkIdType cellId) const
{
  return this->Visit(vtkCellArray_detail::GetCellSizeImpl{}, cellId);
}

//----------------------------------------------------------------------------
inline void vtkCellArray::GetCellAtId(vtkIdType cellId, vtkIdType& cellSize,
  vtkIdType const*& cellPoints, vtkIdList* ptIds) VTK_SIZEHINT(cellPoints, cellSize)
{
  this->Visit(vtkCellArray_detail::GetCellAtIdImpl{}, cellId, cellSize, cellPoints, ptIds);
}

//----------------------------------------------------------------------------
inline void vtkCellArray::GetCellAtId(vtkIdType cellId, vtkIdList* pts)
{
  this->Visit(vtkCellArray_detail::GetCellAtIdImpl{}, cellId, pts);
}

//----------------------------------------------------------------------------
inline void vtkCellArray::GetCellAtId(vtkIdType cellId, vtkIdType& cellSize, vtkIdType* cellPoints)
{
  this->Visit(vtkCellArray_detail::GetCellAtIdImpl{}, cellId, cellSize, cellPoints);
}

//----------------------------------------------------------------------------
inline vtkIdType vtkCellArray::GetCellPointAtId(vtkIdType cellId, vtkIdType cellPointIndex) const
{
  return this->Visit(vtkCellArray_detail::CellPointAtIdImpl{}, cellId, cellPointIndex);
}

//----------------------------------------------------------------------------
inline vtkIdType vtkCellArray::InsertNextCell(vtkIdType npts, const vtkIdType* pts)
  VTK_SIZEHINT(pts, npts)
{
  return this->Visit(vtkCellArray_detail::InsertNextCellImpl{}, npts, pts);
}

//----------------------------------------------------------------------------
inline vtkIdType vtkCellArray::InsertNextCell(int npts)
{
  return this->Visit(vtkCellArray_detail::InsertNextCellImpl{}, npts);
}

//----------------------------------------------------------------------------
inline void vtkCellArray::InsertCellPoint(vtkIdType id)
{
  if (this->Storage.Is64Bit())
  {
    using ValueType = typename ArrayType64::ValueType;
    this->Storage.GetArrays64().Connectivity->InsertNextValue(static_cast<ValueType>(id));
  }
  else
  {
    using ValueType = typename ArrayType32::ValueType;
    this->Storage.GetArrays32().Connectivity->InsertNextValue(static_cast<ValueType>(id));
  }
}

//----------------------------------------------------------------------------
inline void vtkCellArray::UpdateCellCount(int npts)
{
  this->Visit(vtkCellArray_detail::UpdateCellCountImpl{}, npts);
}

//----------------------------------------------------------------------------
inline vtkIdType vtkCellArray::InsertNextCell(vtkIdList* pts)
{
  return this->Visit(
    vtkCellArray_detail::InsertNextCellImpl{}, pts->GetNumberOfIds(), pts->GetPointer(0));
}

//----------------------------------------------------------------------------
inline vtkIdType vtkCellArray::InsertNextCell(vtkCell* cell)
{
  vtkIdList* pts = cell->GetPointIds();
  return this->Visit(
    vtkCellArray_detail::InsertNextCellImpl{}, pts->GetNumberOfIds(), pts->GetPointer(0));
}

//----------------------------------------------------------------------------
inline void vtkCellArray::Reset()
{
  this->Visit(vtkCellArray_detail::ResetImpl{});
}

VTK_ABI_NAMESPACE_END
#endif // vtkCellArray.h
