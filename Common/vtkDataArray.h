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
// .NAME vtkDataArray - abstract superclass for arrays
// .SECTION Description
// vtkDataArray is an abstract superclass for data array objects. This class
// defines an API that all array objects must support. Note that the concrete
// subclasses of this class represent data in native form (char, int, etc.) and
// often have specialized more efficient methods for operating on this data 
// (for example, getting pointers to data or getting/inserting data in native
// form). 
//
// The logical structure of this class is an array of tuples, where each
// tuple is made up of n-components (also called a component group), and n is
// the number of component values in a tuple(n >= 1).  Another view of this
// class is a mxn matrix, where m is the number of tuples, and n is the
// number of components in a tuple. Thus vtkDataArray can be used to
// represent scalars (1-4 components), 3D vectors (3 components), texture
// coordinates (1-3 components), tensors, (9 components) and so on.
// 
// Each data array is required to have a character-string name. The 
// naming of the array occurs automatically when it is instantiated, but 
// you are free to name arrays using the SetName() method. 
// (The array name is used for data manipulation.)
//
// .SECTION See Also
// vtkBitArray vtkCharArray vtkUnsignedCharArray vtkShortArray
// vtkUnsignedShortArray vtkIntArray vtkUnsignedIntArray vtkLongArray
// vtkUnsignedLongArray vtkDoubleArray vtkDoubleArray

#ifndef __vtkDataArray_h
#define __vtkDataArray_h

#include "vtkObject.h"

class vtkDoubleArray;
class vtkLookupTable;
class vtkIdList;

#define VTK_MAXIMUM_NUMBER_OF_CACHED_COMPONENT_RANGES 11

