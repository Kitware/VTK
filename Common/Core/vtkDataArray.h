// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDataArray
 * @brief   abstract superclass for arrays of numeric data
 *
 *
 * vtkDataArray is an abstract superclass for data array objects
 * containing numeric data.  It extends the API defined in
 * vtkAbstractArray.  vtkDataArray is an abstract superclass for data
 * array objects. This class defines an API that all array objects
 * must support. Note that the concrete subclasses of this class
 * represent data in native form (char, int, etc.) and often have
 * specialized more efficient methods for operating on this data (for
 * example, getting pointers to data or getting/inserting data in
 * native form).  Subclasses of vtkDataArray are assumed to contain
 * data whose components are meaningful when cast to and from double.
 *
 * @sa
 * vtkBitArray vtkGenericDataArray
 */

#ifndef vtkDataArray_h
#define vtkDataArray_h

#include "vtkAbstractArray.h"
#include "vtkCommonCoreModule.h"          // For export macro
#include "vtkVTK_USE_SCALED_SOA_ARRAYS.h" // For #define of VTK_USE_SCALED_SOA_ARRAYS
#include "vtkWrappingHints.h"             // For VTK_MARSHALMANUAL

VTK_ABI_NAMESPACE_BEGIN
class vtkDoubleArray;
class vtkIdList;
class vtkInformationStringKey;
class vtkInformationDoubleVectorKey;
class vtkLookupTable;
class vtkPoints;

class VTKCOMMONCORE_EXPORT VTK_MARSHALMANUAL vtkDataArray : public vtkAbstractArray
{
public:
  vtkTypeMacro(vtkDataArray, vtkAbstractArray);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Perform a fast, safe cast from a vtkAbstractArray to a vtkDataArray.
   * This method checks if source->GetArrayType() returns DataArray
   * or a more derived type, and performs a static_cast to return
   * source as a vtkDataArray pointer. Otherwise, nullptr is returned.
   */
  static vtkDataArray* FastDownCast(vtkAbstractArray* source);

  /**
   * This method is here to make backward compatibility easier.  It
   * must return true if and only if an array contains numeric data.
   * All vtkDataArray subclasses contain numeric data, hence this method
   * always returns 1(true).
   */
  int IsNumeric() const override { return 1; }

  /**
   * Return the size, in bytes, of the lowest-level element of an
   * array.  For vtkDataArray and subclasses this is the size of the
   * data type.
   */
  int GetElementComponentSize() const override { return this->GetDataTypeSize(); }

  // Reimplemented virtuals (doc strings are inherited from superclass):
  ///@{
  /**
   * See documentation from parent class.
   * This method assumes that the `source` inherits from `vtkDataArray`, but its value type doesn't
   * have to match the type of the current instance.
   */
  void InsertTuple(vtkIdType dstTupleIdx, vtkIdType srcTupleIdx, vtkAbstractArray* source) override;
  vtkIdType InsertNextTuple(vtkIdType srcTupleIdx, vtkAbstractArray* source) override;
  void InsertTuples(vtkIdList* dstIds, vtkIdList* srcIds, vtkAbstractArray* source) override;
  void InsertTuples(
    vtkIdType dstStart, vtkIdType n, vtkIdType srcStart, vtkAbstractArray* source) override;
  void InsertTuplesStartingAt(
    vtkIdType dstStart, vtkIdList* srcIds, vtkAbstractArray* source) override;
  void SetTuple(vtkIdType dstTupleIdx, vtkIdType srcTupleIdx, vtkAbstractArray* source) override;
  ///@}
  void GetTuples(vtkIdList* tupleIds, vtkAbstractArray* output) override;
  void GetTuples(vtkIdType p1, vtkIdType p2, vtkAbstractArray* output) override;
  void InterpolateTuple(vtkIdType dstTupleIdx, vtkIdList* ptIndices, vtkAbstractArray* source,
    double* weights) override;
  void InterpolateTuple(vtkIdType dstTupleIdx, vtkIdType srcTupleIdx1, vtkAbstractArray* source1,
    vtkIdType srcTupleIdx2, vtkAbstractArray* source2, double t) override;

