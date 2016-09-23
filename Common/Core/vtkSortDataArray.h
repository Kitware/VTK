/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSortDataArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2003 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

/**
 * @class   vtkSortDataArray
 * @brief   provides several methods for sorting VTK arrays.
 *
 *
 * vtkSortDataArray is used to sort data, based on its value, or with an
 * associated key, into either ascending or descending order. This is useful
 * for operations like selection, or analysis, when evaluating and processing
 * data. A variety of sorting functions are provided, treating both arrays
 * (i.e., vtkAbstractArray) and id lists (vtkIdList). Note that complex arrays
 * like variants and string arrays are also handled.
 *
 * Additional functionality is provided to generate data ordering, without
 * necessarily shuffling the data into a final, sorted position. Hence, the
 * sorting process is organized into three steps because of the complexity of
 * dealing with multiple types and multiple component data arrays. The first
 * step involves creating and initializing a sorted index array, and then
 * (second step) sorting this array to produce a map indicating the sorting
 * order.  In other words, the sorting index array is a permutation which can
 * be applied to other, associated data to shuffle it (third step) into an
 * order consistent with the sorting operation. Note that the generation of
 * the sorted index array is useful unto itself (even without the final
 * shuffling of data) because it generates an ordered list (from the data
 * values of any component in any array). So for example, it is possible to
 * find the top N cells with the largest scalar value simply by generating
 * the sorting index array from the call scalar values.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly on
 * multi-core machines.
 *
 * @warning
 * The sort methods below are static, hence the sorting methods can be
 * used without instantiating the class. All methods are thread safe.
 *
 * @sa
 * vtkSortFieldData
*/

#ifndef vtkSortDataArray_h
#define vtkSortDataArray_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"

class vtkIdList;
class vtkAbstractArray;


class VTKCOMMONCORE_EXPORT vtkSortDataArray : public vtkObject
{
public:
  //@{
  /**
   * Standard VTK methods for instantiating, managing type, and printing
   * information about this class.
   */
  static vtkSortDataArray *New();
  vtkTypeMacro(vtkSortDataArray, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

  /**
   * Sorts the given array in ascending order. For this method, the keys must
   * be single-component tuples.
   */
  static void Sort(vtkIdList *keys)
    {vtkSortDataArray::Sort(keys,0);}
  static void Sort(vtkAbstractArray *keys)
    {vtkSortDataArray::Sort(keys,0);}

  //@{
  /**
   * Sorts the given array in either ascending (dir=0) or descending (dir!=0)
   * order. For this method, the keys must be single-component tuples.
   */
  static void Sort(vtkIdList *keys, int dir);
  static void Sort(vtkAbstractArray *keys, int dir);
  //@}

  /**
   * Sorts the given key/value pairs based on the keys (the keys are expected
   * to be 1-tuples, values may have number of components >= 1).
   * Obviously, the two arrays must be of equal size. Sorts in ascending
   * order.
   */
  static void Sort(vtkAbstractArray *keys, vtkAbstractArray *values)
    {vtkSortDataArray::Sort(keys,values,0);}
  static void Sort(vtkAbstractArray *keys, vtkIdList *values)
    {vtkSortDataArray::Sort(keys,values,0);}

  //@{
  /**
   * Sorts the given key/value pairs based on the keys (the keys are expected
   * to be 1-tuples, values may have number of components >= 1).
   * Obviously, the two arrays must be of equal size. Sorts in either
   * ascending (dir=0) or descending (dir=1) order.
   */
  static void Sort(vtkAbstractArray *keys, vtkAbstractArray *values, int dir);
  static void Sort(vtkAbstractArray *keys, vtkIdList *values, int dir);
  //@}

  /**
   * Sorts the given data array using the specified component as a key.
   * Think of the array as a 2-D grid with each tuple representing a row.
   * Tuples are swapped until the \a k-th column of the grid is
   * monotonically increasing. Where two tuples have the same value for
   * the \a k-th component, their order in the final result is unspecified.
   */
  static void SortArrayByComponent( vtkAbstractArray* arr, int k)
    {vtkSortDataArray::SortArrayByComponent(arr,k,0);}

  /**
   * Sorts the given data array using the specified component as a key.
   * Think of the array as a 2-D grid with each tuple representing a row.
   * Tuples are swapped until the \a k-th column of the grid is ascending
   * (dir=0) or descending (dir=1). Where two tuples have the same value for
   * the \a k-th component, their order in the final result is unspecified.
   */
  static void SortArrayByComponent( vtkAbstractArray* arr, int k, int dir);

  //@{
  /**
   * The following are general functions which can be used to produce an
   * ordering, and/or sort various types of VTK arrays. Don't use these
   * methods unless you really know what you are doing. The basic idea is
   * that an initial set of indices (InitializeSortIndices() that refer to
   * the data contained in a vtkAbstractArray or vtkIdList) are sorted
   * (GenerateSortIndices() based on the data values in the array). The
   * result of the sort is the creation of a permutation array (the sort
   * array idx) that indicates where the data tuples originated (e.g., after
   * the sort, idx[0] indicates where in the array the tuple was originally
   * located prior to sorting.) This sorted index array can be used to
   * shuffle various types of VTK arrays (the types supported correspond to
   * the various arrays which are subclasses of vtkDataArrayTemplate, use
   * ShuffleArray() or for vtkIdList, use ShuffleIdList()). Also, the sort
   * array, being an vtkIdType* (i.e., id list), can also be used to identify
   * points or cells in sorted order (based on the data in the originating
   * dataIn array). Note that sorting is always performed in ascending order,
   * and the sorted index array reflects this; however the shuffling of data
   * can be specified as either ascending (dir=0) or descending (dir=1)
   * order. The user is responsible for taking ownership of the sort indices
   * (i.e., free the idx array).
   */
  static vtkIdType *InitializeSortIndices(vtkIdType numKeys);
  static void GenerateSortIndices(int dataType, void *dataIn, vtkIdType numKeys,
                                  int numComp, int k, vtkIdType *idx);
  static void ShuffleArray(vtkIdType *idx, int dataType, vtkIdType numKeys,
                           int numComp, vtkAbstractArray *arr,
                           void *dataIn, int dir);
  static void ShuffleIdList(vtkIdType *idx, vtkIdType sze, vtkIdList *arrayIn,
                            vtkIdType *dataIn, int dir);
  //@}

protected:
  vtkSortDataArray();
  ~vtkSortDataArray() VTK_OVERRIDE;

  // A more efficient sort for single component arrays. This is delegated to
  // by the methods above (if appropriate).
  static void GenerateSort1Indices(int dataType, void *dataIn, vtkIdType numKeys,
                                   vtkIdType *idx);

  // A more efficient shuffle for single component arrays. This is delegated to
  // by the methods above (if appropriate).
  static void Shuffle1Array(vtkIdType *idx, int dataType, vtkIdType numKeys,
                            vtkAbstractArray *arr, void *dataIn, int dir);

private:
  vtkSortDataArray(const vtkSortDataArray &) VTK_DELETE_FUNCTION;
  void operator=(const vtkSortDataArray &) VTK_DELETE_FUNCTION;
};

#endif //vtkSortDataArray_h
