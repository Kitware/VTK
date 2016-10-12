/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkAbstractArray.h"

class vtkDoubleArray;
class vtkIdList;
class vtkInformationStringKey;
class vtkInformationDoubleVectorKey;
class vtkLookupTable;
class vtkPoints;

class VTKCOMMONCORE_EXPORT vtkDataArray : public vtkAbstractArray
{
public:
  vtkTypeMacro(vtkDataArray,vtkAbstractArray);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Perform a fast, safe cast from a vtkAbstractArray to a vtkDataArray.
   * This method checks if source->GetArrayType() returns DataArray
   * or a more derived type, and performs a static_cast to return
   * source as a vtkDataArray pointer. Otherwise, NULL is returned.
   */
  static vtkDataArray* FastDownCast(vtkAbstractArray *source);

  /**
   * This method is here to make backward compatibility easier.  It
   * must return true if and only if an array contains numeric data.
   * All vtkDataArray subclasses contain numeric data, hence this method
   * always returns 1(true).
   */
  int IsNumeric() VTK_OVERRIDE
    { return 1; }

  /**
   * Return the size, in bytes, of the lowest-level element of an
   * array.  For vtkDataArray and subclasses this is the size of the
   * data type.
   */
  int GetElementComponentSize() VTK_OVERRIDE
    { return this->GetDataTypeSize(); }

  // Reimplemented virtuals (doc strings are inherited from superclass):
  void InsertTuple(vtkIdType dstTupleIdx, vtkIdType srcTupleIdx,
                   vtkAbstractArray* source) VTK_OVERRIDE;
  vtkIdType InsertNextTuple(vtkIdType srcTupleIdx,
                                    vtkAbstractArray* source) VTK_OVERRIDE;
  void InsertTuples(vtkIdList *dstIds, vtkIdList *srcIds,
                            vtkAbstractArray *source) VTK_OVERRIDE;
  void InsertTuples(vtkIdType dstStart, vtkIdType n, vtkIdType srcStart,
                            vtkAbstractArray* source) VTK_OVERRIDE;
  void GetTuples(vtkIdList *tupleIds, vtkAbstractArray *output) VTK_OVERRIDE;
  void GetTuples(vtkIdType p1, vtkIdType p2, vtkAbstractArray *output) VTK_OVERRIDE;
  void InterpolateTuple(vtkIdType dstTupleIdx, vtkIdList *ptIndices,
                                vtkAbstractArray* source,  double* weights) VTK_OVERRIDE;
  void InterpolateTuple(vtkIdType dstTupleIdx,
    vtkIdType srcTupleIdx1, vtkAbstractArray* source1,
    vtkIdType srcTupleIdx2, vtkAbstractArray* source2, double t) VTK_OVERRIDE;

  /**
   * Get the data tuple at tupleIdx. Return it as a pointer to an array.
   * Note: this method is not thread-safe, and the pointer is only valid
   * as long as another method invocation to a vtk object is not performed.
   */
  virtual double *GetTuple(vtkIdType tupleIdx) = 0;

  /**
   * Get the data tuple at tupleIdx by filling in a user-provided array,
   * Make sure that your array is large enough to hold the NumberOfComponents
   * amount of data being returned.
   */
  virtual void GetTuple(vtkIdType tupleIdx, double * tuple) = 0;

  //@{
  /**
   * These methods are included as convenience for the wrappers.
   * GetTuple() and SetTuple() which return/take arrays can not be
   * used from wrapped languages. These methods can be used instead.
   */
  double GetTuple1(vtkIdType tupleIdx);
  double* GetTuple2(vtkIdType tupleIdx);
  double* GetTuple3(vtkIdType tupleIdx);
  double* GetTuple4(vtkIdType tupleIdx);
  double* GetTuple6(vtkIdType tupleIdx);
  double* GetTuple9(vtkIdType tupleIdx);
  //@}