  /**
   * Get the data tuple at tupleIdx. Return it as a pointer to an array.
   * Note: this method is not thread-safe, and the pointer is only valid
   * as long as another method invocation to a vtk object is not performed.
   */
  virtual double* GetTuple(vtkIdType tupleIdx)
    VTK_EXPECTS(0 <= tupleIdx && tupleIdx < GetNumberOfTuples()) = 0;

  /**
   * Get the data tuple at tupleIdx by filling in a user-provided array,
   * Make sure that your array is large enough to hold the NumberOfComponents
   * amount of data being returned.
   */
  virtual void GetTuple(vtkIdType tupleIdx, double* tuple)
    VTK_EXPECTS(0 <= tupleIdx && tupleIdx < GetNumberOfTuples()) = 0;

  ///@{
  /**
   * Get/set the data at \a tupleIdx by filling in a user-provided array
   * of integers.
   *
   * This variant accepts signed 64-bit integers for the tuple.
   * Subclasses of vtkDataArray whose IsIntegral() method returns true
   * should override this method to provide exact integer values; the
   * default implementation uses double-precision to integer conversion.
   */
  virtual void GetIntegerTuple(vtkIdType tupleIdx, vtkTypeInt64* tuple)
    VTK_EXPECTS(0 <= tupleIdx && tupleIdx < GetNumberOfTuples());
  virtual void SetIntegerTuple(vtkIdType tupleIdx, vtkTypeInt64* tuple)
    VTK_EXPECTS(0 <= tupleIdx && tupleIdx < GetNumberOfTuples());
  ///@}

  ///@{
  /**
   * Get/set the data at \a tupleIdx by filling in a user-provided array
   * of unsigned integers.
   *
   * This variant accepts unsigned 64-bit integers for the tuple.
   * Subclasses of vtkDataArray whose IsIntegral() method returns true
   * should override this method to provide exact integer values; the
   * default implementation uses double-precision to integer conversion.
   */
  virtual void GetUnsignedTuple(vtkIdType tupleIdx, vtkTypeUInt64* tuple)
    VTK_EXPECTS(0 <= tupleIdx && tupleIdx < GetNumberOfTuples());
  virtual void SetUnsignedTuple(vtkIdType tupleIdx, vtkTypeUInt64* tuple)
    VTK_EXPECTS(0 <= tupleIdx && tupleIdx < GetNumberOfTuples());
  ///@}

  ///@{
  /**
   * These methods are included as convenience for the wrappers.
   * GetTuple() and SetTuple() which return/take arrays can not be
   * used from wrapped languages. These methods can be used instead.
   */
  double GetTuple1(vtkIdType tupleIdx) VTK_EXPECTS(0 <= tupleIdx && tupleIdx < GetNumberOfTuples());
  double* GetTuple2(vtkIdType tupleIdx) VTK_EXPECTS(0 <= tupleIdx && tupleIdx < GetNumberOfTuples())
    VTK_SIZEHINT(2);
  double* GetTuple3(vtkIdType tupleIdx) VTK_EXPECTS(0 <= tupleIdx && tupleIdx < GetNumberOfTuples())
    VTK_SIZEHINT(3);
  double* GetTuple4(vtkIdType tupleIdx) VTK_EXPECTS(0 <= tupleIdx && tupleIdx < GetNumberOfTuples())
    VTK_SIZEHINT(4);
  double* GetTuple6(vtkIdType tupleIdx) VTK_EXPECTS(0 <= tupleIdx && tupleIdx < GetNumberOfTuples())
    VTK_SIZEHINT(6);
  double* GetTuple9(vtkIdType tupleIdx) VTK_EXPECTS(0 <= tupleIdx && tupleIdx < GetNumberOfTuples())
    VTK_SIZEHINT(9);
  ///@}

