/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVariantArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/**
 * @class   vtkVariantArray
 * @brief   An array holding vtkVariants.
 *
 *
 *
 * @par Thanks:
 * Thanks to Patricia Crossno, Ken Moreland, Andrew Wilson and Brian Wylie from
 * Sandia National Laboratories for their help in developing this class.
*/

#ifndef vtkVariantArray_h
#define vtkVariantArray_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkAbstractArray.h"
#include "vtkVariant.h" // For variant type

class vtkVariantArrayLookup;

/// Forward declaration required for Boost serialization
namespace boost { namespace serialization { class access; } }

class VTKCOMMONCORE_EXPORT vtkVariantArray : public vtkAbstractArray
{

  /// Friendship required for Boost serialization
  friend class boost::serialization::access;

public:
  static vtkVariantArray* New();
  vtkTypeMacro(vtkVariantArray,vtkAbstractArray);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //
  // Functions required by vtkAbstractArray
  //

  /**
   * Allocate memory for this array. Delete old storage only if necessary.
   * Note that ext is no longer used.
   */
  int Allocate(vtkIdType sz, vtkIdType ext=1000) VTK_OVERRIDE;

  /**
   * Release storage and reset array to initial state.
   */
  void Initialize() VTK_OVERRIDE;

  /**
   * Return the underlying data type. An integer indicating data type is
   * returned as specified in vtkSetGet.h.
   */
  int GetDataType() VTK_OVERRIDE;

  /**
   * Return the size of the underlying data type.  For a bit, 1 is
   * returned.  For string 0 is returned. Arrays with variable length
   * components return 0.
   */
  int GetDataTypeSize() VTK_OVERRIDE;

  /**
   * Return the size, in bytes, of the lowest-level element of an
   * array.  For vtkDataArray and subclasses this is the size of the
   * data type.  For vtkStringArray, this is
   * sizeof(vtkStdString::value_type), which winds up being
   * sizeof(char).
   */
  int GetElementComponentSize() VTK_OVERRIDE;

  /**
   * Set the number of tuples (a component group) in the array. Note that
   * this may allocate space depending on the number of components.
   */
  void SetNumberOfTuples(vtkIdType number) VTK_OVERRIDE;

  /**
   * Set the tuple at the ith location using the jth tuple in the source array.
   * This method assumes that the two arrays have the same type
   * and structure. Note that range checking and memory allocation is not
   * performed; use in conjunction with SetNumberOfTuples() to allocate space.
   */
  void SetTuple(vtkIdType i, vtkIdType j,
                vtkAbstractArray* source) VTK_OVERRIDE;

  /**
   * Insert the jth tuple in the source array, at ith location in this array.
   * Note that memory allocation is performed as necessary to hold the data.
   */
  void InsertTuple(vtkIdType i, vtkIdType j,
                   vtkAbstractArray* source) VTK_OVERRIDE;

  /**
   * Copy the tuples indexed in srcIds from the source array to the tuple
   * locations indexed by dstIds in this array.
   * Note that memory allocation is performed as necessary to hold the data.
   */
  void InsertTuples(vtkIdList *dstIds, vtkIdList *srcIds,
                    vtkAbstractArray *source) VTK_OVERRIDE;

  /**
   * Copy n consecutive tuples starting at srcStart from the source array to
   * this array, starting at the dstStart location.
   * Note that memory allocation is performed as necessary to hold the data.
   */
  void InsertTuples(vtkIdType dstStart, vtkIdType n, vtkIdType srcStart,
                    vtkAbstractArray* source) VTK_OVERRIDE;

  /**
   * Insert the jth tuple in the source array, at the end in this array.
   * Note that memory allocation is performed as necessary to hold the data.
   * Returns the location at which the data was inserted.
   */
  vtkIdType InsertNextTuple(vtkIdType j, vtkAbstractArray* source) VTK_OVERRIDE;

  /**
   * Return a void pointer. For image pipeline interface and other
   * special pointer manipulation.
   */
  void *GetVoidPointer(vtkIdType id) VTK_OVERRIDE;

  /**
   * Deep copy of data. Implementation left to subclasses, which
   * should support as many type conversions as possible given the
   * data type.
   */
  void DeepCopy(vtkAbstractArray *da) VTK_OVERRIDE;

  /**
   * Set the ith tuple in this array as the interpolated tuple value,
   * given the ptIndices in the source array and associated
   * interpolation weights.
   * This method assumes that the two arrays are of the same type
   * and strcuture.
   */
  void InterpolateTuple(vtkIdType i, vtkIdList *ptIndices,
    vtkAbstractArray* source,  double* weights) VTK_OVERRIDE;

  /**
   * Insert the ith tuple in this array as interpolated from the two values,
   * p1 and p2, and an interpolation factor, t.
   * The interpolation factor ranges from (0,1),
   * with t=0 located at p1. This method assumes that the three arrays are of
   * the same type. p1 is value at index id1 in source1, while, p2 is
   * value at index id2 in source2.
   */
  void InterpolateTuple(vtkIdType i,
    vtkIdType id1, vtkAbstractArray* source1,
    vtkIdType id2, vtkAbstractArray* source2, double t) VTK_OVERRIDE;

