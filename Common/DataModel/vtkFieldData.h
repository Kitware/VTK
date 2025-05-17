// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkFieldData
 * @brief   represent and manipulate fields of data
 *
 * vtkFieldData represents and manipulates fields of data. The model of a field
 * is a m x n matrix of data values, where m is the number of tuples, and n
 * is the number of components. (A tuple is a row of n components in the
 * matrix.) The field is assumed to be composed of a set of one or more data
 * arrays, where the data in the arrays are of different types (e.g., int,
 * double, char, etc.), and there may be variable numbers of components in
 * each array. Note that each data array is assumed to be "m" in length
 * (i.e., number of tuples), which typically corresponds to the number of
 * points or cells in a dataset. Also, each data array must have a
 * character-string name. (This is used to manipulate data.)
 *
 * There are two ways of manipulating and interfacing to fields. You can do
 * it generically by manipulating components/tuples via a double-type data
 * exchange, or you can do it by grabbing the arrays and manipulating them
 * directly. The former is simpler but performs type conversion, which is bad
 * if your data has non-castable types like (void) pointers, or you lose
 * information as a result of the cast. The more efficient method means
 * managing each array in the field.  Using this method you can create
 * faster, more efficient algorithms that do not lose information.
 *
 * @sa
 * vtkAbstractArray vtkDataSetAttributes vtkPointData vtkCellData
 */

#ifndef vtkFieldData_h
#define vtkFieldData_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALMANUAL

#include "vtkAbstractArray.h" // Needed for inline methods.

#include <array>  // For CachedGhostRangeType
#include <tuple>  // For CachedGhostRangeType
#include <vector> // For list indices

VTK_ABI_NAMESPACE_BEGIN
class vtkIdList;
class vtkDoubleArray;
class vtkUnsignedCharArray;

class VTKCOMMONDATAMODEL_EXPORT VTK_MARSHALMANUAL vtkFieldData : public vtkObject
{
public:
  static vtkFieldData* New();
  static vtkFieldData* ExtendedNew();