  ///@{
  /**
   * Set the data tuple at tupleIdx. Note that range checking or
   * memory allocation is not performed; use this method in conjunction
   * with SetNumberOfTuples() to allocate space.
   */
  virtual void SetTuple(vtkIdType tupleIdx, const float* tuple)
    VTK_EXPECTS(0 <= tupleIdx && tupleIdx < GetNumberOfTuples());
  virtual void SetTuple(vtkIdType tupleIdx, const double* tuple)
    VTK_EXPECTS(0 <= tupleIdx && tupleIdx < GetNumberOfTuples());
  ///@}

  ///@{
  /**
   * These methods are included as convenience for the wrappers.
   * GetTuple() and SetTuple() which return/take arrays can not be
   * used from wrapped languages. These methods can be used instead.
   */
  void SetTuple1(vtkIdType tupleIdx, double value)
    VTK_EXPECTS(0 <= tupleIdx && tupleIdx < GetNumberOfTuples());
  void SetTuple2(vtkIdType tupleIdx, double val0, double val1)
    VTK_EXPECTS(0 <= tupleIdx && tupleIdx < GetNumberOfTuples());
  void SetTuple3(vtkIdType tupleIdx, double val0, double val1, double val2)
    VTK_EXPECTS(0 <= tupleIdx && tupleIdx < GetNumberOfTuples());
  void SetTuple4(vtkIdType tupleIdx, double val0, double val1, double val2, double val3)
    VTK_EXPECTS(0 <= tupleIdx && tupleIdx < GetNumberOfTuples());
  void SetTuple6(vtkIdType tupleIdx, double val0, double val1, double val2, double val3,
    double val4, double val5) VTK_EXPECTS(0 <= tupleIdx && tupleIdx < GetNumberOfTuples());
  void SetTuple9(vtkIdType tupleIdx, double val0, double val1, double val2, double val3,
    double val4, double val5, double val6, double val7, double val8)
    VTK_EXPECTS(0 <= tupleIdx && tupleIdx < GetNumberOfTuples());
  ///@}

  ///@{
  /**
   * Insert the data tuple at tupleIdx. Note that memory allocation
   * is performed as necessary to hold the data.
   */
  virtual void InsertTuple(vtkIdType tupleIdx, const float* tuple) VTK_EXPECTS(0 <= tupleIdx) = 0;
  virtual void InsertTuple(vtkIdType tupleIdx, const double* tuple) VTK_EXPECTS(0 <= tupleIdx) = 0;
  ///@}

  ///@{
  /**
   * These methods are included as convenience for the wrappers.
   * InsertTuple() which takes arrays can not be
   * used from wrapped languages. These methods can be used instead.
   */
  void InsertTuple1(vtkIdType tupleIdx, double value) VTK_EXPECTS(0 <= tupleIdx);
  void InsertTuple2(vtkIdType tupleIdx, double val0, double val1) VTK_EXPECTS(0 <= tupleIdx);
  void InsertTuple3(vtkIdType tupleIdx, double val0, double val1, double val2)
    VTK_EXPECTS(0 <= tupleIdx);
  void InsertTuple4(vtkIdType tupleIdx, double val0, double val1, double val2, double val3)
    VTK_EXPECTS(0 <= tupleIdx);
  void InsertTuple6(vtkIdType tupleIdx, double val0, double val1, double val2, double val3,
    double val4, double val5) VTK_EXPECTS(0 <= tupleIdx);
  void InsertTuple9(vtkIdType tupleIdx, double val0, double val1, double val2, double val3,
    double val4, double val5, double val6, double val7, double val8) VTK_EXPECTS(0 <= tupleIdx);
  ///@}

  ///@{
  /**
   * Insert the data tuple at the end of the array and return the tuple index at
   * which the data was inserted. Memory is allocated as necessary to hold
   * the data.
   */
  virtual vtkIdType InsertNextTuple(const float* tuple) = 0;
  virtual vtkIdType InsertNextTuple(const double* tuple) = 0;
  ///@}

