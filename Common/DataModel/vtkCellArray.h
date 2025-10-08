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
 * Additionally, the internal arrays may be "fixed size", meaning that
 * each cell is required to have the same number of points. This allows
 * more compact storage and faster traversal, but is only appropriate for
 * datasets where all cells are the same type (e.g., all triangles, or all
 * quads, etc.). Fixed size storage with a cell size of N means that the
 * Connectivity array will have N entries per cell, and the Offsets array
 * will have values {0, N, 2N, 3N, ... }.
 *
 * Methods for managing the storage type are:
 *
 * - `bool IsStorage64Bit()`
 * - `bool IsStorage32Bit()`
 * - `bool IsStorageFixedSize64Bit()`
 * - `bool IsStorageFixedSize32Bit()`
 * - `bool IsStorageFixedSize() // Either 32- or 64-bit fixed size`
 * - `bool IsStorageGeneric()`
 * - `bool IsStorageShareable() // Can pointers to internal storage be shared`
 * - `void Use64BitStorage()`
 * - `void Use32BitStorage()`
 * - `void UseDefaultStorage() // Depends on vtkIdType`
 * - `void UseFixedSize64BitStorage(vtkIdType cellSize)`
 * - `void UseFixedSize32BitStorage(vtkIdType cellSize)`
 * - `void UseFixedSizeStorage(vtkIdType cellSize) // Depends on vtkIdType`
 * - `bool CanConvertTo32BitStorage()`
 * - `bool CanConvertTo64BitStorage()`
 * - `bool CanConvertToDefaultStorage() // Depends on vtkIdType`
 * - `bool CanConvertToFixedSize32BitStorage()`
 * - `bool CanConvertToFixedSize64BitStorage()`
 * - `bool CanConvertToFixedSizeStorage() // Depends on vtkIdType`
 * - `bool CanConvertToStorageType(int type) // Int64, Int32, FixedSizeInt64, FixedSizeInt32`
 * - `bool ConvertTo64BitStorage()`
 * - `bool ConvertTo32BitStorage()`
 * - `bool ConvertToDefaultStorage() // Depends on vtkIdType`
 * - `bool ConvertToFixedSize64BitStorage()`
 * - `bool ConvertToFixedSize32BitStorage()`
 * - `bool ConvertToFixedSizeStorage() // Depends on vtkIdType`
 * - `bool ConvertToSmallestStorage() // Depends on current values in arrays`
 * - `bool ConvertToStorageType(int type) // Int64, Int32, FixedSizeInt64, FixedSizeInt32`
 *
 * There is an additional mode of storage to allow any kind of vtkDataArray derived
 * types. The Generic storage mode allows separate array types for offsets and connectivity.
 * There is no method to explicitly use generic storage mode.
 * vtkCellArray will automatically switch over to using generic
 * storage when any overload of vtkCellArray::SetData is invoked with array types that
 * are NOT in vtkCellArray::InputConnectivityArrays.
 *
 * @sa vtkAbstractCellArray vtkStructuredCellArray vtkCellTypes vtkCellLinks
 */

#ifndef vtkCellArray_h
#define vtkCellArray_h

#include "vtkAbstractCellArray.h"
#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkWrappingHints.h"         // For VTK_MARSHALMANUAL

#include "vtkAffineArray.h"    // Needed for inline methods
#include "vtkCell.h"           // Needed for inline methods
#include "vtkDataArrayRange.h" // Needed for inline methods
#include "vtkDeprecation.h"    // For VTK_DEPRECATED_IN_9_6_0
#include "vtkFeatures.h"       // for VTK_USE_MEMKIND
#include "vtkSmartPointer.h"   // For vtkSmartPointer
#include "vtkTypeInt32Array.h" // Needed for inline methods
#include "vtkTypeInt64Array.h" // Needed for inline methods
#include "vtkTypeList.h"       // Needed for ArrayList definition

#include <cassert>          // Needed for assert
#include <initializer_list> // Needed for API
#include <type_traits>      // Needed for std::is_same
#include <utility>          // Needed for std::forward

VTK_ABI_NAMESPACE_BEGIN
class vtkCellArrayIterator;
class vtkIdTypeArray;

class VTKCOMMONDATAMODEL_EXPORT VTK_MARSHALMANUAL vtkCellArray : public vtkAbstractCellArray
{
public:
  using ArrayType32 = vtkTypeInt32Array;
  using ArrayType64 = vtkTypeInt64Array;
  using AffineArrayType32 = vtkAffineArray<vtkTypeInt32>;
  using AffineArrayType64 = vtkAffineArray<vtkTypeInt64>;

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

  ///@{
  /**
   * List of possible array types used for storage. May be used with
   * vtkArrayDispatch::Dispatch[2]ByArray to process internal arrays.
   * Both the Connectivity and Offset arrays are guaranteed to have the same
   * type.
   *
   * @sa vtkCellArray::Dispatch() for a simpler mechanism.
   */
  using StorageOffsetsArrays =
    vtkTypeList::Create<ArrayType32, ArrayType64, AffineArrayType32, AffineArrayType64>;
  using StorageConnectivityArrays = vtkTypeList::Create<ArrayType32, ArrayType64>;
  using StorageArrayList VTK_DEPRECATED_IN_9_6_0(
    "Use StorageOffsetsArrays/StorageConnectivityArrays instead.") = StorageConnectivityArrays;
  ///@}