class VTK_COMMON_EXPORT vtkDataArray : public vtkObject 
{
public:
  vtkTypeRevisionMacro(vtkDataArray,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Allocate memory for this array. Delete old storage only if necessary.
  // Note that ext is no longer used.
  virtual int Allocate(vtkIdType sz, vtkIdType ext=1000) = 0;

  // Description:
  // Release storage and reset array to initial state.
  virtual void Initialize() = 0;

  // Description:
  // Return the underlying data type. An integer indicating data type is 
  // returned as specified in vtkSetGet.h.
  virtual int GetDataType() = 0;

  // Description:
  // Return the size of the underlying data type.  For a bit, 0 is returned.
  virtual int GetDataTypeSize() = 0;
  static unsigned long GetDataTypeSize(int type);

  // Description:
  // Set/Get the dimension (n) of the components. Must be >= 1. Make sure that
  // this is set before allocation.
  vtkSetClampMacro(NumberOfComponents,int,1,VTK_LARGE_INTEGER);
  int GetNumberOfComponents() {return this->NumberOfComponents;};

  // Description:
  // Set the number of tuples (a component group) in the array. Note that 
  // this may allocate space depending on the number of components.
  virtual void SetNumberOfTuples(vtkIdType number) = 0;

  // Description:
  // Get the number of tuples (a component group) in the array.
  vtkIdType GetNumberOfTuples() 
    {return (this->MaxId + 1)/this->NumberOfComponents;}

  // Description:
  // Get the data tuple at ith location. Return it as a pointer to an array.
  // Note: this method is not thread-safe, and the pointer is only valid
  // as long as another method invocation to a vtk object is not performed.
  virtual double *GetTuple(vtkIdType i) = 0;

  // Description:
  // Get the data tuple at ith location by filling in a user-provided array,
  // Make sure that your array is large enough to hold the NumberOfComponents
  // amount of data being returned.
  virtual void GetTuple(vtkIdType i, double * tuple) = 0;

  // Description:
  // These methods are included as convenience for the wrappers.
  // GetTuple() and SetTuple() which return/take arrays can not be 
  // used from wrapped languages. These methods can be used instead.
  double GetTuple1(vtkIdType i);
  double* GetTuple2(vtkIdType i);
  double* GetTuple3(vtkIdType i);
  double* GetTuple4(vtkIdType i);
  double* GetTuple9(vtkIdType i);

  // Description:
  // Given a list of point ids, return an array of tuples.
  // You must insure that the output array has been previously 
  // allocated with enough space to hold the data.
  void GetTuples(vtkIdList *ptIds, vtkDataArray *output);

  // Description:
  // Get the tuples for the range of points ids specified 
  // (i.e., p1->p2 inclusive). You must insure that the output array has 
  // been previously allocated with enough space to hold the data.
  void GetTuples(vtkIdType p1, vtkIdType p2, vtkDataArray *output);

  // Description:
  // Set the data tuple at ith location. Note that range checking or
  // memory allocation is not performed; use this method in conjunction
  // with SetNumberOfTuples() to allocate space.
  virtual void SetTuple(vtkIdType i, const float * tuple) = 0;
  virtual void SetTuple(vtkIdType i, const double * tuple) = 0;

  // Description:
  // These methods are included as convenience for the wrappers.
  // GetTuple() and SetTuple() which return/take arrays can not be 
  // used from wrapped languages. These methods can be used instead.
  void SetTuple1(vtkIdType i, double value);
  void SetTuple2(vtkIdType i, double val0, double val1);
  void SetTuple3(vtkIdType i, double val0, double val1, double val2);
  void SetTuple4(vtkIdType i, double val0, double val1, double val2,
                 double val3);
  void SetTuple9(vtkIdType i, double val0, double val1, double val2,
                 double val3, double val4, double val5, double val6,
                 double val7, double val8);

  // Description:
  // Insert the data tuple at ith location. Note that memory allocation
  // is performed as necessary to hold the data.
  virtual void InsertTuple(vtkIdType i, const float * tuple) = 0;
  virtual void InsertTuple(vtkIdType i, const double * tuple) = 0;

  // Description:
  // These methods are included as convenience for the wrappers.
  // InsertTuple() which takes arrays can not be 
  // used from wrapped languages. These methods can be used instead.
  void InsertTuple1(vtkIdType i, double value);
  void InsertTuple2(vtkIdType i, double val0, double val1);
  void InsertTuple3(vtkIdType i, double val0, double val1, double val2);
  void InsertTuple4(vtkIdType i, double val0, double val1, double val2,
                    double val3);
  void InsertTuple9(vtkIdType i, double val0, double val1, double val2,
                    double val3, double val4, double val5, double val6,
                    double val7, double val8);

  // Description:
  // Insert the data tuple at the end of the array and return the location at
  // which the data was inserted. Memory is allocated as necessary to hold
  // the data.
  virtual vtkIdType InsertNextTuple(const float * tuple) = 0;
  virtual vtkIdType InsertNextTuple(const double * tuple) = 0;

  // Description:
  // These methods are included as convenience for the wrappers.
  // InsertTuple() which takes arrays can not be 
  // used from wrapped languages. These methods can be used instead.
  void InsertNextTuple1(double value);
  void InsertNextTuple2(double val0, double val1);
  void InsertNextTuple3(double val0, double val1, double val2);
  void InsertNextTuple4(double val0, double val1, double val2,
                        double val3);
  void InsertNextTuple9(double val0, double val1, double val2,
                        double val3, double val4, double val5, double val6,
                        double val7, double val8);

  // Description:
  // These methods remove tuples from the data array. They shift data and
  // resize array, so the data array is still valid after this operation. Note,
  // this operation is fairly slow.
  virtual void RemoveTuple(vtkIdType id) = 0;
  virtual void RemoveFirstTuple() = 0;
  virtual void RemoveLastTuple() = 0;

  // Description:
  // Return the data component at the ith tuple and jth component location.
  // Note that i is less than NumberOfTuples and j is less than 
  // NumberOfComponents.
  virtual double GetComponent(vtkIdType i, int j);

  // Description:
  // Set the data component at the ith tuple and jth component location.
  // Note that i is less than NumberOfTuples and j is less than
  //  NumberOfComponents. Make sure enough memory has been allocated 
  // (use SetNumberOfTuples() and SetNumberOfComponents()).
  virtual void SetComponent(vtkIdType i, int j, double c);

  // Description:
  // Insert the data component at ith tuple and jth component location. 
  // Note that memory allocation is performed as necessary to hold the data.
  virtual void InsertComponent(vtkIdType i, int j, double c);

  // Description:
  // Get the data as a double array in the range (tupleMin,tupleMax) and
  // (compMin, compMax). The resulting double array consists of all data in
  // the tuple range specified and only the component range specified. This
  // process typically requires casting the data from native form into
  // doubleing point values. This method is provided as a convenience for data
  // exchange, and is not very fast.
  virtual void GetData(vtkIdType tupleMin, vtkIdType tupleMax, int compMin,
                       int compMax, vtkDoubleArray* data);

  // Description:
  // Deep copy of data. Copies data from different data arrays even if
  // they are different types (using doubleing-point exchange).
  virtual void DeepCopy(vtkDataArray *da);

  // Description:
  // Fill a component of a data array with a specified value. This method
  // sets the specified component to specified value for all tuples in the
  // data array.  This methods can be used to initialize or reinitialize a
  // single component of a multi-component array.
  virtual void FillComponent(int j, double c);

  // Description:
  // Copy a component from one data array into a component on this data array.
  // This method copies the specified component ("fromComponent") from the
  // specified data array ("from") to the specified component ("j") over all
  // the tuples in this data array.  This method can be used to extract
  // a component (column) from one data array and paste that data into
  // a component on this data array.
  virtual void CopyComponent(int j, vtkDataArray *from,
                             int fromComponent);

  // Description:
  // Get the address of a particular data index. Make sure data is allocated
  // for the number of items requested. Set MaxId according to the number of
  // data values requested.
  virtual void* WriteVoidPointer(vtkIdType id, vtkIdType number) = 0;

  // Description:
  // Return a void pointer. For image pipeline interface and other 
  // special pointer manipulation.
  virtual void *GetVoidPointer(vtkIdType id) = 0;

  // Description:
  // Free any unnecessary memory.
  virtual void Squeeze() = 0;

  // Description:
  // Resize the array while conserving the data.
  virtual void Resize(vtkIdType numTuples) = 0;

  // Description:
  // Reset to an empty state, without freeing any memory.
  void Reset() 
    {this->MaxId = -1;}

  // Description:
  // Return the size of the data.
  vtkIdType GetSize() 
    {return this->Size;}
  
  // Description:
  // What is the maximum id currently in the array.
  vtkIdType GetMaxId() 
    {return this->MaxId;}

  // Description:
  // This method lets the user specify data to be held by the array.  The 
  // array argument is a pointer to the data.  size is the size of 
  // the array supplied by the user.  Set save to 1 to keep the class
  // from deleting the array when it cleans up or reallocates memory.
  // The class uses the actual array provided; it does not copy the data 
  // from the supplied array.
  virtual void SetVoidArray(void *vtkNotUsed(array),
                            vtkIdType vtkNotUsed(size),
                            int vtkNotUsed(save)) {};

  // Description:
  // This method copies the array data to the void pointer specified
  // by the user.  It is up to the user to allocate enough memory for
  // the void pointer.
  virtual void ExportToVoidPointer(void *vtkNotUsed(out_ptr)) {}

  // Description:
  // Return the memory in kilobytes consumed by this data array. Used to
  // support streaming and reading/writing data. The value returned is
  // guaranteed to be greater than or equal to the memory required to
  // actually represent the data represented by this object. The 
  // information returned is valid only after the pipeline has 
  // been updated.
  unsigned long GetActualMemorySize();
  
  // Description:
  // Create default lookup table. Generally used to create one when none
  // is available.
  void CreateDefaultLookupTable();

  // Description:
  // Set/get the lookup table associated with this scalar data, if any.
  void SetLookupTable(vtkLookupTable *lut);
  vtkGetObjectMacro(LookupTable,vtkLookupTable);
  
  // Description:
  // Set/get array's name
  vtkSetStringMacro(Name);
  vtkGetStringMacro(Name);

  // Description:
  // Return the range of the array values for the given component. 
  // Range is copied into the array provided.
  // If comp is equal to -1, it returns the range of the magnitude
  // (if the number of components is equal to 1 it still returns the range of
  // component 0).
  void GetRange(double range[2], int comp)
    {
    this->ComputeRange(comp);
    memcpy(range, this->Range, 2*sizeof(double));
    }
  double* GetRange(int comp)
    {
    this->ComputeRange(comp);
    return this->Range;
    }

  virtual void ComputeRange(int comp);
  // Description:
  // Return the range of the array values for the 0th component. 
  // Range is copied into the array provided.
  double* GetRange()
    {
    this->ComputeRange(0);
    return this->Range;
    }
  void GetRange(double range[2])
    {
    this->GetRange(range,0);
    }

  // Description:
  // These methods return the Min and Max possible range of the native
  // data type. For example if a vtkScalars consists of unsigned char
  // data these will return (0,255). 
  void GetDataTypeRange(double range[2]);
  double GetDataTypeMin();
  double GetDataTypeMax();
  static void GetDataTypeRange(int type, double range[2]);
  static double GetDataTypeMin(int type);
  static double GetDataTypeMax(int type);

  // Description:
  // Return the maximum norm for the tuples.
  // Note that the max. is computed everytime GetMaxNorm is called.
  virtual double GetMaxNorm();

  // Description:
  // Creates an array for dataType where dataType is one of
  // VTK_BIT, VTK_CHAR, VTK_SIGNED_CHAR, VTK_UNSIGNED_CHAR, VTK_SHORT,
  // VTK_UNSIGNED_SHORT, VTK_INT, VTK_UNSIGNED_INT, VTK_LONG,
  // VTK_UNSIGNED_LONG, VTK_DOUBLE, VTK_DOUBLE, VTK_ID_TYPE.
  // Note that the data array returned has be deleted by the
  // user.
  static vtkDataArray* CreateDataArray(int dataType);

protected:
  // Construct object with default tuple dimension (number of components) of 1.
  vtkDataArray(vtkIdType numComp=1);
  ~vtkDataArray();

  vtkLookupTable *LookupTable;

  vtkIdType Size;      // allocated size of data
  vtkIdType MaxId;     // maximum index inserted thus far
  int NumberOfComponents; // the number of components per tuple

  char* Name;

  double Range[2];

  // We can have arbitrary number of components, but 11 should
  // take care of 99.99% of the cases.  Components greater
  // than 11 do not get cached.  The comment below assume max of 4 comps.  
  // 5 components since you can compute the range of components
  // less than 0 to get a magnitude range. ComponentRange[4] is 
  // this magnitude range
  vtkTimeStamp 
     ComponentRangeComputeTime[VTK_MAXIMUM_NUMBER_OF_CACHED_COMPONENT_RANGES];
  double ComponentRange[VTK_MAXIMUM_NUMBER_OF_CACHED_COMPONENT_RANGES][2];
  
private:
  double* GetTupleN(vtkIdType i, int n);
  
private:
  vtkDataArray(const vtkDataArray&);  // Not implemented.
  void operator=(const vtkDataArray&);  // Not implemented.
};

#endif