  ///@{
  /**
   * These methods are included as convenience for the wrappers.
   * InsertTuple() which takes arrays can not be
   * used from wrapped languages. These methods can be used instead.
   */
  void InsertNextTuple1(double value);
  void InsertNextTuple2(double val0, double val1);
  void InsertNextTuple3(double val0, double val1, double val2);
  void InsertNextTuple4(double val0, double val1, double val2, double val3);
  void InsertNextTuple6(
    double val0, double val1, double val2, double val3, double val4, double val5);
  void InsertNextTuple9(double val0, double val1, double val2, double val3, double val4,
    double val5, double val6, double val7, double val8);
  ///@}

  ///@{
  /**
   * These methods remove tuples from the data array. They shift data and
   * resize array, so the data array is still valid after this operation. Note,
   * this operation is fairly slow.
   */
  virtual void RemoveTuple(vtkIdType tupleIdx)
    VTK_EXPECTS(0 <= tupleIdx && tupleIdx < GetNumberOfTuples()) = 0;
  virtual void RemoveFirstTuple() { this->RemoveTuple(0); }
  virtual void RemoveLastTuple();
  ///@}

  /**
   * Return the data component at the location specified by tupleIdx and
   * compIdx.
   */
  virtual double GetComponent(vtkIdType tupleIdx, int compIdx)
    VTK_EXPECTS(0 <= tupleIdx && GetNumberOfComponents() * tupleIdx + compIdx < GetNumberOfValues())
      VTK_EXPECTS(0 <= compIdx && compIdx < GetNumberOfComponents());

  /**
   * Set the data component at the location specified by tupleIdx and compIdx
   * to value.
   * Note that i is less than NumberOfTuples and j is less than
   * NumberOfComponents. Make sure enough memory has been allocated
   * (use SetNumberOfTuples() and SetNumberOfComponents()).
   */
  virtual void SetComponent(vtkIdType tupleIdx, int compIdx, double value)
    VTK_EXPECTS(0 <= tupleIdx && GetNumberOfComponents() * tupleIdx + compIdx < GetNumberOfValues())
      VTK_EXPECTS(0 <= compIdx && compIdx < GetNumberOfComponents());

  /**
   * Insert value at the location specified by tupleIdx and compIdx.
   * Note that memory allocation is performed as necessary to hold the data.
   */
  virtual void InsertComponent(vtkIdType tupleIdx, int compIdx, double value)
    VTK_EXPECTS(0 <= tupleIdx) VTK_EXPECTS(0 <= compIdx && compIdx < GetNumberOfComponents());

  /**
   * Get the data as a double array in the range (tupleMin,tupleMax) and
   * (compMin, compMax). The resulting double array consists of all data in
   * the tuple range specified and only the component range specified. This
   * process typically requires casting the data from native form into
   * doubleing point values. This method is provided as a convenience for data
   * exchange, and is not very fast.
   */
  virtual void GetData(
    vtkIdType tupleMin, vtkIdType tupleMax, int compMin, int compMax, vtkDoubleArray* data);

  ///@{
  /**
   * Deep copy of data. Copies data from different data arrays even if
   * they are different types (using doubleing-point exchange).
   */
  void DeepCopy(vtkAbstractArray* aa) override;
  virtual void DeepCopy(vtkDataArray* da);
  ///@}

  /**
   * Create a shallow copy of other into this, if possible. Shallow copies are
   * only possible:
   * (a) if both arrays are the same data type
   * (b) if both arrays are the same array type (e.g. AOS vs. SOA)
   * (c) if both arrays support shallow copies (e.g. vtkBitArray currently
   * does not.)
   * If a shallow copy is not possible, a deep copy will be performed instead.
   */
  virtual void ShallowCopy(vtkDataArray* other);

  /**
   * Fill a component of a data array with a specified value. This method
   * sets the specified component to specified value for all tuples in the
   * data array.  This methods can be used to initialize or reinitialize a
   * single component of a multi-component array.
   */
  virtual void FillComponent(int compIdx, double value)
    VTK_EXPECTS(0 <= compIdx && compIdx < GetNumberOfComponents());

  /**
   * Fill all values of a data array with a specified value.
   */
  virtual void Fill(double value);