  ///@{
  /**
   * List of possible ArrayTypes that are compatible with internal storage.
   * Single component AOS-layout arrays holding one of these types may be
   * passed to the method SetData to setup the cell array state.
   *
   * This can be used with vtkArrayDispatch::DispatchByArray, etc to
   * check input arrays before assigning them to a cell array.
   */
  using InputOffsetsArrays =
    typename vtkTypeList::Unique<vtkTypeList::Create<vtkAOSDataArrayTemplate<int>,
      vtkAOSDataArrayTemplate<long>, vtkAOSDataArrayTemplate<long long>, vtkAffineArray<int>,
      vtkAffineArray<long>, vtkAffineArray<long long>>>::Result;
  using InputConnectivityArrays =
    typename vtkTypeList::Unique<vtkTypeList::Create<vtkAOSDataArrayTemplate<int>,
      vtkAOSDataArrayTemplate<long>, vtkAOSDataArrayTemplate<long long>>>::Result;
  using InputArrayList VTK_DEPRECATED_IN_9_6_0(
    "Use InputOffsetsArrays/InputConnectivityArrays instead.") = InputConnectivityArrays;
  ///@}

  /**
   * Allocate memory.
   *
   * This currently allocates both the offsets and connectivity arrays to @a sz.
   *
   * @note It is preferable to use AllocateEstimate(numCells, maxCellSize)
   * or AllocateExact(numCells, connectivitySize) instead.
   */
  VTK_DEPRECATED_IN_9_6_0("Use AllocateEstimate or AllocateExact instead.")
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
  vtkIdType GetNumberOfCells() const override { return this->Offsets->GetNumberOfValues() - 1; }

  /**
   * Get the number of elements in the offsets array. This will be the number of
   * cells + 1.
   */
  vtkIdType GetNumberOfOffsets() const override { return this->Offsets->GetNumberOfValues(); }

  /**
   * Get the offset (into the connectivity) for a specified cell id.
   */
  vtkIdType GetOffset(vtkIdType cellId) override
  {
    return static_cast<vtkIdType>(this->Offsets->GetComponent(cellId, 0));
  }

  /**
   * Set the offset (into the connectivity) for a specified cell id.
   */
  void SetOffset(vtkIdType cellId, vtkIdType offset)
  {
    this->Offsets->SetComponent(cellId, 0, static_cast<double>(offset));
  }

  /**
   * Get the size of the connectivity array that stores the point ids.
   */
  vtkIdType GetNumberOfConnectivityIds() const override
  {
    return this->Connectivity->GetNumberOfValues();
  }

  /**
   * @brief NewIterator returns a new instance of vtkCellArrayIterator that
   * is initialized to point at the first cell's data. The caller is responsible
   * for Delete()'ing the object.
   */
  VTK_NEWINSTANCE vtkCellArrayIterator* NewIterator();

  ///@{
  /**
   * Set the internal data arrays to the supplied offsets and connectivity
   * arrays.
   *
   * Note that the input arrays may be copied and not used directly. To avoid
   * copying, use vtkIdTypeArray, vtkCellArray::ArrayType32, or
   * vtkCellArray::ArrayType64 for connectivity, and vtkIdTypeArray,
   * vtkCellArray::ArrayType32, vtkCellArray::ArrayType64,
   * vtkCellArray::AffineArrayType32, or vtkCellArray::AffineArrayType64
   * for offsets.
   */
  void SetData(vtkIdTypeArray* offsets, vtkIdTypeArray* connectivity);
  void SetData(vtkAOSDataArrayTemplate<int>* offsets, vtkAOSDataArrayTemplate<int>* connectivity);
  void SetData(vtkAOSDataArrayTemplate<long>* offsets, vtkAOSDataArrayTemplate<long>* connectivity);
  void SetData(
    vtkAOSDataArrayTemplate<long long>* offsets, vtkAOSDataArrayTemplate<long long>* connectivity);
  void SetData(vtkTypeInt32Array* offsets, vtkTypeInt32Array* connectivity);
  void SetData(vtkTypeInt64Array* offsets, vtkTypeInt64Array* connectivity);
  void SetData(vtkAffineArray<vtkIdType>* offsets, vtkIdTypeArray* connectivity);
  void SetData(vtkAffineArray<int>* offsets, vtkAOSDataArrayTemplate<int>* connectivity);
  void SetData(vtkAffineArray<long>* offsets, vtkAOSDataArrayTemplate<long>* connectivity);
  void SetData(
    vtkAffineArray<long long>* offsets, vtkAOSDataArrayTemplate<long long>* connectivity);
  void SetData(vtkAffineArray<vtkTypeInt32>* offsets, vtkTypeInt32Array* connectivity);
  void SetData(vtkAffineArray<vtkTypeInt64>* offsets, vtkTypeInt64Array* connectivity);
  ///@}

  /**
   * Sets the internal arrays to the supplied offsets and connectivity arrays.
   *
   * This is a convenience method, and may fail if the following conditions
   * are not met:
   *
   * - Both arrays must be of the same type.
   * - The offsets array type must be one of the types in InputOffsetsArrays.
   * - The connectivity array type must be one of the types in InputConnectivityArrays.
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
   * - The `connectivity` array must be one of the types in InputConnectivityArrays.
   * - The `connectivity` array size must be a multiple of `cellSize`.
   *
   * If invalid arrays are passed in, an error is logged and the function
   * will return false.
   */
  bool SetData(vtkIdType cellSize, vtkDataArray* connectivity);

