/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
/**
 * @class   vtkAbstractArray
 * @brief   Abstract superclass for all arrays
 *
 *
 *
 * vtkAbstractArray is an abstract superclass for data array objects.
 * This class defines an API that all subclasses must support.  The
 * data type must be assignable and copy-constructible, but no other
 * assumptions about its type are made.  Most of the subclasses of
 * this array deal with numeric data either as scalars or tuples of
 * scalars.  A program can use the IsNumeric() method to check whether
 * an instance of vtkAbstractArray contains numbers.  It is also
 * possible to test for this by attempting to SafeDownCast an array to
 * an instance of vtkDataArray, although this assumes that all numeric
 * arrays will always be descended from vtkDataArray.
 *
 * <p>
 *
 * Every array has a character-string name. The naming of the array
 * occurs automatically when it is instantiated, but you are free to
 * change this name using the SetName() method.  (The array name is
 * used for data manipulation.)
 *
 * This class (and subclasses) use two forms of addressing elements:
 * - Value Indexing: The index of an element assuming an array-of-structs
 *   memory layout.
 * - Tuple/Component Indexing: Explicitly specify the tuple and component
 *   indices.
 *
 * It is also worth pointing out that the behavior of the "Insert*" methods
 * of classes in this hierarchy may not behave as expected. They work exactly
 * as the corresponding "Set*" methods, except that memory allocation will
 * be performed if acting on a value past the end of the array. If the data
 * already exists, "inserting" will overwrite existing values, rather than shift
 * the array contents and insert the new data at the specified location.
 *
 * @sa
 * vtkDataArray vtkStringArray vtkCellArray
*/

#ifndef vtkAbstractArray_h
#define vtkAbstractArray_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"
#include "vtkVariant.h" // for variant arguments

class vtkArrayIterator;
class vtkDataArray;
class vtkIdList;
class vtkIdTypeArray;
class vtkInformation;
class vtkInformationDoubleVectorKey;
class vtkInformationIntegerKey;
class vtkInformationInformationVectorKey;
class vtkInformationVariantVectorKey;
class vtkVariantArray;