  /**
   * Copy a component from one data array into a component on this data array.
   * This method copies the specified component ("srcComponent") from the
   * specified data array ("src") to the specified component ("dstComponent")
   * over all the tuples in this data array.  This method can be used to extract
   * a component (column) from one data array and paste that data into
   * a component on this data array.
   */
  virtual void CopyComponent(int dstComponent, vtkDataArray* src, int srcComponent);

  /**
   * Get the address of a particular data index. Make sure data is allocated
   * for the number of items requested. If needed, increase MaxId to mark any
   * new value ranges as in-use.
   */
  virtual void* WriteVoidPointer(vtkIdType valueIdx, vtkIdType numValues) = 0;

  /**
   * Return the memory in kibibytes (1024 bytes) consumed by this data array. Used to
   * support streaming and reading/writing data. The value returned is
   * guaranteed to be greater than or equal to the memory required to
   * actually represent the data represented by this object. The
   * information returned is valid only after the pipeline has
   * been updated.
   */
  unsigned long GetActualMemorySize() const override;

  /**
   * Create default lookup table. Generally used to create one when none
   * is available.
   */
  void CreateDefaultLookupTable();

  ///@{
  /**
   * Set/get the lookup table associated with this scalar data, if any.
   */
  void SetLookupTable(vtkLookupTable* lut);
  vtkGetObjectMacro(LookupTable, vtkLookupTable);
  ///@}

  ///@{
  /**
   * The range of the data array values for the given component will be
   * returned in the provided range array argument. If comp is -1, the range
   * of the magnitude (L2 norm) over all components will be provided. The
   * range is computed and then cached, and will not be re-computed on
   * subsequent calls to GetRange() unless the array is modified or the
   * requested component changes.
   *
   * The version of this method with `ghosts` and `ghostsToSkip` allows to skip
   * values in the computation of the range. At a given id, if `ghosts[id] & ghostsToSkip != 0`,
   * then the corresponding tuple is not accounted for when computing the range.
   * Note that when the ghost array is provided, no cached value is stored inside
   * this instance. See `vtkFieldData::GetRange`, which caches the computated range
   * when using a ghost array.
   *
   * THIS METHOD IS NOT THREAD SAFE.
   */
  void GetRange(double range[2], int comp) { this->ComputeRange(range, comp); }
  void GetRange(double range[2], int comp, const unsigned char* ghosts, unsigned char ghostsToSkip)
  {
    this->ComputeRange(range, comp, ghosts, ghostsToSkip);
  }
  ///@}

  ///@{
  /**
   * Return the range of the data array values for the given component. If
   * comp is -1, return the range of the magnitude (L2 norm) over all
   * components.The range is computed and then cached, and will not be
   * re-computed on subsequent calls to GetRange() unless the array is
   * modified or the requested component changes.
   * THIS METHOD IS NOT THREAD SAFE.
   */
  double* GetRange(int comp) VTK_SIZEHINT(2)
  {
    this->GetRange(this->Range, comp);
    return this->Range;
  }
  ///@}

  /**
   * Return the range of the data array. If the array has multiple components,
   * then this will return the range of only the first component (component
   * zero). The range is computed and then cached, and will not be re-computed
   * on subsequent calls to GetRange() unless the array is modified.
   * THIS METHOD IS NOT THREAD SAFE.
   */
  double* GetRange() VTK_SIZEHINT(2) { return this->GetRange(0); }

  /**
   * The range of the data array values will be returned in the provided
   * range array argument. If the data array has multiple components, then
   * this will return the range of only the first component (component zero).
   * The range is computend and then cached, and will not be re-computed on
   * subsequent calls to GetRange() unless the array is modified.
   * THIS METHOD IS NOT THREAD SAFE.
   */
  void GetRange(double range[2]) { this->GetRange(range, 0); }