  enum StorageTypes
  {
    Int64,
    Int32,
    FixedSizeInt64,
    FixedSizeInt32,
    Generic,
  };

  /**
   * @return Type of internal storage.
   */
  StorageTypes GetStorageType() const noexcept { return this->StorageType; }

  /**
   * @return True if the internal storage is using 64 bit arrays.
   */
  bool IsStorage64Bit() const { return this->StorageType == StorageTypes::Int64; }

  /**
   * @return True if the internal storage is using 32 bit arrays.
   */
  bool IsStorage32Bit() const { return this->StorageType == StorageTypes::Int32; }

  /**
   * @return True if the internal storage is using FixedSize 64 bit arrays.
   */
  bool IsStorageFixedSize64Bit() const { return this->StorageType == StorageTypes::FixedSizeInt64; }

  /**
   * @return True if the internal storage is using FixedSize 32 bit arrays.
   */
  bool IsStorageFixedSize32Bit() const { return this->StorageType == StorageTypes::FixedSizeInt32; }

  /**
   * @return True if the internal storage is using FixedSize arrays.
   */
  bool IsStorageFixedSize() const
  {
    return this->IsStorageFixedSize32Bit() || this->IsStorageFixedSize64Bit();
  }

  /**
   * @return True if the internal storage is using vtkDataArray.
   */
  bool IsStorageGeneric() const { return this->StorageType == StorageTypes::Generic; }

