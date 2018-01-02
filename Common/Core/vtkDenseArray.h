/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDenseArray.h

-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkDenseArray
 * @brief   Contiguous storage for N-way arrays.
 *
 *
 * vtkDenseArray is a concrete vtkArray implementation that stores values
 * using a contiguous block of memory.  Values are stored with fortran ordering,
 * meaning that if you iterated over the memory block, the left-most coordinates
 * would vary the fastest.
 *
 * In addition to the retrieval and update methods provided by vtkTypedArray,
 * vtkDenseArray provides methods to:
 *
 * Fill the entire array with a specific value.
 *
 * Retrieve a pointer to the storage memory block.
 *
 * @sa
 * vtkArray, vtkTypedArray, vtkSparseArray
 *
 * @par Thanks:
 * Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.
*/

#ifndef vtkDenseArray_h
#define vtkDenseArray_h

#include "vtkArrayCoordinates.h"
#include "vtkObjectFactory.h"
#include "vtkTypedArray.h"

template<typename T>
class vtkDenseArray : public vtkTypedArray<T>
{
public:
  static vtkDenseArray<T>* New();
  vtkTemplateTypeMacro(vtkDenseArray<T>, vtkTypedArray<T>)
  void PrintSelf(ostream &os, vtkIndent indent) override;

  typedef typename vtkArray::CoordinateT CoordinateT;
  typedef typename vtkArray::DimensionT DimensionT;
  typedef typename vtkArray::SizeT SizeT;

  // vtkArray API
  bool IsDense() override;
  const vtkArrayExtents& GetExtents() override;
  SizeT GetNonNullSize() override;
  void GetCoordinatesN(const SizeT n, vtkArrayCoordinates& coordinates) override;
  vtkArray* DeepCopy() override;

  // vtkTypedArray API
  const T& GetValue(CoordinateT i) override;
  const T& GetValue(CoordinateT i, CoordinateT j) override;
  const T& GetValue(CoordinateT i, CoordinateT j, CoordinateT k) override;
  const T& GetValue(const vtkArrayCoordinates& coordinates) override;
  const T& GetValueN(const SizeT n) override;
  void SetValue(CoordinateT i, const T& value) override;
  void SetValue(CoordinateT i, CoordinateT j, const T& value) override;
  void SetValue(CoordinateT i, CoordinateT j, CoordinateT k, const T& value) override;
  void SetValue(const vtkArrayCoordinates& coordinates, const T& value) override;
  void SetValueN(const SizeT n, const T& value) override;

  // vtkDenseArray API

  /**
   * Strategy object that contains a block of memory to be used by vtkDenseArray
   * for value storage.  The MemoryBlock object is responsible for freeing
   * memory when destroyed.
   */
  class MemoryBlock
  {
  public:
    virtual ~MemoryBlock();
    //@{
    /**
     * Returns a pointer to the block of memory to be used for storage.
     */
    virtual T* GetAddress() = 0;
  };
    //@}

  //@{
  /**
   * MemoryBlock implementation that manages internally-allocated memory using
   * new[] and delete[].  Note: HeapMemoryBlock is the default used by vtkDenseArray
   * for its "normal" internal memory allocation.
   */
  class HeapMemoryBlock :
    public MemoryBlock
  {
  public:
    HeapMemoryBlock(const vtkArrayExtents& extents);
    ~HeapMemoryBlock() override;
    T* GetAddress() override;
  //@}

  private:
    T* Storage;
  };

  //@{
  /**
   * MemoryBlock implementation that manages a static (will not be freed) memory block.
   */
  class StaticMemoryBlock :
    public MemoryBlock
  {
  public:
    StaticMemoryBlock(T* const storage);
    T* GetAddress() override;
  //@}

  private:
    T* Storage;
  };

  /**
   * Initializes the array to use an externally-allocated memory block.  The supplied
   * MemoryBlock must be large enough to store extents.GetSize() values.  The contents of
   * the memory must be stored contiguously with fortran ordering,

   * Dimension-labels are undefined after calling ExternalStorage() - you should
   * initialize them accordingly.

   * The array will use the supplied memory for storage until the array goes out of
   * scope, is configured to use a different memory block by calling ExternalStorage()
   * again, or is configured to use internally-allocated memory by calling Resize().

   * Note that the array will delete the supplied memory block when it is no longer in use.
   * caller's responsibility to ensure that the memory does not go out-of-scope until
   * the array has been destroyed or is no longer using it.
   */
  void ExternalStorage(const vtkArrayExtents& extents, MemoryBlock* storage);

  /**
   * Fills every element in the array with the given value.
   */
  void Fill(const T& value);

  /**
   * Returns a value by-reference, which is useful for performance and code-clarity.
   */
  T& operator[](const vtkArrayCoordinates& coordinates);

  /**
   * Returns a read-only reference to the underlying storage.  Values are stored
   * contiguously with fortran ordering.
   */
  const T* GetStorage() const;

  /**
   * Returns a mutable reference to the underlying storage.  Values are stored
   * contiguously with fortran ordering.  Use at your own risk!
   */
  T* GetStorage();

protected:
  vtkDenseArray();
  ~vtkDenseArray() override;

private:
  vtkDenseArray(const vtkDenseArray&) = delete;
  void operator=(const vtkDenseArray&) = delete;

  void InternalResize(const vtkArrayExtents& extents) override;
  void InternalSetDimensionLabel(DimensionT i, const vtkStdString& label) override;
  vtkStdString InternalGetDimensionLabel(DimensionT i) override;
  inline vtkIdType MapCoordinates(CoordinateT i);
  inline vtkIdType MapCoordinates(CoordinateT i, CoordinateT j);
  inline vtkIdType MapCoordinates(CoordinateT i, CoordinateT j, CoordinateT k);
  inline vtkIdType MapCoordinates(const vtkArrayCoordinates& coordinates);

  void Reconfigure(const vtkArrayExtents& extents, MemoryBlock* storage);

  typedef vtkDenseArray<T> ThisT;

  /**
   * Stores the current array extents (its size along each dimension)
   */
  vtkArrayExtents Extents;

  /**
   * Stores labels for each array dimension
   */
  std::vector<vtkStdString> DimensionLabels;

  /**
   * Manages array value memory storage.
   */
  MemoryBlock* Storage;

  //@{
  /**
   * Stores array values using a contiguous range of memory
   * with constant-time value lookup.
   */
  T* Begin;
  T* End;
  //@}

  /**
   * Stores the offset along each array dimension (used for fast lookups).
   */
  std::vector<vtkIdType> Offsets;
  //@{
  /**
   * Stores the stride along each array dimension (used for fast lookups).
   */
  std::vector<vtkIdType> Strides;
};
  //@}

#include "vtkDenseArray.txx"

#endif

// VTK-HeaderTest-Exclude: vtkDenseArray.h