  ///@{
  /**
   * The range of the data array values for the given component will be
   * returned in the provided range array argument. If comp is -1, the range
   * of the magnitude (L2 norm) over all components will be provided. The
   * range is computed and then cached, and will not be re-computed on
   * subsequent calls to GetRange() unless the array is modified or the
   * requested component changes.
   *
   * The version of this method with `ghosts` and `ghostsToSkip` allows to skip
   * values in the computation of the range. At a given id, if `ghosts[id] & ghostsToSkip != 0`,
   * then the corresponding tuple is not accounted for when computing the range.
   *
   * Note that when the ghost array is provided, no cached value is stored inside
   * this instance. See `vtkFieldData::GetRange`, which caches the computated range
   * when using a ghost array.
   *
   * THIS METHOD IS NOT THREAD SAFE.
   */
  void GetFiniteRange(double range[2], int comp) { this->ComputeFiniteRange(range, comp); }
  void GetFiniteRange(
    double range[2], int comp, const unsigned char* ghosts, unsigned char ghostsToSkip)
  {
    this->ComputeFiniteRange(range, comp, ghosts, ghostsToSkip);
  }
  ///@}

  ///@{
  /**
   * Return the range of the data array values for the given component. If
   * comp is -1, return the range of the magnitude (L2 norm) over all
   * components.The range is computed and then cached, and will not be
   * re-computed on subsequent calls to GetRange() unless the array is
   * modified or the requested component changes.
   * THIS METHOD IS NOT THREAD SAFE.
   */
  double* GetFiniteRange(int comp) VTK_SIZEHINT(2)
  {
    this->GetFiniteRange(this->FiniteRange, comp);
    return this->FiniteRange;
  }
  ///@}

  /**
   * Return the range of the data array. If the array has multiple components,
   * then this will return the range of only the first component (component
   * zero). The range is computed and then cached, and will not be re-computed
   * on subsequent calls to GetRange() unless the array is modified.
   * THIS METHOD IS NOT THREAD SAFE.
   */
  double* GetFiniteRange() VTK_SIZEHINT(2) { return this->GetFiniteRange(0); }

  /**
   * The range of the data array values will be returned in the provided
   * range array argument. If the data array has multiple components, then
   * this will return the range of only the first component (component zero).
   * The range is computend and then cached, and will not be re-computed on
   * subsequent calls to GetRange() unless the array is modified.
   * THIS METHOD IS NOT THREAD SAFE.
   */
  void GetFiniteRange(double range[2]) { this->GetFiniteRange(range, 0); }

  ///@{
  /**
   * These methods return the Min and Max possible range of the native
   * data type. For example if a vtkScalars consists of unsigned char
   * data these will return (0,255).
   */
  void GetDataTypeRange(double range[2]);
  double GetDataTypeMin();
  double GetDataTypeMax();
  static void GetDataTypeRange(int type, double range[2]);
  static double GetDataTypeMin(int type);
  static double GetDataTypeMax(int type);
  ///@}

  /**
   * Return the maximum norm for the tuples.
   * Note that the max. is computed every time GetMaxNorm is called.
   */
  virtual double GetMaxNorm();

  /**
   * Creates an array for dataType where dataType is one of
   * VTK_BIT, VTK_CHAR, VTK_SIGNED_CHAR, VTK_UNSIGNED_CHAR, VTK_SHORT,
   * VTK_UNSIGNED_SHORT, VTK_INT, VTK_UNSIGNED_INT, VTK_LONG,
   * VTK_UNSIGNED_LONG, VTK_FLOAT, VTK_DOUBLE, VTK_ID_TYPE.
   * Note that the data array returned has be deleted by the
   * user.
   */
  VTK_NEWINSTANCE
  static vtkDataArray* CreateDataArray(int dataType);

  /**
   * This key is used to hold tight bounds on the range of
   * one component over all tuples of the array.
   * Two values (a minimum and maximum) are stored for each component.
   * When GetRange() is called when no tuples are present in the array
   * this value is set to { VTK_DOUBLE_MAX, VTK_DOUBLE_MIN }.
   */
  static vtkInformationDoubleVectorKey* COMPONENT_RANGE();