  void SetTuple(vtkIdType dstTupleIdx, vtkIdType srcTupleIdx,
                        vtkAbstractArray* source) VTK_OVERRIDE;

  //@{
  /**
   * Set the data tuple at tupleIdx. Note that range checking or
   * memory allocation is not performed; use this method in conjunction
   * with SetNumberOfTuples() to allocate space.
   */
  virtual void SetTuple(vtkIdType tupleIdx, const float * tuple);
  virtual void SetTuple(vtkIdType tupleIdx, const double * tuple);
  //@}

  //@{
  /**
   * These methods are included as convenience for the wrappers.
   * GetTuple() and SetTuple() which return/take arrays can not be
   * used from wrapped languages. These methods can be used instead.
   */
  void SetTuple1(vtkIdType tupleIdx, double value);
  void SetTuple2(vtkIdType tupleIdx, double val0, double val1);
  void SetTuple3(vtkIdType tupleIdx, double val0, double val1, double val2);
  void SetTuple4(vtkIdType tupleIdx, double val0, double val1, double val2,
                 double val3);
  void SetTuple6(vtkIdType tupleIdx, double val0, double val1, double val2,
                 double val3, double val4, double val5);
  void SetTuple9(vtkIdType tupleIdx, double val0, double val1, double val2,
                 double val3, double val4, double val5, double val6,
                 double val7, double val8);
  //@}

  //@{
  /**
   * Insert the data tuple at tupleIdx. Note that memory allocation
   * is performed as necessary to hold the data.
   */
  virtual void InsertTuple(vtkIdType tupleIdx, const float * tuple) = 0;
  virtual void InsertTuple(vtkIdType tupleIdx, const double * tuple) = 0;
  //@}

  //@{
  /**
   * These methods are included as convenience for the wrappers.
   * InsertTuple() which takes arrays can not be
   * used from wrapped languages. These methods can be used instead.
   */
  void InsertTuple1(vtkIdType tupleIdx, double value);
  void InsertTuple2(vtkIdType tupleIdx, double val0, double val1);
  void InsertTuple3(vtkIdType tupleIdx, double val0, double val1, double val2);
  void InsertTuple4(vtkIdType tupleIdx, double val0, double val1, double val2,
                    double val3);
  void InsertTuple6(vtkIdType tupleIdx, double val0, double val1, double val2,
                    double val3, double val4, double val5);
  void InsertTuple9(vtkIdType tupleIdx, double val0, double val1, double val2,
                    double val3, double val4, double val5, double val6,
                    double val7, double val8);
  //@}

  //@{
  /**
   * Insert the data tuple at the end of the array and return the tuple index at
   * which the data was inserted. Memory is allocated as necessary to hold
   * the data.
   */
  virtual vtkIdType InsertNextTuple(const float * tuple) = 0;
  virtual vtkIdType InsertNextTuple(const double * tuple) = 0;
  //@}

  //@{
  /**
   * These methods are included as convenience for the wrappers.
   * InsertTuple() which takes arrays can not be
   * used from wrapped languages. These methods can be used instead.
   */
  void InsertNextTuple1(double value);
  void InsertNextTuple2(double val0, double val1);
  void InsertNextTuple3(double val0, double val1, double val2);
  void InsertNextTuple4(double val0, double val1, double val2,
                        double val3);
  void InsertNextTuple6(double val0, double val1, double val2,
                        double val3, double val4, double val5);
  void InsertNextTuple9(double val0, double val1, double val2,
                        double val3, double val4, double val5, double val6,
                        double val7, double val8);
  //@}

  //@{
  /**
   * These methods remove tuples from the data array. They shift data and
   * resize array, so the data array is still valid after this operation. Note,
   * this operation is fairly slow.
   */
  virtual void RemoveTuple(vtkIdType tupleIdx) = 0;
  virtual void RemoveFirstTuple() { this->RemoveTuple(0); }
  virtual void RemoveLastTuple();
  //@}