  /**
   * Free any unnecessary memory.
   * Description:
   * Resize object to just fit data requirement. Reclaims extra memory.
   */
  void Squeeze() VTK_OVERRIDE;

  /**
   * Resize the array while conserving the data.  Returns 1 if
   * resizing succeeded and 0 otherwise.
   */
  int Resize(vtkIdType numTuples) VTK_OVERRIDE;

  //@{
  /**
   * This method lets the user specify data to be held by the array.  The
   * array argument is a pointer to the data.  size is the size of
   * the array supplied by the user.  Set save to 1 to keep the class
   * from deleting the array when it cleans up or reallocates memory.
   * The class uses the actual array provided; it does not copy the data
   * from the supplied array.
   */
  void SetVoidArray(void *arr, vtkIdType size, int save) VTK_OVERRIDE;
  void SetVoidArray(void *arr, vtkIdType size, int save,
                    int deleteM) VTK_OVERRIDE;
  //@}

  /**
   * Return the memory in kibibytes (1024 bytes) consumed by this data array. Used to
   * support streaming and reading/writing data. The value returned is
   * guaranteed to be greater than or equal to the memory required to
   * actually represent the data represented by this object. The
   * information returned is valid only after the pipeline has
   * been updated.
   */
  unsigned long GetActualMemorySize() VTK_OVERRIDE;

  /**
   * Since each item can be of a different type, we say that a variant array is not numeric.
   */
  int IsNumeric() VTK_OVERRIDE;

  /**
   * Subclasses must override this method and provide the right
   * kind of templated vtkArrayIteratorTemplate.
   */
  VTK_NEWINSTANCE vtkArrayIterator* NewIterator() VTK_OVERRIDE;

  //
  // Additional functions
  //

  /**
   * Get the data at a particular index.
   */
  vtkVariant & GetValue(vtkIdType id) const;

  /**
   * Set the data at a particular index. Does not do range checking. Make sure
   * you use the method SetNumberOfValues() before inserting data.
   */
  void SetValue(vtkIdType id, vtkVariant value);

  /**
   * If id < GetNumberOfValues(), overwrite the array at that index.
   * If id >= GetNumberOfValues(), expand the array size to id+1
   * and set the final value to the specified value.
   */
  void InsertValue(vtkIdType id, vtkVariant value);

  /**
   * Insert a value into the array from a variant.
   */
  void SetVariantValue(vtkIdType idx, vtkVariant value) VTK_OVERRIDE;

  /**
   * Safely insert a value into the array from a variant.
   */
  void InsertVariantValue(vtkIdType idx, vtkVariant value) VTK_OVERRIDE;

  /**
   * Expand the array by one and set the value at that location.
   * Return the array index of the inserted value.
   */
  vtkIdType InsertNextValue(vtkVariant value);

  /**
   * Return a pointer to the location in the internal array at the specified index.
   */
  vtkVariant* GetPointer(vtkIdType id);

  /**
   * Set the internal array used by this object.
   */
  void SetArray(vtkVariant* arr, vtkIdType size, int save);

  /**
   * Specify the number of values for this object to hold. Does an
   * allocation as well as setting the MaxId ivar. Used in conjunction with
   * SetValue() method for fast insertion.
   */
  void SetNumberOfValues(vtkIdType number) VTK_OVERRIDE;

  /**
   * Return the number of values in the array.
   */
  vtkIdType GetNumberOfValues() { return this->MaxId + 1; }

  //@{
  /**
   * Return the indices where a specific value appears.
   */
  vtkIdType LookupValue(vtkVariant value) VTK_OVERRIDE;
  void LookupValue(vtkVariant value, vtkIdList* ids) VTK_OVERRIDE;
  //@}

  /**
   * Tell the array explicitly that the data has changed.
   * This is only necessary to call when you modify the array contents
   * without using the array's API (i.e. you retrieve a pointer to the
   * data and modify the array contents).  You need to call this so that
   * the fast lookup will know to rebuild itself.  Otherwise, the lookup
   * functions will give incorrect results.
   */
  void DataChanged() VTK_OVERRIDE;

  /**
   * Tell the array explicitly that a single data element has
   * changed. Like DataChanged(), then is only necessary when you
   * modify the array contents without using the array's API.
   */
  virtual void DataElementChanged(vtkIdType id);

  /**
   * Delete the associated fast lookup data structure on this array,
   * if it exists.  The lookup will be rebuilt on the next call to a lookup
   * function.
   */
  void ClearLookup() VTK_OVERRIDE;

  /**
   * This destructor is public to work around a bug in version 1.36.0 of
   * the Boost.Serialization library.
   */
  ~vtkVariantArray() VTK_OVERRIDE;

protected:
  // Construct object with default tuple dimension (number of components) of 1.
  vtkVariantArray();

  // Pointer to data

  vtkVariant* Array;

  // Function to resize data
  vtkVariant* ResizeAndExtend(vtkIdType sz);

  int SaveUserArray;

private:
  vtkVariantArray(const vtkVariantArray&) VTK_DELETE_FUNCTION;
  void operator=(const vtkVariantArray&) VTK_DELETE_FUNCTION;

  vtkVariantArrayLookup* Lookup;
  void UpdateLookup();
};

#endif