  /**
   * @return True if the internal storage can be shared as a
   * pointer to vtkIdType, i.e., the type and organization of internal
   * storage is such that copying of data can be avoided, and instead
   * a pointer to vtkIdType can be used.
   */
  bool IsStorageShareable() const override
  {
    switch (this->StorageType)
    {
      case StorageTypes::Int32:
      case StorageTypes::FixedSizeInt32:
        return std::is_same_v<vtkTypeInt32, vtkIdType>;
      case StorageTypes::Int64:
      case StorageTypes::FixedSizeInt64:
        return std::is_same_v<vtkTypeInt64, vtkIdType>;
      case StorageTypes::Generic:
      default:
        return false;
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
  void UseFixedSize32BitStorage(vtkIdType cellSize);
  void UseFixedSize64BitStorage(vtkIdType cellSize);
  void UseFixedSizeDefaultStorage(vtkIdType cellSize);
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
  bool CanConvertToFixedSize32BitStorage() const;
  bool CanConvertToFixedSize64BitStorage() const;
  bool CanConvertToFixedSizeDefaultStorage() const;
  bool CanConvertToStorageType(StorageTypes type) const
  {
    switch (type)
    {
      case StorageTypes::Int32:
        return this->CanConvertTo32BitStorage();
      case StorageTypes::Int64:
        return this->CanConvertTo64BitStorage();
      case StorageTypes::FixedSizeInt32:
        return this->CanConvertToFixedSize32BitStorage();
      case StorageTypes::FixedSizeInt64:
        return this->CanConvertToFixedSize64BitStorage();
      case StorageTypes::Generic:
      default:
        return true;
    }
  }
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
  bool ConvertToFixedSize32BitStorage();
  bool ConvertToFixedSize64BitStorage();
  bool ConvertToFixedSizeDefaultStorage();
  bool ConvertToSmallestStorage();
  bool ConvertToStorageType(StorageTypes type)
  {
    switch (type)
    {
      case StorageTypes::Int32:
        return this->ConvertTo32BitStorage();
      case StorageTypes::Int64:
        return this->ConvertTo64BitStorage();
      case StorageTypes::FixedSizeInt32:
        return this->ConvertToFixedSize32BitStorage();
      case StorageTypes::FixedSizeInt64:
        return this->ConvertToFixedSize64BitStorage();
      case StorageTypes::Generic:
      default:
        return true;
    }
  }
  /**@}*/

  /**
   * Return the array used to store cell offsets. The 32/64 variants are only
   * valid when IsStorage64Bit() returns the appropriate value.
   * @{
   */
  vtkDataArray* GetOffsetsArray() const { return this->Offsets; }
  ArrayType32* GetOffsetsArray32() const { return ArrayType32::FastDownCast(this->Offsets); }
  ArrayType64* GetOffsetsArray64() const { return ArrayType64::FastDownCast(this->Offsets); }
  AffineArrayType32* GetOffsetsAffineArray32()
  {
    return AffineArrayType32::FastDownCast(this->Offsets);
  }
  AffineArrayType64* GetOffsetsAffineArray64()
  {
    return AffineArrayType64::FastDownCast(this->Offsets);
  }
  /**@}*/

  /**
   * Return the array used to store the point ids that define the cells'
   * connectivity. The 32/64 variants are only valid when IsStorage64Bit()
   * returns the appropriate value.
   * @{
   */
  vtkDataArray* GetConnectivityArray() const { return this->Connectivity; }
  ArrayType32* GetConnectivityArray32() const
  {
    return ArrayType32::FastDownCast(this->Connectivity);
  }
  ArrayType64* GetConnectivityArray64() const
  {
    return ArrayType64::FastDownCast(this->Connectivity);
  }
  /**@}*/

  /**
   * Check if all cells have the same number of vertices.
   *
   * The return value is coded as:
   * * -1 = heterogeneous
   * * 0 = Cell array empty
   * * n (positive integer) = homogeneous array of cell size n
   */
  vtkIdType IsHomogeneous() const override;

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
  /*
   * Utilities class that every dispatch functor used with Dispatch() must inherit
   * or optionally use its static methods.
   */
  struct DispatchUtilities
  {
    template <class ArrayT>
    using GetAPIType = vtk::GetAPIType<ArrayT, vtkIdType>;

    template <class OffsetsT>
    vtkIdType GetNumberOfCells(OffsetsT* offsets)
    {
      return offsets->GetNumberOfValues() - 1;
    }

    template <class ArrayT>
    static decltype(vtk::DataArrayValueRange<1, vtkIdType>(std::declval<ArrayT>())) GetRange(
      ArrayT* array)
    {
      return vtk::DataArrayValueRange<1, vtkIdType>(array);
    }

    template <class OffsetsT>
    static vtkIdType GetBeginOffset(OffsetsT* offsets, vtkIdType cellId)
    {
      return static_cast<vtkIdType>(GetRange(offsets)[cellId]);
    }

    template <class OffsetsT>
    static vtkIdType GetEndOffset(OffsetsT* offsets, vtkIdType cellId)
    {
      return static_cast<vtkIdType>(GetRange(offsets)[cellId + 1]);
    }

    template <class OffsetsT>
    static vtkIdType GetCellSize(OffsetsT* offsets, vtkIdType cellId)
    {
      auto offsetsRange = GetRange(offsets);
      return static_cast<vtkIdType>(offsetsRange[cellId + 1] - offsetsRange[cellId]);
    }

    template <class OffsetsT, class ConnectivityT>
    static decltype(vtk::DataArrayValueRange<1, vtkIdType>(std::declval<ConnectivityT>()))
    GetCellRange(OffsetsT* offsets, ConnectivityT* conn, vtkIdType cellId)
    {
      auto offsetsRange = GetRange(offsets);
      return vtk::DataArrayValueRange<1, vtkIdType>(
        conn, offsetsRange[cellId], offsetsRange[cellId + 1]);
    }
  };

  ///@{
  /**
   * @warning Advanced use only.
   *
   * The Dispatch methods allow efficient bulk modification of the vtkCellArray
   * internal arrays by dispatching a functor with the actual arrays.
   * The simplest functor is of the form:
   *
   * ```
   * // Functor definition:
   * struct Worker : public vtkCellArray::DispatchUtilities
   * {
   *   template <class OffsetsT, class ConnectivityT>
   *   void operator()(OffsetsT* offsets, ConnectivityT* conn)
   *   {
   *     // Do work on state object
   *   }
   * };
   *
   * // Functor usage:
   * vtkCellArray *cellArray = ...;
   * cellArray->Dispatch(Worker{});
   * ```
   *
   * where DispatchUtilities provides convenience methods to work with the
   * arrays, such as GetRange(), GetCellSize(), GetCellRange(), etc.
   *
   * The functor may also:
   * - "Return" a value from `operator()` using a reference argument.
   * - Pass additional arguments to `operator()`
   *
   * A more advanced functor that does these things is shown below, along
   * with its usage. This functor scans a range of cells and returns the largest
   * cell's id:
   *
   * ```
   * struct FindLargestCellInRange : public vtkCellArray::DispatchUtilities
   * {
   *   template <class OffsetsT, class ConnectivityT>
   *   vtkIdType operator()(OffsetsT* offsets, ConnectivityT* conn,
   *                        vtkIdType rangeBegin, vtkIdType rangeEnd, vtkIdType& largest)
   *   {
   *     largest = rangeBegin;
   *     vtkIdType largestSize = GetCellSize(offsets, rangeBegin);
   *     ++rangeBegin;
   *     for (; rangeBegin < rangeEnd; ++rangeBegin)
   *     {
   *       const vtkIdType curSize = GetCellSize(offsets, rangeBegin);
   *       if (curSize > largestSize)
   *       {
   *         largest = rangeBegin;
   *         largestSize = curSize;
   *       }
   *     }
   *   }
   * };
   *
   * // Usage:
   * // Scan cells in range [128, 1024) and return the id of the largest.
   * vtkCellArray cellArray = ...;
   * vtkIdType largest;
   * cellArray->Dispatch(FindLargestCellInRange{}, 128, 1024, largest);
   * ```
   */
  template <typename Functor, typename... Args>
  void Dispatch(Functor&& functor, Args&&... args)
  {
    switch (this->StorageType)
    {
      case StorageTypes::Int32:
        functor(static_cast<ArrayType32*>(this->Offsets.Get()),
          static_cast<ArrayType32*>(this->Connectivity.Get()), std::forward<Args>(args)...);
        break;
      case StorageTypes::Int64:
        functor(static_cast<ArrayType64*>(this->Offsets.Get()),
          static_cast<ArrayType64*>(this->Connectivity.Get()), std::forward<Args>(args)...);
        break;
      case StorageTypes::FixedSizeInt32:
        functor(static_cast<AffineArrayType32*>(this->Offsets.Get()),
          static_cast<ArrayType32*>(this->Connectivity.Get()), std::forward<Args>(args)...);
        break;
      case StorageTypes::FixedSizeInt64:
        functor(static_cast<AffineArrayType64*>(this->Offsets.Get()),
          static_cast<ArrayType64*>(this->Connectivity.Get()), std::forward<Args>(args)...);
        break;
      case StorageTypes::Generic:
      default:
        functor(this->Offsets.Get(), this->Connectivity.Get(), std::forward<Args>(args)...);
        break;
    }
  }
  template <typename Functor, typename... Args>
  void Dispatch(Functor&& functor, Args&&... args) const
  {
    switch (this->StorageType)
    {
      case StorageTypes::Int32:
        functor(static_cast<ArrayType32*>(this->Offsets.Get()),
          static_cast<ArrayType32*>(this->Connectivity.Get()), std::forward<Args>(args)...);
        break;
      case StorageTypes::Int64:
        functor(static_cast<ArrayType64*>(this->Offsets.Get()),
          static_cast<ArrayType64*>(this->Connectivity.Get()), std::forward<Args>(args)...);
        break;
      case StorageTypes::FixedSizeInt32:
        functor(static_cast<AffineArrayType32*>(this->Offsets.Get()),
          static_cast<ArrayType32*>(this->Connectivity.Get()), std::forward<Args>(args)...);
        break;
      case StorageTypes::FixedSizeInt64:
        functor(static_cast<AffineArrayType64*>(this->Offsets.Get()),
          static_cast<ArrayType64*>(this->Connectivity.Get()), std::forward<Args>(args)...);
        break;
      case StorageTypes::Generic:
      default:
        functor(this->Offsets.Get(), this->Connectivity.Get(), std::forward<Args>(args)...);
        break;
    }
  }
  ///@}

  // Holds connectivity and offset arrays of the given ArrayType.
  // VTK_DEPRECATED_IN_9_6_0("Use DispatchUtilities")
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
  VTK_DEPRECATED_IN_9_6_0("Use Dispatch instead")
  void Visit(Functor&& functor, Args&&... args)
  {
    switch (this->StorageType)
    {
      case StorageTypes::Int32:
      {
        VisitState<ArrayType32> state;
        state.Offsets = ArrayType32::FastDownCast(this->Offsets);
        state.Connectivity = ArrayType32::FastDownCast(this->Connectivity);
        functor(state, std::forward<Args>(args)...);
        break;
      }
      case StorageTypes::Int64:
      {
        VisitState<ArrayType64> state;
        state.Offsets = ArrayType64::FastDownCast(this->Offsets);
        state.Connectivity = ArrayType64::FastDownCast(this->Connectivity);
        functor(state, std::forward<Args>(args)...);
        break;
      }
      case StorageTypes::FixedSizeInt32:
      case StorageTypes::FixedSizeInt64:
      case StorageTypes::Generic:
      default:
      {
        vtkWarningMacro("Use Dispatch");
        break;
      }
    }
  }

  template <typename Functor, typename... Args,
    typename = typename std::enable_if<ReturnsVoid<Functor, Args...>::value>::type>
  VTK_DEPRECATED_IN_9_6_0("Use Dispatch instead")
  void Visit(Functor&& functor, Args&&... args) const
  {
    switch (this->StorageType)
    {
      case StorageTypes::Int32:
      {
        VisitState<ArrayType32> state;
        state.Offsets = ArrayType32::FastDownCast(this->Offsets);
        state.Connectivity = ArrayType32::FastDownCast(this->Connectivity);
        functor(state, std::forward<Args>(args)...);
        break;
      }
      case StorageTypes::Int64:
      {
        VisitState<ArrayType64> state;
        state.Offsets = ArrayType64::FastDownCast(this->Offsets);
        state.Connectivity = ArrayType64::FastDownCast(this->Connectivity);
        functor(state, std::forward<Args>(args)...);
        break;
      }
      case StorageTypes::FixedSizeInt32:
      case StorageTypes::FixedSizeInt64:
      case StorageTypes::Generic:
      default:
      {
        vtkWarningMacro("Use Dispatch");
        break;
      }
    }
  }

  template <typename Functor, typename... Args,
    typename = typename std::enable_if<!ReturnsVoid<Functor, Args...>::value>::type>
  VTK_DEPRECATED_IN_9_6_0("Use Dispatch instead")
  GetReturnType<Functor, Args...> Visit(Functor&& functor, Args&&... args)
  {
    switch (this->StorageType)
    {
      case StorageTypes::Int32:
      {
        VisitState<ArrayType32> state;
        state.Offsets = ArrayType32::FastDownCast(this->Offsets);
        state.Connectivity = ArrayType32::FastDownCast(this->Connectivity);
        return functor(state, std::forward<Args>(args)...);
      }
      case StorageTypes::Int64:
      {
        VisitState<ArrayType64> state;
        state.Offsets = ArrayType64::FastDownCast(this->Offsets);
        state.Connectivity = ArrayType64::FastDownCast(this->Connectivity);
        return functor(state, std::forward<Args>(args)...);
      }
      case StorageTypes::FixedSizeInt32:
      case StorageTypes::FixedSizeInt64:
      case StorageTypes::Generic:
      default:
      {
        vtkWarningMacro("Use Dispatch");
        return GetReturnType<Functor, Args...>();
      }
    }
  }
  template <typename Functor, typename... Args,
    typename = typename std::enable_if<!ReturnsVoid<Functor, Args...>::value>::type>
  VTK_DEPRECATED_IN_9_6_0("Use Dispatch instead")
  GetReturnType<Functor, Args...> Visit(Functor&& functor, Args&&... args) const
  {
    switch (this->StorageType)
    {
      case StorageTypes::Int32:
      {
        VisitState<ArrayType32> state;
        state.Offsets = ArrayType32::FastDownCast(this->Offsets);
        state.Connectivity = ArrayType32::FastDownCast(this->Connectivity);
        return functor(state, std::forward<Args>(args)...);
      }
      case StorageTypes::Int64:
      {
        VisitState<ArrayType64> state;
        state.Offsets = ArrayType64::FastDownCast(this->Offsets);
        state.Connectivity = ArrayType64::FastDownCast(this->Connectivity);
        return functor(state, std::forward<Args>(args)...);
      }
      case StorageTypes::FixedSizeInt32:
      case StorageTypes::FixedSizeInt64:
      case StorageTypes::Generic:
      default:
      {
        vtkWarningMacro("Use Dispatch");
        return GetReturnType<Functor, Args...>();
      }
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
  VTK_DEPRECATED_IN_9_6_0("This call has no effect.")
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
  VTK_DEPRECATED_IN_9_6_0("Use AllocateEstimate directly instead.")
  vtkIdType EstimateSize(vtkIdType numCells, int maxPtsPerCell);

  /**
   * Get the size of the allocated connectivity array.
   *
   * @warning This returns the allocated capacity of the internal arrays as a
   * number of elements, NOT the number of elements in use.
   *
   * @note Method incompatible with current internal storage.
   */
  VTK_DEPRECATED_IN_9_6_0("Method incompatible with current internal storage.")
  vtkIdType GetSize();

  /**
   * Return the size of the array that would be returned from
   * ExportLegacyFormat().
   *
   * @note Method incompatible with current internal storage.
   */
  VTK_DEPRECATED_IN_9_6_0("Method incompatible with current internal storage.")
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
  VTK_DEPRECATED_IN_9_6_0("Use GetCellAtId.")
  void GetCell(vtkIdType loc, vtkIdType& npts, const vtkIdType*& pts)
    VTK_EXPECTS(0 <= loc && loc < GetNumberOfConnectivityEntries()) VTK_SIZEHINT(pts, npts);

  /**
   * Internal method used to retrieve a cell given a legacy offset location.
   *
   * @note The location-based API is now a super-slow compatibility layer.
   * Prefer GetCellAtId.
   */
  VTK_DEPRECATED_IN_9_6_0("Use GetCellAtId.")
  void GetCell(vtkIdType loc, vtkIdList* pts)
    VTK_EXPECTS(0 <= loc && loc < GetNumberOfConnectivityEntries());

  /**
   * Computes the current legacy insertion location within the internal array.
   * Used in conjunction with GetCell(int loc,...).
   *
   * @note The location-based API is now a super-slow compatibility layer.
   */
  VTK_DEPRECATED_IN_9_6_0("Use GetNumberOfCells.")
  vtkIdType GetInsertLocation(int npts);

  /**
   * Get/Set the current traversal legacy location.
   *
   * @note The location-based API is now a super-slow compatibility layer.
   * Prefer Get/SetTraversalCellId.
   * @{
   */
  VTK_DEPRECATED_IN_9_6_0("Use GetTraversalCellId.")
  vtkIdType GetTraversalLocation();
  VTK_DEPRECATED_IN_9_6_0("Use GetTraversalCellId.")
  vtkIdType GetTraversalLocation(vtkIdType npts);
  VTK_DEPRECATED_IN_9_6_0("Use SetTraversalCellId.")
  void SetTraversalLocation(vtkIdType loc);
  /**@}*/

  /**
   * Special method inverts ordering of cell at the specified legacy location.
   * Must be called carefully or the cell topology may be corrupted.
   *
   * @note The location-based API is now a super-slow compatibility layer.
   * Prefer ReverseCellAtId;
   */
  VTK_DEPRECATED_IN_9_6_0("Use ReverseCellAtId.")
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
  VTK_DEPRECATED_IN_9_6_0("Use ReplaceCellAtId.")
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
  VTK_DEPRECATED_IN_9_6_0("Use ImportLegacyFormat or SetData instead.")
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
  VTK_DEPRECATED_IN_9_6_0(
    "Use ExportLegacyFormat, or GetOffsetsArray/GetConnectivityArray instead.")
  vtkIdTypeArray* GetData();

  //=================== End Legacy Methods =====================================

  friend class vtkCellArrayIterator;

protected:
  vtkCellArray();
  ~vtkCellArray() override;

  vtkSmartPointer<vtkDataArray> Offsets;
  vtkSmartPointer<vtkDataArray> Connectivity;
  StorageTypes StorageType;
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

struct InsertNextCellImpl : public vtkCellArray::DispatchUtilities
{
  // Insert full cell
  template <class OffsetsT, class ConnectivityT>
  void operator()(OffsetsT* offsets, ConnectivityT* conn, const vtkIdType npts,
    const vtkIdType pts[], vtkIdType& cellId)
  {
    using ValueType = GetAPIType<OffsetsT>;
    using OffsetsAccessorType = vtkDataArrayAccessor<OffsetsT>;
    using ConnectivityAccessorType = vtkDataArrayAccessor<ConnectivityT>;
    OffsetsAccessorType offsetsAccesor(offsets);
    ConnectivityAccessorType connAccesor(conn);

    cellId = offsets->GetNumberOfValues() - 1;

    offsetsAccesor.InsertNext(static_cast<ValueType>(conn->GetNumberOfValues() + npts));

    for (vtkIdType i = 0; i < npts; ++i)
    {
      connAccesor.InsertNext(static_cast<ValueType>(pts[i]));
    }
  }

  // Just update offset table (for incremental API)
  template <class OffsetsT, class ConnectivityT>
  void operator()(OffsetsT* offsets, ConnectivityT* conn, const vtkIdType npts, vtkIdType& cellId)
  {
    using ValueType = GetAPIType<OffsetsT>;
    using AccessorType = vtkDataArrayAccessor<OffsetsT>;
    AccessorType offsetsAccesor(offsets);

    cellId = offsets->GetNumberOfValues() - 1;

    offsetsAccesor.InsertNext(static_cast<ValueType>(conn->GetNumberOfValues() + npts));
  }
};

// for incremental API:
struct UpdateCellCountImpl : public vtkCellArray::DispatchUtilities
{
  template <class OffsetsT, class ConnectivityT>
  void operator()(OffsetsT* offsets, ConnectivityT* vtkNotUsed(conn), const vtkIdType npts)
  {
    using ValueType = GetAPIType<OffsetsT>;

    auto offsetsRange = GetRange(offsets);
    const ValueType cellBegin = offsetsRange[offsets->GetMaxId() - 1];
    offsetsRange[offsets->GetMaxId()] = static_cast<ValueType>(cellBegin + npts);
  }
};

struct GetCellSizeImpl : public vtkCellArray::DispatchUtilities
{
  template <class OffsetsT, class ConnectivityT>
  void operator()(
    OffsetsT* offsets, ConnectivityT* vtkNotUsed(conn), vtkIdType cellId, vtkIdType& cellSize)
  {
    cellSize = GetCellSize(offsets, cellId);
  }
};

struct GetCellAtIdImpl : public vtkCellArray::DispatchUtilities
{
  template <class OffsetsT, class ConnectivityT>
  void operator()(OffsetsT* offsets, ConnectivityT* conn, const vtkIdType cellId, vtkIdList* ids)
  {
    auto offsetsRange = GetRange(offsets);
    const auto& beginOffset = offsetsRange[cellId];
    const auto& endOffset = offsetsRange[cellId + 1];
    const vtkIdType cellSize = static_cast<vtkIdType>(endOffset - beginOffset);
    const auto cellConnectivity = GetRange(conn).begin() + beginOffset;

    // ValueType differs from vtkIdType, so we have to copy into a temporary buffer:
    ids->SetNumberOfIds(cellSize);
    vtkIdType* idPtr = ids->GetPointer(0);
    for (vtkIdType i = 0; i < cellSize; ++i)
    {
      idPtr[i] = static_cast<vtkIdType>(cellConnectivity[i]);
    }
  }

  template <class OffsetsT, class ConnectivityT>
  void operator()(OffsetsT* offsets, ConnectivityT* conn, const vtkIdType cellId,
    vtkIdType& cellSize, vtkIdType* cellPoints)
  {
    auto offsetsRange = GetRange(offsets);
    const auto& beginOffset = offsetsRange[cellId];
    const auto& endOffset = offsetsRange[cellId + 1];
    cellSize = static_cast<vtkIdType>(endOffset - beginOffset);
    const auto cellConnectivity = GetRange(conn).begin() + beginOffset;

    // ValueType differs from vtkIdType, so we have to copy into a temporary buffer:
    for (vtkIdType i = 0; i < cellSize; ++i)
    {
      cellPoints[i] = static_cast<vtkIdType>(cellConnectivity[i]);
    }
  }

  // SFINAE helper to check if a Functors's connectivity array's memory can be used as a vtkIdType*.
  template <typename ConnectivityT>
  struct CanShareConnPtr
  {
    static constexpr bool value =
      std::is_base_of_v<vtkAOSDataArrayTemplate<vtkIdType>, ConnectivityT>;
  };

  template <class OffsetsT, class ConnectivityT>
  typename std::enable_if_t<CanShareConnPtr<ConnectivityT>::value, void> operator()(
    OffsetsT* offsets, ConnectivityT* conn, const vtkIdType cellId, vtkIdType& cellSize,
    vtkIdType const*& cellPoints, vtkIdList* vtkNotUsed(temp))
  {
    auto offsetsRange = GetRange(offsets);
    const auto& beginOffset = offsetsRange[cellId];
    const auto& endOffset = offsetsRange[cellId + 1];
    cellSize = static_cast<vtkIdType>(endOffset - beginOffset);
    // This is safe, see CanShareConnPtr helper above.
    cellPoints = conn->GetPointer(beginOffset);
  }

  template <class OffsetsT, class ConnectivityT>
  typename std::enable_if_t<!CanShareConnPtr<ConnectivityT>::value, void> operator()(
    OffsetsT* offsets, ConnectivityT* conn, const vtkIdType cellId, vtkIdType& cellSize,
    vtkIdType const*& cellPoints, vtkIdList* temp)
  {
    auto offsetsRange = GetRange(offsets);
    const auto& beginOffset = offsetsRange[cellId];
    const auto& endOffset = offsetsRange[cellId + 1];
    cellSize = static_cast<vtkIdType>(endOffset - beginOffset);
    const auto cellConnectivity = GetRange(conn).begin() + beginOffset;

    temp->SetNumberOfIds(cellSize);
    vtkIdType* tempPtr = temp->GetPointer(0);
    for (vtkIdType i = 0; i < cellSize; ++i)
    {
      tempPtr[i] = static_cast<vtkIdType>(cellConnectivity[i]);
    }

    cellPoints = tempPtr;
  }
};

struct CellPointAtIdImpl : public vtkCellArray::DispatchUtilities
{
  template <class OffsetsT, class ConnectivityT>
  void operator()(OffsetsT* offsets, ConnectivityT* conn, vtkIdType cellId,
    vtkIdType cellPointIndex, vtkIdType& pointId)
  {
    pointId =
      static_cast<vtkIdType>(GetRange(conn)[GetBeginOffset(offsets, cellId) + cellPointIndex]);
  }
};

struct ResetImpl : public vtkCellArray::DispatchUtilities
{
  template <class OffsetsT, class ConnectivityT>
  void operator()(OffsetsT* offsets, ConnectivityT* conn)
  {
    using ValueType = GetAPIType<OffsetsT>;
    using AccessorType = vtkDataArrayAccessor<OffsetsT>;
    offsets->Reset();
    conn->Reset();
    AccessorType accessor(offsets);
    ValueType firstOffset = 0;
    accessor.InsertNext(firstOffset);
  }
};

struct InsertCellPointImpl : public vtkCellArray::DispatchUtilities
{
  template <class OffsetsT, class ConnectivityT>
  void operator()(OffsetsT* vtkNotUsed(offsets), ConnectivityT* conn, vtkIdType id)
  {
    using ValueType = GetAPIType<ConnectivityT>;
    using AccessorType = vtkDataArrayAccessor<ConnectivityT>;
    AccessorType accessor(conn);
    accessor.InsertNext(static_cast<ValueType>(id));
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
  vtkIdType cellSize;
  this->Dispatch(vtkCellArray_detail::GetCellSizeImpl{}, cellId, cellSize);
  return cellSize;
}

//----------------------------------------------------------------------------
inline void vtkCellArray::GetCellAtId(vtkIdType cellId, vtkIdType& cellSize,
  vtkIdType const*& cellPoints, vtkIdList* ptIds) VTK_SIZEHINT(cellPoints, cellSize)
{
  this->Dispatch(vtkCellArray_detail::GetCellAtIdImpl{}, cellId, cellSize, cellPoints, ptIds);
}

//----------------------------------------------------------------------------
inline void vtkCellArray::GetCellAtId(vtkIdType cellId, vtkIdList* pts)
{
  this->Dispatch(vtkCellArray_detail::GetCellAtIdImpl{}, cellId, pts);
}

//----------------------------------------------------------------------------
inline void vtkCellArray::GetCellAtId(vtkIdType cellId, vtkIdType& cellSize, vtkIdType* cellPoints)
{
  this->Dispatch(vtkCellArray_detail::GetCellAtIdImpl{}, cellId, cellSize, cellPoints);
}

//----------------------------------------------------------------------------
inline vtkIdType vtkCellArray::GetCellPointAtId(vtkIdType cellId, vtkIdType cellPointIndex) const
{
  vtkIdType pointId;
  this->Dispatch(vtkCellArray_detail::CellPointAtIdImpl{}, cellId, cellPointIndex, pointId);
  return pointId;
}

//----------------------------------------------------------------------------
inline vtkIdType vtkCellArray::InsertNextCell(vtkIdType npts, const vtkIdType* pts)
  VTK_SIZEHINT(pts, npts)
{
  vtkIdType cellId;
  this->Dispatch(vtkCellArray_detail::InsertNextCellImpl{}, npts, pts, cellId);
  return cellId;
}

//----------------------------------------------------------------------------
inline vtkIdType vtkCellArray::InsertNextCell(int npts)
{
  vtkIdType cellId;
  this->Dispatch(vtkCellArray_detail::InsertNextCellImpl{}, npts, cellId);
  return cellId;
}

//----------------------------------------------------------------------------
inline void vtkCellArray::InsertCellPoint(vtkIdType id)
{
  this->Dispatch(vtkCellArray_detail::InsertCellPointImpl{}, id);
}

//----------------------------------------------------------------------------
inline void vtkCellArray::UpdateCellCount(int npts)
{
  this->Dispatch(vtkCellArray_detail::UpdateCellCountImpl{}, npts);
}

//----------------------------------------------------------------------------
inline vtkIdType vtkCellArray::InsertNextCell(vtkIdList* pts)
{
  vtkIdType cellId;
  this->Dispatch(
    vtkCellArray_detail::InsertNextCellImpl{}, pts->GetNumberOfIds(), pts->GetPointer(0), cellId);
  return cellId;
}

//----------------------------------------------------------------------------
inline vtkIdType vtkCellArray::InsertNextCell(vtkCell* cell)
{
  vtkIdList* pts = cell->GetPointIds();
  vtkIdType cellId;
  this->Dispatch(
    vtkCellArray_detail::InsertNextCellImpl{}, pts->GetNumberOfIds(), pts->GetPointer(0), cellId);
  return cellId;
}

//----------------------------------------------------------------------------
inline void vtkCellArray::Reset()
{
  this->Dispatch(vtkCellArray_detail::ResetImpl{});
}

VTK_ABI_NAMESPACE_END
#endif // vtkCellArray.h