  /**
   * Return the data component at the location specified by tupleIdx and
   * compIdx.
   */
  virtual double GetComponent(vtkIdType tupleIdx, int compIdx);

  /**
   * Set the data component at the location specified by tupleIdx and compIdx
   * to value.
   * Note that i is less than NumberOfTuples and j is less than
   * NumberOfComponents. Make sure enough memory has been allocated
   * (use SetNumberOfTuples() and SetNumberOfComponents()).
   */
  virtual void SetComponent(vtkIdType tupleIdx, int compIdx, double value);

  /**
   * Insert value at the location specified by tupleIdx and compIdx.
   * Note that memory allocation is performed as necessary to hold the data.
   */
  virtual void InsertComponent(vtkIdType tupleIdx, int compIdx, double value);

  /**
   * Get the data as a double array in the range (tupleMin,tupleMax) and
   * (compMin, compMax). The resulting double array consists of all data in
   * the tuple range specified and only the component range specified. This
   * process typically requires casting the data from native form into
   * doubleing point values. This method is provided as a convenience for data
   * exchange, and is not very fast.
   */
  virtual void GetData(vtkIdType tupleMin, vtkIdType tupleMax, int compMin,
                       int compMax, vtkDoubleArray* data);

  //@{
  /**
   * Deep copy of data. Copies data from different data arrays even if
   * they are different types (using doubleing-point exchange).
   */
  void DeepCopy(vtkAbstractArray *aa) VTK_OVERRIDE;
  virtual void DeepCopy(vtkDataArray *da);
  //@}

  /**
   * Create a shallow copy of other into this, if possible. Shallow copies are
   * only possible:
   * (a) if both arrays are the same data type
   * (b) if both arrays are the same array type (e.g. AOS vs. SOA)
   * (c) if both arrays support shallow copies (e.g. vtkBitArray currently
   * does not.)
   * If a shallow copy is not possible, a deep copy will be performed instead.
   */
  virtual void ShallowCopy(vtkDataArray *other);

  /**
   * Fill a component of a data array with a specified value. This method
   * sets the specified component to specified value for all tuples in the
   * data array.  This methods can be used to initialize or reinitialize a
   * single component of a multi-component array.
   */
  virtual void FillComponent(int compIdx, double value);

  /**
   * Copy a component from one data array into a component on this data array.
   * This method copies the specified component ("srcComponent") from the
   * specified data array ("src") to the specified component ("dstComponent")
   * over all the tuples in this data array.  This method can be used to extract
   * a component (column) from one data array and paste that data into
   * a component on this data array.
   */
  virtual void CopyComponent(int dstComponent, vtkDataArray *src,
                             int srcComponent);

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
  unsigned long GetActualMemorySize() VTK_OVERRIDE;

  /**
   * Create default lookup table. Generally used to create one when none
   * is available.
   */
  void CreateDefaultLookupTable();

  //@{
  /**
   * Set/get the lookup table associated with this scalar data, if any.
   */
  void SetLookupTable(vtkLookupTable *lut);
  vtkGetObjectMacro(LookupTable,vtkLookupTable);
  //@}

  /**
   * The range of the data array values for the given component will be
   * returned in the provided range array argument. If comp is -1, the range
   * of the magnitude (L2 norm) over all components will be provided. The
   * range is computed and then cached, and will not be re-computed on
   * subsequent calls to GetRange() unless the array is modified or the
   * requested component changes.
   * THIS METHOD IS NOT THREAD SAFE.
   */
  void GetRange(double range[2], int comp)
  {
    this->ComputeRange(range, comp);
  }

  //@{
  /**
   * Return the range of the data array values for the given component. If
   * comp is -1, return the range of the magnitude (L2 norm) over all
   * components.The range is computed and then cached, and will not be
   * re-computed on subsequent calls to GetRange() unless the array is
   * modified or the requested component changes.
   * THIS METHOD IS NOT THREAD SAFE.
   */
  double* GetRange(int comp)
  {
    this->GetRange(this->Range, comp);
    return this->Range;
  }
  //@}