  /**
   * This key is used to hold tight bounds on the $L_2$ norm
   * of tuples in the array.
   * Two values (a minimum and maximum) are stored for each component.
   * When GetRange() is called when no tuples are present in the array
   * this value is set to { VTK_DOUBLE_MAX, VTK_DOUBLE_MIN }.
   */
  static vtkInformationDoubleVectorKey* L2_NORM_RANGE();

  /**
   * This key is used to hold tight bounds on the $L_2$ norm
   * of tuples in the array.
   * Two values (a minimum and maximum) are stored for each component.
   * When GetFiniteRange() is called when no tuples are present in the array
   * this value is set to { VTK_DOUBLE_MAX, VTK_DOUBLE_MIN }.
   */
  static vtkInformationDoubleVectorKey* L2_NORM_FINITE_RANGE();

  /**
   * Removes out-of-date L2_NORM_RANGE() and L2_NORM_FINITE_RANGE() values.
   */
  void Modified() override;

  /**
   * A human-readable string indicating the units for the array data.
   */
  static vtkInformationStringKey* UNITS_LABEL();

  /**
   * Copy information instance. Arrays use information objects
   * in a variety of ways. It is important to have flexibility in
   * this regard because certain keys should not be copied, while
   * others must be. NOTE: Up to the implmenter to make sure that
   * keys not intended to be copied are excluded here.
   */
  int CopyInformation(vtkInformation* infoFrom, vtkTypeBool deep = 1) override;

  /**
   * Method for type-checking in FastDownCast implementations.
   */
  int GetArrayType() const override { return DataArray; }

protected:
  friend class vtkPoints;
  friend class vtkFieldData;

  ///@{
  /**
   * Compute the range for a specific component. If comp is set -1
   * then L2 norm is computed on all components. Call ClearRange
   * to force a recomputation if it is needed. The range is copied
   * to the range argument.
   *
   * The version of this method with `ghosts` and `ghostsToSkip` allows to skip
   * values in the computation of the range. At a given id, if `ghosts[id] & ghostsToSkip != 0`,
   * then the corresponding tuple is not accounted for when computing the range.
   *
   * Note that when the ghost array is provided, no cached value is stored inside
   * this instance. See `vtkFieldData::GetRange`, which caches the computated range
   * when using a ghost array.
   *
   * THIS METHOD IS NOT THREAD SAFE.
   */
  virtual void ComputeRange(double range[2], int comp);
  virtual void ComputeRange(
    double range[2], int comp, const unsigned char* ghosts, unsigned char ghostsToSkip = 0xff);
  ///@}

  ///@{
  /**
   * Compute the range for a specific component. If comp is set -1
   * then L2 norm is computed on all components. Call ClearRange
   * to force a recomputation if it is needed. The range is copied
   * to the range argument.
   *
   * The version of this method with `ghosts` and `ghostsToSkip` allows to skip
   * values in the computation of the range. At a given id, if `ghosts[id] & ghostsToSkip != 0`,
   * then the corresponding tuple is not accounted for when computing the range.
   *
   * Note that when the ghost array is provided, no cached value is stored inside
   * this instance. See `vtkFieldData::GetFiniteRange`, which caches the computated range
   * when using a ghost array.
   *
   * THIS METHOD IS NOT THREAD SAFE.
   */
  virtual void ComputeFiniteRange(double range[2], int comp);
  virtual void ComputeFiniteRange(
    double range[2], int comp, const unsigned char* ghosts, unsigned char ghostsToSkip = 0xff);
  ///@}

  ///@{
  /**
   * Computes the range for each component of an array, the length
   * of \a ranges must be two times the number of components.
   * Returns true if the range was computed. Will return false
   * if you try to compute the range of an array of length zero.
   *
   * The version of this method with `ghosts` and `ghostsToSkip` allows to skip
   * values in the computation of the range. At a given id, if `ghosts[id] & ghostsToSkip != 0`,
   * then the corresponding tuple is not accounted for when computing the range.
   *
   * Note that when the ghost array is provided, no cached value is stored inside
   * this instance. See `vtkFieldData::GetRange`, which caches the computated range
   * when using a ghost array.
   *
   */
  virtual bool ComputeScalarRange(double* ranges);
  virtual bool ComputeScalarRange(
    double* ranges, const unsigned char* ghosts, unsigned char ghostsToSkip = 0xff);
  ///@}