  vtkTypeMacro(vtkFieldData, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Release all data but do not delete object.
   * Also, clear the copy flags.
   */
  virtual void Initialize();

  /**
   * Allocate data for each array.
   * Note that ext is no longer used.
   */
  vtkTypeBool Allocate(vtkIdType sz, vtkIdType ext = 1000);

  /**
   * Copy data array structure from a given field.  The same arrays
   * will exist with the same types, but will contain nothing in the
   * copy.
   */
  void CopyStructure(vtkFieldData*);

  /**
   * AllocateArrays actually sets the number of
   * vtkAbstractArray pointers in the vtkFieldData object, not the
   * number of used pointers (arrays). Adding more arrays will
   * cause the object to dynamically adjust the number of pointers
   * if it needs to extend. Although AllocateArrays can
   * be used if the number of arrays which will be added is
   * known, it can be omitted with a small computation cost.
   */
  void AllocateArrays(int num);

  /**
   * Get the number of arrays of data available.
   * This does not include nullptr array pointers therefore after
   * fd->AllocateArray(n); nArrays = GetNumberOfArrays();
   * nArrays is not necessarily equal to n.
   */
  int GetNumberOfArrays() { return this->NumberOfActiveArrays; }

  /**
   * Add an array to the array list. If an array with the same name
   * already exists - then the added array will replace it.
   * Return the index of the added array. If the given array is nullptr,
   * does nothing and returns -1.
   */
  int AddArray(vtkAbstractArray* array);

  /**
   * Sets every vtkDataArray at index id to a null tuple.
   */
  void NullData(vtkIdType id);

  ///@{
  /**
   * Remove an array (with the given name) from the list of arrays.
   */
  virtual void RemoveArray(const char* name);

  /**
   * Remove an array (with the given index) from the list of arrays.
   */
  virtual void RemoveArray(int index);
  ///@}

  /**
   * Not recommended for use. Use GetAbstractArray(int i) instead.
   *
   * Return the ith array in the field. A nullptr is returned if the
   * index i is out of range, or if the array at the given
   * index is not a vtkDataArray. To access vtkStringArray,
   * or vtkVariantArray, use GetAbstractArray(int i).
   */
  vtkDataArray* GetArray(int i);

  /**
   * Not recommended for use. Use
   * GetAbstractArray(const char *arrayName, int &index) instead.
   *
   * Return the array with the name given. Returns nullptr if array not found.
   * A nullptr is also returned if the array with the given name is not a
   * vtkDataArray. To access vtkStringArray, or
   * vtkVariantArray, use GetAbstractArray(const char* arrayName, int &index).
   * Also returns the index of the array if found, -1 otherwise.
   */
  vtkDataArray* GetArray(const char* arrayName, int& index);

  ///@{
  /**
   * Not recommended for use. Use GetAbstractArray(const char *arrayName)
   * instead.
   *
   * Return the array with the name given. Returns nullptr if array not found.
   * A nullptr is also returned if the array with the given name is not a
   * vtkDataArray. To access vtkStringArray, or
   * vtkVariantArray, use GetAbstractArray(const char *arrayName).
   */
  vtkDataArray* GetArray(const char* arrayName)
  {
    int i;
    return this->GetArray(arrayName, i);
  }
  ///@}

  /**
   * Returns the ith array in the field. Unlike GetArray(), this method returns
   * a vtkAbstractArray and can be used to access any array type. A nullptr is
   * returned only if the index i is out of range.
   */
  vtkAbstractArray* GetAbstractArray(int i);

  /**
   * Return the array with the name given. Returns nullptr if array not found.
   * Unlike GetArray(), this method returns a vtkAbstractArray and can be used
   * to access any array type. Also returns index of array if found, -1
   * otherwise.
   */
  vtkAbstractArray* GetAbstractArray(const char* arrayName, int& index);

  ///@{
  /**
   * Return the array with the name given. Returns nullptr if array not found.
   * Unlike GetArray(), this method returns a vtkAbstractArray and can be used
   * to access any array type.
   */
  vtkAbstractArray* GetAbstractArray(const char* arrayName)
  {
    int i;
    return this->GetAbstractArray(arrayName, i);
  }
  ///@}

  ///@{
  /**
   * Return 1 if an array with the given name could be found. 0 otherwise.
   */
  vtkTypeBool HasArray(const char* name)
  {
    int i;
    vtkAbstractArray* array = this->GetAbstractArray(name, i);
    return array ? 1 : 0;
  }
  ///@}

  ///@{
  /**
   * Get the name of ith array.
   * Note that this is equivalent to:
   * GetAbstractArray(i)->GetName() if ith array pointer is not nullptr
   */
  const char* GetArrayName(int i)
  {
    vtkAbstractArray* da = this->GetAbstractArray(i);
    return da ? da->GetName() : nullptr;
  }
  ///@}

  /**
   * Pass entire arrays of input data through to output. Obey the "copy"
   * flags.
   */
  virtual void PassData(vtkFieldData* fd);

  /**
   * Turn on/off the copying of the field specified by name.
   * During the copying/passing, the following rules are followed for each
   * array:
   * 1. If the copy flag for an array is set (on or off), it is applied.
   * This overrides rule 2.
   * 2. If CopyAllOn is set, copy the array.
   * If CopyAllOff is set, do not copy the array.
   */
  void CopyFieldOn(const char* name) { this->CopyFieldOnOff(name, 1); }
  void CopyFieldOff(const char* name) { this->CopyFieldOnOff(name, 0); }

  /**
   * Turn on copying of all data.
   * During the copying/passing, the following rules are followed for each
   * array:
   * 1. If the copy flag for an array is set (on or off), it is applied.
   * This overrides rule 2.
   * 2. If CopyAllOn is set, copy the array.
   * If CopyAllOff is set, do not copy the array.
   */
  virtual void CopyAllOn(int unused = 0);

  /**
   * Turn off copying of all data.
   * During the copying/passing, the following rules are followed for each
   * array:
   * 1. If the copy flag for an array is set (on or off), it is applied.
   * This overrides rule 2.
   * 2. If CopyAllOn is set, copy the array.
   * If CopyAllOff is set, do not copy the array.
   */
  virtual void CopyAllOff(int unused = 0);

  /**
   * Copy a field by creating new data arrays (i.e., duplicate storage).
   */
  virtual void DeepCopy(vtkFieldData* da);

  /**
   * Copy a field by reference counting the data arrays.
   */
  virtual void ShallowCopy(vtkFieldData* da);

  /**
   * Squeezes each data array in the field (Squeeze() reclaims unused memory.)
   */
  void Squeeze();

  /**
   * Resets each data array in the field (Reset() does not release memory but
   * it makes the arrays look like they are empty.)
   */
  void Reset();

  /**
   * Return the memory in kibibytes (1024 bytes) consumed by this field data. Used to
   * support streaming and reading/writing data. The value returned is
   * guaranteed to be greater than or equal to the memory required to
   * actually represent the data represented by this object.
   */
  virtual unsigned long GetActualMemorySize();

  /**
   * Check object's components for modified times.
   */
  vtkMTimeType GetMTime() override;

  /**
   * Get a field from a list of ids. Supplied field f should have same
   * types and number of data arrays as this one (i.e., like
   * CopyStructure() creates).  This method should not be used if the
   * instance is from a subclass of vtkFieldData (vtkPointData or
   * vtkCellData).  This is because in those cases, the attribute data
   * is stored with the other fields and will cause the method to
   * behave in an unexpected way.
   */
  void GetField(vtkIdList* ptId, vtkFieldData* f);

  /**
   * Return the array containing the ith component of the field. The
   * return value is an integer number n 0<=n<this->NumberOfArrays. Also,
   * an integer value is returned indicating the component in the array
   * is returned. Method returns -1 if specified component is not
   * in the field.
   */
  int GetArrayContainingComponent(int i, int& arrayComp);

  /**
   * Get the number of components in the field. This is determined by adding
   * up the components in each non-nullptr array.
   * This method should not be used if the instance is from a
   * subclass of vtkFieldData (vtkPointData or vtkCellData).
   * This is because in those cases, the attribute data is
   * stored with the other fields and will cause the method
   * to behave in an unexpected way.
   */
  int GetNumberOfComponents();

  /**
   * Get the number of tuples in the field. Note: some fields have arrays with
   * different numbers of tuples; this method returns the number of tuples in
   * the first array. Mixed-length arrays may have to be treated specially.
   * This method should not be used if the instance is from a
   * subclass of vtkFieldData (vtkPointData or vtkCellData).
   * This is because in those cases, the attribute data is
   * stored with the other fields and will cause the method
   * to behave in an unexpected way.
   */
  vtkIdType GetNumberOfTuples();

  /**
   * Set the number of tuples for each data array in the field.
   * This method should not be used if the instance is from a
   * subclass of vtkFieldData (vtkPointData or vtkCellData).
   * This is because in those cases, the attribute data is
   * stored with the other fields and will cause the method
   * to behave in an unexpected way.
   */
  void SetNumberOfTuples(vtkIdType number);

  /**
   * Set the jth tuple in source field data at the ith location.
   * Set operations mean that no range checking is performed, so
   * they're faster.
   */
  void SetTuple(vtkIdType i, vtkIdType j, vtkFieldData* source);

  /**
   * Insert the jth tuple in source field data at the ith location.
   * Range checking is performed and memory allocates as necessary.
   */
  void InsertTuple(vtkIdType i, vtkIdType j, vtkFieldData* source);

  /**
   * Insert the jth tuple in source field data at the end of the
   * tuple matrix. Range checking is performed and memory is allocated
   * as necessary.
   */
  vtkIdType InsertNextTuple(vtkIdType j, vtkFieldData* source);

  ///@{
  /**
   * Computes the range of the input data array (specified through its `name` or the `index`
   * in this field data). If the targeted array is not polymorphic
   * with a `vtkDataArray`, or if no array match the input `name` or `index`, or
   * if `comp` is out of bounds, then the returned range is `[NaN, NaN]`.
   *
   * The computed range is cached to avoid recomputing it. The range is recomputed
   * if the held array has been modified, if `GhostsToSkip` has been changed, or if
   * the ghost array has been changed / modified.
   *
   * If a ghost array is present in the field data, then the binary mask `GhostsToSkip`
   * is used to skip values associated with a ghost that intersects this mask.
   *
   * `comp` targets which component of the array the range is to be computed on.
   * Setting it to -1 results in computing the range of the magnitude of the array.
   *
   * The `Finite` version of this method skips infinite values in the array in addition
   * to ghosts matching with `GhostsToSkip`.
   */
  bool GetRange(const char* name, double range[2], int comp = 0);
  bool GetRange(int index, double range[2], int comp = 0);
  bool GetFiniteRange(const char* name, double range[2], int comp = 0);
  bool GetFiniteRange(int index, double range[2], int comp = 0);
  ///@}

  ///@{
  /**
   * Set / Get the binary mask filtering out certain types of ghosts when calling `GetRange`.
   * By default, it is set to 0xff for pure `vtkFieldData`. In `vtkCellData`, it is set to
   * `HIDDENCELL` and in `vtkPointData`, it is set to `HIDDENPOINT` by default.
   * See `vtkDataSetAttributes` for more context on ghost types definitions.
   *
   * @sa
   * vtkDataSetAttributes
   * vtkPointData
   * vtkCellData
   */
  vtkGetMacro(GhostsToSkip, unsigned char);
  virtual void SetGhostsToSkip(unsigned char);
  ///@}

  /**
   * Helper function that tests if any of the values in ghost array has been set.
   * The test performed is (value & bitFlag).
   */
  bool HasAnyGhostBitSet(int bitFlag);

  /**
   * Get the ghost array, if present in this field data. If no ghost array is set,
   * returns `nullptr`. A ghost array is a `vtkUnsignedCharArray` called `vtkGhostType`.
   * See `vtkDataSetAttributes` for more context on ghost types.
   *
   * @sa
   * vtkDataSetAttributes
   */
  vtkGetObjectMacro(GhostArray, vtkUnsignedCharArray);

protected:
  vtkFieldData();
  ~vtkFieldData() override;

  int NumberOfArrays;
  int NumberOfActiveArrays;
  vtkAbstractArray** Data;

  /**
   * Set an array to define the field.
   */
  void SetArray(int i, vtkAbstractArray* array);

  /**
   * Release all data but do not delete object.
   */
  virtual void InitializeFields();

  struct CopyFieldFlag
  {
    char* ArrayName;
    int IsCopied;
  };

  CopyFieldFlag* CopyFieldFlags; // the names of fields not to be copied
  int NumberOfFieldFlags;        // the number of fields not to be copied
  void CopyFieldOnOff(const char* name, int onOff);
  void ClearFieldFlags();
  int FindFlag(const char* field);
  int GetFlag(const char* field);
  void CopyFlags(const vtkFieldData* source);
  int DoCopyAllOn;
  int DoCopyAllOff;

  /*
   * This tuple holds: [array time stamp, ghost array time stamp, cached ranges].
   * Those time stamps are used to decide whether the cached range should be recomputed or not.
   * when requesting the range of an array.
   *
   * When there is no ghost array, the ghost array time stamp is defined as equal to 0.
   */
  using CachedGhostRangeType = std::tuple<vtkMTimeType, vtkMTimeType, std::vector<double>>;
  unsigned char GhostsToSkip;
  vtkUnsignedCharArray* GhostArray;

  ///@{
  /**
   * `Ranges` and `FiniteRanges` store cached ranges for arrays stored in this field data.
   * Given the array at index `idx`, 2 ranges are stored: the magnitude range at `Ranges[idx][0]`,
   * and all the component ranges at `Ranges[idx][1]`. The ranges are stored in the third
   * component of the tuple `CachedGhostRangeType`. For the component ranges, they are stored
   * in an array of size 2 times the number of components, storing `[min0, max0, ..., minn, maxn]`.
   */
  std::vector<std::array<CachedGhostRangeType, 2>> Ranges;
  std::vector<std::array<CachedGhostRangeType, 2>> FiniteRanges;
  ///@}

private:
  vtkFieldData(const vtkFieldData&) = delete;
  void operator=(const vtkFieldData&) = delete;

  friend class vtkFieldDataSerDesHelper; // for access to SetArray()

public:
  class VTKCOMMONDATAMODEL_EXPORT BasicIterator
  {
  public:
    BasicIterator() = default;
    BasicIterator(const BasicIterator& source);
    BasicIterator(const int* list, unsigned int listSize);
    BasicIterator& operator=(const BasicIterator& source);
    virtual ~BasicIterator() = default;
    void PrintSelf(ostream& os, vtkIndent indent);

    int GetListSize() const { return static_cast<int>(this->List.size()); }
    int GetCurrentIndex() { return this->List[this->Position]; }
    int BeginIndex()
    {
      this->Position = -1;
      return this->NextIndex();
    }
    int End() const { return (this->Position >= static_cast<int>(this->List.size())); }
    int NextIndex()
    {
      this->Position++;
      return (this->End() ? -1 : this->List[this->Position]);
    }

    // Support C++ range-for loops; e.g, code like
    // "for (const auto& i : basicIterator)".
    std::vector<int>::const_iterator begin() { return this->List.begin(); }
    std::vector<int>::const_iterator end() { return this->List.end(); }

  protected:
    std::vector<int> List;
    int Position;
  };

  class VTKCOMMONDATAMODEL_EXPORT Iterator : public BasicIterator
  {
  public:
    Iterator(const Iterator& source);
    Iterator& operator=(const Iterator& source);
    ~Iterator() override;
    Iterator(vtkFieldData* dsa, const int* list = nullptr, unsigned int listSize = 0);

    vtkDataArray* Begin()
    {
      this->Position = -1;
      return this->Next();
    }

    vtkDataArray* Next()
    {
      this->Position++;
      if (this->End())
      {
        return nullptr;
      }

      // vtkFieldData::GetArray() can return null, which implies that
      // a the array at the given index in not a vtkDataArray subclass.
      // This iterator skips such arrays.
      vtkDataArray* cur = Fields->GetArray(this->List[this->Position]);
      return (cur ? cur : this->Next());
    }

    void DetachFieldData();

  protected:
    vtkFieldData* Fields;
    int Detached;
  };
};

VTK_ABI_NAMESPACE_END
#endif