  /**
   * Return the range of the data array. If the array has multiple components,
   * then this will return the range of only the first component (component
   * zero). The range is computed and then cached, and will not be re-computed
   * on subsequent calls to GetRange() unless the array is modified.
   * THIS METHOD IS NOT THREAD SAFE.
   */
  double* GetRange()
  {
    return this->GetRange(0);
  }

  /**
   * The the range of the data array values will be returned in the provided
   * range array argument. If the data array has multiple components, then
   * this will return the range of only the first component (component zero).
   * The range is computend and then cached, and will not be re-computed on
   * subsequent calls to GetRange() unless the array is modified.
   * THIS METHOD IS NOT THREAD SAFE.
   */
  void GetRange(double range[2])
  {
    this->GetRange(range,0);
  }

  //@{
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
  //@}

  /**
   * Return the maximum norm for the tuples.
   * Note that the max. is computed every time GetMaxNorm is called.
   */
  virtual double GetMaxNorm();

  /**
   * Creates an array for dataType where dataType is one of
   * VTK_BIT, VTK_CHAR, VTK_SIGNED_CHAR, VTK_UNSIGNED_CHAR, VTK_SHORT,
   * VTK_UNSIGNED_SHORT, VTK_INT, VTK_UNSIGNED_INT, VTK_LONG,
   * VTK_UNSIGNED_LONG, VTK_DOUBLE, VTK_DOUBLE, VTK_ID_TYPE.
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
   * A human-readable string indicating the units for the array data.
   */
  static vtkInformationStringKey *UNITS_LABEL();

  /**
   * Copy information instance. Arrays use information objects
   * in a variety of ways. It is important to have flexibility in
   * this regard because certain keys should not be coppied, while
   * others must be. NOTE: Up to the implmeneter to make sure that
   * keys not inteneded to be coppied are excluded here.
   */
  int CopyInformation(vtkInformation *infoFrom, int deep=1) VTK_OVERRIDE;

  /**
   * Method for type-checking in FastDownCast implementations.
   */
  int GetArrayType() VTK_OVERRIDE { return DataArray; }

protected:

  friend class vtkPoints;

  /**
   * Compute the range for a specific component. If comp is set -1
   * then L2 norm is computed on all components. Call ClearRange
   * to force a recomputation if it is needed. The range is copied
   * to the range argument.
   * THIS METHOD IS NOT THREAD SAFE.
   */
  virtual void ComputeRange(double range[2], int comp);

  /**
   * Computes the range for each component of an array, the length
   * of \a ranges must be two times the number of components.
   * Returns true if the range was computed. Will return false
   * if you try to compute the range of an array of length zero.
   */
  virtual bool ComputeScalarRange(double* ranges);

  // Returns true if the range was computed. Will return false
  // if you try to compute the range of an array of length zero.
  virtual bool ComputeVectorRange(double range[2]);

  // Construct object with default tuple dimension (number of components) of 1.
  vtkDataArray();
  ~vtkDataArray() VTK_OVERRIDE;

  vtkLookupTable *LookupTable;
  double Range[2];

private:
  double* GetTupleN(vtkIdType i, int n);

private:
  vtkDataArray(const vtkDataArray&) VTK_DELETE_FUNCTION;
  void operator=(const vtkDataArray&) VTK_DELETE_FUNCTION;
};

//------------------------------------------------------------------------------
inline vtkDataArray* vtkDataArray::FastDownCast(vtkAbstractArray *source)
{
  if (source)
  {
    switch (source->GetArrayType())
    {
      case AoSDataArrayTemplate:
      case SoADataArrayTemplate:
      case TypedDataArray:
      case DataArray:
      case MappedDataArray:
        return static_cast<vtkDataArray*>(source);
      default:
        break;
    }
  }
  return NULL;
}

vtkArrayDownCast_FastCastMacro(vtkDataArray)

#endif