class VTKCOMMONCORE_EXPORT vtkAbstractArray : public vtkObject
{
public:
  vtkTypeMacro(vtkAbstractArray,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Allocate memory for this array. Delete old storage only if necessary.
   * Note that ext is no longer used.
   * This method will reset MaxId to -1 and resize the array capacity such that
   * this->Size >= numValues.
   * If numValues is 0, all memory will be freed.
   * Return 1 on success, 0 on failure.
   */
  virtual int Allocate(vtkIdType numValues, vtkIdType ext=1000) = 0;

  /**
   * Release storage and reset array to initial state.
   */
  virtual void Initialize() = 0;

  /**
   * Return the underlying data type. An integer indicating data type is
   * returned as specified in vtkSetGet.h.
   */
  virtual int GetDataType() =0;

  //@{
  /**
   * Return the size of the underlying data type.  For a bit, 0 is
   * returned.  For string 0 is returned. Arrays with variable length
   * components return 0.
   */
  virtual int GetDataTypeSize() = 0;
  static int GetDataTypeSize(int type);
  //@}

  /**
   * Return the size, in bytes, of the lowest-level element of an
   * array.  For vtkDataArray and subclasses this is the size of the
   * data type.  For vtkStringArray, this is
   * sizeof(vtkStdString::value_type), which winds up being
   * sizeof(char).
   */
  virtual int GetElementComponentSize() = 0;

  //@{
  /**
   * Set/Get the dimension (n) of the components. Must be >= 1. Make sure that
   * this is set before allocation.
   */
  vtkSetClampMacro(NumberOfComponents, int, 1, VTK_INT_MAX);
  int GetNumberOfComponents() { return this->NumberOfComponents; }
  //@}

  /**
   * Set the name for a component. Must be >= 1.
   */
  void SetComponentName( vtkIdType component, const char *name );

  /**
   * Get the component name for a given component.
   * Note: will return the actual string that is stored
   */
  const char* GetComponentName( vtkIdType component );

  /**
   * Returns if any component has had a name assigned
   */
  bool HasAComponentName();

  /**
   * Copies the component names from the inputed array to the current array
   * make sure that the current array has the same number of components as the input array
   */
  int CopyComponentNames( vtkAbstractArray *da );

  /**
   * Set the number of tuples (a component group) in the array. Note that
   * this may allocate space depending on the number of components.
   * Also note that if allocation is performed no copy is performed so
   * existing data will be lost (if data conservation is sought, one may
   * use the Resize method instead).
   */
  virtual void SetNumberOfTuples(vtkIdType numTuples) = 0;

  /**
   * Specify the number of values (tuples * components) for this object to hold.
   * Does an allocation as well as setting the MaxId ivar. Used in conjunction
   * with SetValue() method for fast insertion.
   */
  virtual void SetNumberOfValues(vtkIdType numValues);

  /**
   * Get the number of complete tuples (a component group) in the array.
   */
  vtkIdType GetNumberOfTuples()
    {return (this->MaxId + 1)/this->NumberOfComponents;}

  /**
   * Get the total number of values in the array. This is typically equivalent
   * to (numTuples * numComponents). The exception is during incremental array
   * construction for subclasses that support component insertion, which may
   * result in an incomplete trailing tuple.
   */
  inline vtkIdType GetNumberOfValues() const
  {
    return (this->MaxId + 1);
  }

  /**
   * Set the tuple at dstTupleIdx in this array to the tuple at srcTupleIdx in
   * the source array. This method assumes that the two arrays have the same
   * type and structure. Note that range checking and memory allocation is not
   * performed; use in conjunction with SetNumberOfTuples() to allocate space.
   */
  virtual void SetTuple(vtkIdType dstTupleIdx, vtkIdType srcTupleIdx,
                        vtkAbstractArray *source) = 0;

  /**
   * Insert the tuple at srcTupleIdx in the source array into this array at
   * dstTupleIdx.
   * Note that memory allocation is performed as necessary to hold the data.
   */
  virtual void InsertTuple(vtkIdType dstTupleIdx, vtkIdType srcTupleIdx,
                           vtkAbstractArray* source) = 0;

  /**
   * Copy the tuples indexed in srcIds from the source array to the tuple
   * locations indexed by dstIds in this array.
   * Note that memory allocation is performed as necessary to hold the data.
   */
  virtual void InsertTuples(vtkIdList *dstIds, vtkIdList *srcIds,
                            vtkAbstractArray* source) = 0;

  /**
   * Copy n consecutive tuples starting at srcStart from the source array to
   * this array, starting at the dstStart location.
   * Note that memory allocation is performed as necessary to hold the data.
   */
  virtual void InsertTuples(vtkIdType dstStart, vtkIdType n, vtkIdType srcStart,
                            vtkAbstractArray* source) = 0;

  /**
   * Insert the tuple from srcTupleIdx in the source array at the end of this
   * array. Note that memory allocation is performed as necessary to hold the
   * data. Returns the tuple index at which the data was inserted.
   */
  virtual vtkIdType InsertNextTuple(vtkIdType srcTupleIdx,
                                    vtkAbstractArray* source) = 0;

  /**
   * Given a list of tuple ids, return an array of tuples.
   * You must insure that the output array has been previously
   * allocated with enough space to hold the data.
   */
  virtual void GetTuples(vtkIdList *tupleIds, vtkAbstractArray* output);

  /**
   * Get the tuples for the range of tuple ids specified
   * (i.e., p1->p2 inclusive). You must insure that the output array has
   * been previously allocated with enough space to hold the data.
   */
  virtual void GetTuples(vtkIdType p1, vtkIdType p2, vtkAbstractArray *output);

  /**
   * Returns true if this array uses the standard memory layout defined in the
   * VTK user guide, e.g. a contiguous array:
   * {t1c1, t1c2, t1c3, ... t1cM, t2c1, ... tNcM}
   * where t1c2 is the second component of the first tuple.
   */
  virtual bool HasStandardMemoryLayout();

  /**
   * Return a void pointer. For image pipeline interface and other
   * special pointer manipulation.
   * Use of this method is discouraged, as newer arrays require a deep-copy of
   * the array data in order to return a suitable pointer. See vtkArrayDispatch
   * for a safer alternative for fast data access.
   */
  virtual void *GetVoidPointer(vtkIdType valueIdx) = 0;

  /**
   * Deep copy of data. Implementation left to subclasses, which
   * should support as many type conversions as possible given the
   * data type.

   * Subclasses should call vtkAbstractArray::DeepCopy() so that the
   * information object (if one exists) is copied from \a da.
   */
  virtual void DeepCopy(vtkAbstractArray* da);

  /**
   * Set the tuple at dstTupleIdx in this array to the interpolated tuple value,
   * given the ptIndices in the source array and associated interpolation
   * weights.
   * This method assumes that the two arrays are of the same type
   * and strcuture.
   */
  virtual void InterpolateTuple(vtkIdType dstTupleIdx, vtkIdList *ptIndices,
                                vtkAbstractArray* source,  double* weights) = 0;

  /**
   * Insert the tuple at dstTupleIdx in this array to the tuple interpolated
   * from the two tuple indices, srcTupleIdx1 and srcTupleIdx2, and an
   * interpolation factor, t. The interpolation factor ranges from (0,1),
   * with t=0 located at the tuple described by srcTupleIdx1. This method
   * assumes that the three arrays are of the same type, srcTupleIdx1 is an
   * index to array source1, and srcTupleIdx2 is an index to array source2.
   */
  virtual void InterpolateTuple(vtkIdType dstTupleIdx,
    vtkIdType srcTupleIdx1, vtkAbstractArray* source1,
    vtkIdType srcTupleIdx2, vtkAbstractArray* source2, double t) =0;

  /**
   * Free any unnecessary memory.
   * Description:
   * Resize object to just fit data requirement. Reclaims extra memory.
   */
  virtual void Squeeze() = 0;

  /**
   * Resize the array to the requested number of tuples and preserve data.
   * Increasing the array size may allocate extra memory beyond what was
   * requested. MaxId will not be modified when increasing array size.
   * Decreasing the array size will trim memory to the requested size and
   * may update MaxId if the valid id range is truncated.
   * Requesting an array size of 0 will free all memory.
   * Returns 1 if resizing succeeded and 0 otherwise.
   */
  virtual int Resize(vtkIdType numTuples) = 0;

  //@{
  /**
   * Reset to an empty state, without freeing any memory.
   */
  void Reset()
  {
      this->MaxId = -1;
      this->DataChanged();
  }
  //@}

  /**
   * Return the size of the data.
   */
  vtkIdType GetSize()
  {return this->Size;}

  /**
   * What is the maximum id currently in the array.
   */
  vtkIdType GetMaxId()
    {return this->MaxId;}

  enum DeleteMethod
  {
    VTK_DATA_ARRAY_FREE,
    VTK_DATA_ARRAY_DELETE,
    VTK_DATA_ARRAY_ALIGNED_FREE
  };

  //@{
  /**
   * This method lets the user specify data to be held by the array.  The
   * array argument is a pointer to the data.  size is the size of the array
   * supplied by the user.  Set save to 1 to keep the class from deleting the
   * array when it cleans up or reallocates memory.  The class uses the
   * actual array provided; it does not copy the data from the supplied
   * array. If specified, the delete method determines how the data array
   * will be deallocated. If the delete method is VTK_DATA_ARRAY_FREE, free()
   * will be used. If the delete method is VTK_DATA_ARRAY_DELETE, delete[]
   * will be used. If the delete method is VTK_DATA_ARRAY_ALIGNED_FREE
   * _aligned_free() will be used on windows, while free() will be used
   * everywhere else.The default is FREE.
   * (Note not all subclasses can support deleteMethod.)
   */
  virtual void SetVoidArray(void *vtkNotUsed(array),
                            vtkIdType vtkNotUsed(size),
                            int vtkNotUsed(save)) =0;
  virtual void SetVoidArray(void *array, vtkIdType size, int save,
                            int vtkNotUsed(deleteMethod))
    {this->SetVoidArray(array,size,save);};
  //@}

  /**
   * This method copies the array data to the void pointer specified
   * by the user.  It is up to the user to allocate enough memory for
   * the void pointer.
   */
  virtual void ExportToVoidPointer(void *out_ptr);

  /**
   * Return the memory in kibibytes (1024 bytes) consumed by this data array. Used to
   * support streaming and reading/writing data. The value returned is
   * guaranteed to be greater than or equal to the memory required to
   * actually represent the data represented by this object. The
   * information returned is valid only after the pipeline has
   * been updated.
   */
  virtual unsigned long GetActualMemorySize() = 0;

  //@{
  /**
   * Set/get array's name
   */
  vtkSetStringMacro(Name);
  vtkGetStringMacro(Name);
  //@}

  /**
   * Get the name of a data type as a string.
   */
  virtual const char *GetDataTypeAsString( void )
    { return vtkImageScalarTypeNameMacro( this->GetDataType() ); }

  /**
   * Creates an array for dataType where dataType is one of
   * VTK_BIT, VTK_CHAR, VTK_UNSIGNED_CHAR, VTK_SHORT,
   * VTK_UNSIGNED_SHORT, VTK_INT, VTK_UNSIGNED_INT, VTK_LONG,
   * VTK_UNSIGNED_LONG, VTK_DOUBLE, VTK_DOUBLE, VTK_ID_TYPE,
   * VTK_STRING.
   * Note that the data array returned has to be deleted by the
   * user.
   */
  VTK_NEWINSTANCE
  static vtkAbstractArray* CreateArray(int dataType);

  /**
   * This method is here to make backward compatibility easier.  It
   * must return true if and only if an array contains numeric data.
   */
  virtual int IsNumeric() = 0;

  /**
   * Subclasses must override this method and provide the right kind
   * of templated vtkArrayIteratorTemplate.
   */
  VTK_NEWINSTANCE
  virtual vtkArrayIterator* NewIterator() = 0;

  /**
   * Returns the size of the data in DataTypeSize units. Thus, the
   * number of bytes for the data can be computed by GetDataSize() *
   * GetDataTypeSize(). Non-contiguous or variable- size arrays need
   * to override this method.
   */
  virtual vtkIdType GetDataSize()
  {
    return this->GetNumberOfComponents() * this->GetNumberOfTuples();
  }

  //@{
  /**
   * Return the value indices where a specific value appears.
   */
  virtual vtkIdType LookupValue(vtkVariant value) = 0;
  virtual void LookupValue(vtkVariant value, vtkIdList* valueIds) = 0;
  //@}

  /**
   * Retrieve value from the array as a variant.
   */
  virtual vtkVariant GetVariantValue(vtkIdType valueIdx);

  /**
   * Insert a value into the array from a variant.  This method does
   * bounds checking.
   */
  virtual void InsertVariantValue(vtkIdType valueIdx, vtkVariant value) = 0;

  /**
   * Set a value in the array from a variant.  This method does NOT do
   * bounds checking.
   */
  virtual void SetVariantValue(vtkIdType valueIdx, vtkVariant value) = 0;

  /**
   * Tell the array explicitly that the data has changed.
   * This is only necessary to call when you modify the array contents
   * without using the array's API (i.e. you retrieve a pointer to the
   * data and modify the array contents).  You need to call this so that
   * the fast lookup will know to rebuild itself.  Otherwise, the lookup
   * functions will give incorrect results.
   */
  virtual void DataChanged() = 0;

  /**
   * Delete the associated fast lookup data structure on this array,
   * if it exists.  The lookup will be rebuilt on the next call to a lookup
   * function.
   */
  virtual void ClearLookup() = 0;

  /**
   * Populate the given vtkVariantArray with a set of distinct values taken on
   * by the requested component (or, when passed -1, by the tuples as a whole).
   * If the set of prominent values has more than 32 entries, then the array
   * is assumed to be continuous in nature and no values are returned.

   * This method takes 2 parameters: \a uncertainty and \a minimumProminence.
   * Note that this set of returned values may not be complete if
   * \a uncertainty and \a minimumProminence are both larger than 0.0;
   * in order to perform interactively, a subsample of the array is
   * used to determine the set of values.

   * The first parameter (\a uncertainty, U) is the maximum acceptable
   * probability that a prominent value will not be detected.
   * Setting this to 0 will cause every value in the array to be examined.

   * The second parameter (\a minimumProminence, P) specifies the smallest
   * relative frequency (in [0,1]) with which a value in the array may
   * occur and still be considered prominent. Setting this to 0
   * will force every value in the array to be traversed.
   * Using numbers close to 0 for this parameter quickly causes
   * the number of samples required to obtain the given uncertainty to
   * subsume the entire array, as rare occurrences require frequent
   * sampling to detect.

   * For an array with T tuples and given uncertainty U and mininumum
   * prominence P, we sample N values, with N = f(T; P, U).
   * We want f to be sublinear in T in order to interactively handle large
   * arrays; in practice, we can make f independent of T:
   * \f$ N >= \frac{5}{P}\mathrm{ln}\left(\frac{1}{PU}\right) \f$,
   * but note that small values of P are costly to achieve.
   * The default parameters will locate prominent values that occur at least
   * 1 out of every 1000 samples with a confidence of 0.999999 (= 1 - 1e6).
   * Thanks to Seshadri Comandur (Sandia National Laboratories) for the
   * bounds on the number of samples.

   * The first time this is called, the array is examined and unique values
   * are stored in the vtkInformation object associated with the array.
   * The list of unique values will be updated on subsequent calls only if
   * the array's MTime is newer than the associated vtkInformation object or
   * if better sampling (lower \a uncertainty or \a minimumProminence) is
   * requested.
   * The DISCRETE_VALUE_SAMPLE_PARAMETERS() information key is used to
   * store the numbers which produced any current set of prominent values.

   * Also, note that every value encountered is reported and counts toward
   * the maximum of 32 distinct values, regardless of the value's frequency.
   * This is required for an efficient implementation.
   * Use the vtkOrderStatistics filter if you wish to threshold the set of
   * distinct values to eliminate "unprominent" (infrequently-occurring)
   * values.
   */
  virtual void GetProminentComponentValues(int comp, vtkVariantArray* values,
    double uncertainty = 1.e-6, double minimumProminence = 1.e-3);

  // TODO: Implement these lookup functions also.
  //virtual void LookupRange(vtkVariant min, vtkVariant max, vtkIdList* ids,
  //  bool includeMin = true, bool includeMax = true) = 0;
  //virtual void LookupGreaterThan(vtkVariant min, vtkIdList* ids, bool includeMin = false) = 0;
  //virtual void LookupLessThan(vtkVariant max, vtkIdList* ids, bool includeMax = false) = 0;

  /**
   * Get an information object that can be used to annotate the array.
   * This will always return an instance of vtkInformation, if one is
   * not currently associated with the array it will be created.
   */
  vtkInformation* GetInformation();
  /**
   * Inquire if this array has an instance of vtkInformation
   * already associated with it.
   */
  bool HasInformation(){ return this->Information!=0; }

  /**
   * Copy information instance. Arrays use information objects
   * in a variety of ways. It is important to have flexibility in
   * this regard because certain keys should not be coppied, while
   * others must be.

   * NOTE: Subclasses must always call their superclass's CopyInformation
   * method, so that all classes in the hierarchy get a chance to remove
   * keys they do not wish to be coppied. The subclass will not need to
   * explicilty copy the keys as it's handled here.
   */
  virtual int CopyInformation(vtkInformation *infoFrom, int deep=1);

  /**
   * This key is a hint to end user interface that this array
   * is internal and should not be shown to the end user.
   */
  static vtkInformationIntegerKey* GUI_HIDE();

  /**
   * This key is used to hold a vector of COMPONENT_VALUES (and, for
   * vtkDataArray subclasses, COMPONENT_RANGE) keys -- one
   * for each component of the array.  You may add additional per-component
   * key-value pairs to information objects in this vector. However if you
   * do so, you must be sure to either (1) set COMPONENT_VALUES to
   * an invalid variant and set COMPONENT_RANGE to
   * {VTK_DOUBLE_MAX, VTK_DOUBLE_MIN} or (2) call ComputeUniqueValues(component)
   * and ComputeRange(component) <b>before</b> modifying the information object.
   * Otherwise it is possible for modifications to the array to take place
   * without the bounds on the component being updated.
   */
  static vtkInformationInformationVectorKey* PER_COMPONENT();

  /**
   * This key is used to hold a vector of COMPONENT_VALUES (and, for
   * vtkDataArray subclasses, COMPONENT_RANGE) keys -- one
   * for each component of the array.  You may add additional per-component
   * key-value pairs to information objects in this vector. However if you
   * do so, you must be sure to either (1) set COMPONENT_VALUES to
   * an invalid variant and set COMPONENT_RANGE to
   * {VTK_DOUBLE_MAX, VTK_DOUBLE_MIN} or (2) call ComputeUniqueValues(component)
   * and ComputeFiniteRange(component) <b>before</b> modifying the information object.
   * Otherwise it is possible for modifications to the array to take place
   * without the bounds on the component being updated.
   */
  static vtkInformationInformationVectorKey* PER_FINITE_COMPONENT();

  /**
   * Removes out-of-date PER_COMPONENT() and PER_FINITE_COMPONENT() values.
   */
  void Modified() VTK_OVERRIDE;

  /**
   * A key used to hold discrete values taken on either by the tuples of the
   * array (when present in this->GetInformation()) or individual components
   * (when present in one entry of the PER_COMPONENT() information vector).
   */
  static vtkInformationVariantVectorKey* DISCRETE_VALUES();

  /**
   * A key used to hold conditions under which cached discrete values were generated;
   * the value is a 2-vector of doubles.
   * The first entry corresponds to the maximum uncertainty that prominent values
   * exist but have not been detected. The second entry corresponds to the smallest
   * relative frequency a value is allowed to have and still appear on the list.
   */
  static vtkInformationDoubleVectorKey* DISCRETE_VALUE_SAMPLE_PARAMETERS();

  // Deprecated.  Use vtkAbstractArray::MaxDiscreteValues instead.
  enum {
    MAX_DISCRETE_VALUES = 32
  };

  //@{
  /**
   * Get/Set the maximum number of prominent values this array may contain
   * before it is considered continuous.  Default value is 32.
   */
  vtkGetMacro(MaxDiscreteValues, unsigned int);
  vtkSetMacro(MaxDiscreteValues, unsigned int);
  //@}

  enum {
    AbstractArray = 0,
    DataArray,
    AoSDataArrayTemplate,
    SoADataArrayTemplate,
    TypedDataArray,
    MappedDataArray,

    DataArrayTemplate = AoSDataArrayTemplate //! Legacy
  };

  /**
   * Method for type-checking in FastDownCast implementations. See also
   * vtkArrayDownCast.
   */
  virtual int GetArrayType()
  {
    return AbstractArray;
  }

protected:
  // Construct object with default tuple dimension (number of components) of 1.
  vtkAbstractArray();
  ~vtkAbstractArray() VTK_OVERRIDE;

  /**
   * Set an information object that can be used to annotate the array.
   * Use this with caution as array instances depend on persistence of
   * information keys. See CopyInformation.
   */
  virtual void SetInformation( vtkInformation* );

  /**
   * Obtain the set of unique values taken on by each component of the array,
   * as well as by the tuples of the array.

   * The results are stored in the PER_COMPONENT() vtkInformation objects
   * using the DISCRETE_VALUES() key.
   * If the key is present but stores 0 values, the array either has no
   * entries or does not behave as a discrete set.
   * If the key is not present, the array has not been examined for
   * distinct values or has been modified since the last examination.
   */
  virtual void UpdateDiscreteValueSet(double uncertainty, double minProminence);

  vtkIdType Size;         // allocated size of data
  vtkIdType MaxId;        // maximum index inserted thus far
  int NumberOfComponents; // the number of components per tuple

  // maximum number of prominent values before array is considered continuous.
  unsigned int MaxDiscreteValues;

  char* Name;

  bool RebuildArray;      // whether to rebuild the fast lookup data structure.

  vtkInformation* Information;

  class vtkInternalComponentNames;
  vtkInternalComponentNames* ComponentNames; //names for each component

private:
  vtkAbstractArray(const vtkAbstractArray&) VTK_DELETE_FUNCTION;
  void operator=(const vtkAbstractArray&) VTK_DELETE_FUNCTION;
};

//@{
/**
 * Implementation of vtkArrayDownCast. The templating/etc is moved to this
 * worker struct to get around limitations of template functions (no partial
 * specialization, ambiguities, etc).
 */
template <typename ArrayT>
struct vtkArrayDownCast_impl
{
  inline ArrayT* operator()(vtkAbstractArray* array)
  {
    return ArrayT::SafeDownCast(array);
  }
};
//@}

/**
 * vtkArrayDownCast is to be used by generic (e.g. templated) code for quickly
 * downcasting vtkAbstractArray pointers to more derived classes.
 * The typical VTK downcast pattern (SafeDownCast) performs a string comparison
 * on the class names in the object's inheritance hierarchy, which is quite
 * expensive and can dominate computational resource usage when downcasting is
 * needed in a worker function.
 * To address this, certain arrays support a FastDownCast method, which replaces
 * the chain of string comparisons with 1-2 integer comparisons and thus is
 * significantly more efficient.
 * However, not all arrays support the FastDownCast mechanism. vtkArrayDownCast
 * exists to select between the two; Arrays that support FastDownCast will use
 * it, while others will fallback to the slower SafeDownCast.

 * A more detailed description of this class and related tools can be found
 * \ref VTK-7-1-ArrayDispatch "here".
 */
template <typename ArrayT>
ArrayT* vtkArrayDownCast(vtkAbstractArray *array)
{
  // The default vtkArrayDownCast_impl struct uses SafeDownCast, but is
  // specialized for arrays that support FastDownCast.
  return vtkArrayDownCast_impl<ArrayT>()(array);
}

//@{
/**
 * This macro is used to tell vtkArrayDownCast to use FastDownCast instead of
 * SafeDownCast.
 */
#define vtkArrayDownCast_FastCastMacro(ArrayT) \
  template <> struct vtkArrayDownCast_impl<ArrayT> \
  { \
    inline ArrayT* operator()(vtkAbstractArray *array) \
    { \
      return ArrayT::FastDownCast(array); \
    } \
  };
//@}

//@{
/**
 * Same as vtkArrayDownCast_FastCastMacro, but treats ArrayT as a
 * single-parameter template (the parameter is the value type). Defines a
 * vtkArrayDownCast implementation that uses the specified array template class
 * with any value type.
 */
#define vtkArrayDownCast_TemplateFastCastMacro(ArrayT) \
  template <typename ValueT> struct vtkArrayDownCast_impl<ArrayT<ValueT> > \
  { \
    inline ArrayT<ValueT>* operator()(vtkAbstractArray *array) \
    { \
      return ArrayT<ValueT>::FastDownCast(array); \
    } \
  };
//@}

#endif