  ///@{
  /**
   * Returns true if the range was computed. Will return false
   * if you try to compute the range of an array of length zero.
   *
   * The version of this method with `ghosts` and `ghostsToSkip` allows to skip
   * values in the computation of the range. At a given id, if `ghosts[id] & ghostsToSkip != 0`,
   * then the corresponding tuple is not accounted for when computing the range.
   *
   * Note that when the ghost array is provided, no cached value is stored inside
   * this instance. See `vtkFieldData::GetRange`, which caches the computated range
   * when using a ghost array.
   */
  virtual bool ComputeVectorRange(double range[2]);
  virtual bool ComputeVectorRange(
    double range[2], const unsigned char* ghosts, unsigned char ghostsToSkip = 0xff);
  ///@}

  ///@{
  /**
   * Computes the range for each component of an array, the length
   * of \a ranges must be two times the number of components.
   * Returns true if the range was computed. Will return false
   * if you try to compute the range of an array of length zero.
   *
   * The version of this method with `ghosts` and `ghostsToSkip` allows to skip
   * values in the computation of the range. At a given id, if `ghosts[id] & ghostsToSkip != 0`,
   * then the corresponding tuple is not accounted for when computing the range.
   *
   * Note that when the ghost array is provided, no cached value is stored inside
   * this instance. See `vtkFieldData::GetFiniteRange`, which caches the computated range
   * when using a ghost array.
   */
  virtual bool ComputeFiniteScalarRange(double* ranges);
  virtual bool ComputeFiniteScalarRange(
    double* ranges, const unsigned char* ghosts, unsigned char ghostsToSkip = 0xff);
  ///@}

  ///@{
  /**
   * Returns true if the range was computed. Will return false
   * if you try to compute the range of an array of length zero.
   *
   * The version of this method with `ghosts` and `ghostsToSkip` allows to skip
   * values in the computation of the range. At a given id, if `ghosts[id] & ghostsToSkip != 0`,
   * then the corresponding tuple is not accounted for when computing the range.
   *
   * Note that when the ghost array is provided, no cached value is stored inside
   * this instance. See `vtkFieldData::GetFiniteRange`, which caches the computated range
   * when using a ghost array.
   */
  virtual bool ComputeFiniteVectorRange(double range[2]);
  virtual bool ComputeFiniteVectorRange(
    double range[2], const unsigned char* ghosts, unsigned char ghostsToSkip = 0xff);
  ///@}

  // Construct object with default tuple dimension (number of components) of 1.
  vtkDataArray();
  ~vtkDataArray() override;

  vtkLookupTable* LookupTable;
  double Range[2];
  double FiniteRange[2];

private:
  double* GetTupleN(vtkIdType i, int n);

  vtkDataArray(const vtkDataArray&) = delete;
  void operator=(const vtkDataArray&) = delete;
};

//------------------------------------------------------------------------------
inline vtkDataArray* vtkDataArray::FastDownCast(vtkAbstractArray* source)
{
  if (source)
  {
    switch (source->GetArrayType())
    {
      case AoSDataArrayTemplate:
      case SoADataArrayTemplate:
      case ImplicitArray:
      case TypedDataArray:
      case DataArray:
      case MappedDataArray:
        return static_cast<vtkDataArray*>(source);
      default:
        break;
    }
  }
  return nullptr;
}

vtkArrayDownCast_FastCastMacro(vtkDataArray);
VTK_ABI_NAMESPACE_END

// These are used by vtkDataArrayPrivate.txx, but need to be available to
// vtkGenericDataArray.h as well.
namespace vtkDataArrayPrivate
{
VTK_ABI_NAMESPACE_BEGIN
struct AllValues
{
};
struct FiniteValues
{
};
VTK_ABI_NAMESPACE_END
}

#endif
