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
// .NAME vtkAbstractArray - Abstract superclass for all arrays
//
// .SECTION Description
//
// vtkAbstractArray is an abstract superclass for data array objects.
// This class defines an API that all subclasses must support.  The
// data type must be assignable and copy-constructible, but no other
// assumptions about its type are made.  Most of the subclasses of
// this array deal with numeric data either as scalars or tuples of
// scalars.  A program can use the IsNumeric() method to check whether
// an instance of vtkAbstractArray contains numbers.  It is also
// possible to test for this by attempting to SafeDownCast an array to
// an instance of vtkDataArray, although this assumes that all numeric
// arrays will always be descended from vtkDataArray.
//
// <p>
//
// Every array has a character-string name. The naming of the array
// occurs automatically when it is instantiated, but you are free to
// change this name using the SetName() method.  (The array name is
// used for data manipulation.)
//
// .SECTION See Also
// vtkDataArray vtkStringArray vtkCellArray

#ifndef __vtkAbstractArray_h
#define __vtkAbstractArray_h

#include "vtkObject.h"
#include "vtkVariant.h" // for variant arguments

class vtkArrayIterator;
class vtkDataArray;
class vtkIdList;
class vtkIdTypeArray;
class vtkInformation;

class VTK_COMMON_EXPORT vtkAbstractArray : public vtkObject 
{
public:
  vtkTypeMacro(vtkAbstractArray,vtkObject);
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
  virtual int GetDataType() =0;

  // Description:
  // Return the size of the underlying data type.  For a bit, 0 is
  // returned.  For string 0 is returned. Arrays with variable length
  // components return 0.
  virtual int GetDataTypeSize() = 0;
  static int GetDataTypeSize(int type);

  // Description:
  // Return the size, in bytes, of the lowest-level element of an
  // array.  For vtkDataArray and subclasses this is the size of the
  // data type.  For vtkStringArray, this is
  // sizeof(vtkStdString::value_type), which winds up being
  // sizeof(char).  
  virtual int GetElementComponentSize() = 0;

  // Description:
  // Set/Get the dimention (n) of the components. Must be >= 1. Make sure that
  // this is set before allocation.
  vtkSetClampMacro(NumberOfComponents, int, 1, VTK_LARGE_INTEGER);
  int GetNumberOfComponents() { return this->NumberOfComponents; }

  // Description:
  // Set the number of tuples (a component group) in the array. Note that 
  // this may allocate space depending on the number of components.
  // Also note that if allocation is performed no copy is performed so
  // existing data will be lost (if data conservation is sought, one may
  // use the Resize method instead).
  virtual void SetNumberOfTuples(vtkIdType number) = 0;

  // Description:
  // Get the number of tuples (a component group) in the array.
  vtkIdType GetNumberOfTuples() 
    {return (this->MaxId + 1)/this->NumberOfComponents;}

  // Description:
  // Set the tuple at the ith location using the jth tuple in the source array.
  // This method assumes that the two arrays have the same type
  // and structure. Note that range checking and memory allocation is not 
  // performed; use in conjunction with SetNumberOfTuples() to allocate space.
  virtual void SetTuple(vtkIdType i, vtkIdType j, vtkAbstractArray* source) = 0;

  // Description:
  // Insert the jth tuple in the source array, at ith location in this array. 
  // Note that memory allocation is performed as necessary to hold the data.
  virtual void InsertTuple(vtkIdType i, vtkIdType j, vtkAbstractArray* source) = 0;

  // Description:
  // Insert the jth tuple in the source array, at the end in this array. 
  // Note that memory allocation is performed as necessary to hold the data.
  // Returns the location at which the data was inserted.
  virtual vtkIdType InsertNextTuple(vtkIdType j, vtkAbstractArray* source) = 0;

  // Description:
  // Given a list of point ids, return an array of tuples.
  // You must insure that the output array has been previously 
  // allocated with enough space to hold the data.
  virtual void GetTuples(vtkIdList *ptIds, vtkAbstractArray* output);

  // Description:
  // Get the tuples for the range of points ids specified 
  // (i.e., p1->p2 inclusive). You must insure that the output array has 
  // been previously allocated with enough space to hold the data.
  virtual void GetTuples(vtkIdType p1, vtkIdType p2, vtkAbstractArray *output);

  // Description:
  // Return a void pointer. For image pipeline interface and other 
  // special pointer manipulation.
  virtual void *GetVoidPointer(vtkIdType id) = 0;

  // Description:
  // Deep copy of data. Implementation left to subclasses, which
  // should support as many type conversions as possible given the
  // data type.
  //
  // Subclasses should call vtkAbstractArray::DeepCopy() so that the
  // information object (if one exists) is copied from \a da.
  virtual void DeepCopy(vtkAbstractArray* da);

  // Description:
  // Set the ith tuple in this array as the interpolated tuple value,
  // given the ptIndices in the source array and associated 
  // interpolation weights.
  // This method assumes that the two arrays are of the same type
  // and strcuture.
  virtual void InterpolateTuple(vtkIdType i, vtkIdList *ptIndices,
    vtkAbstractArray* source,  double* weights) = 0;

  // Description
  // Insert the ith tuple in this array as interpolated from the two values, 
  // p1 and p2, and an interpolation factor, t. 
  // The interpolation factor ranges from (0,1), 
  // with t=0 located at p1. This method assumes that the three arrays are of 
  // the same type. p1 is value at index id1 in source1, while, p2 is
  // value at index id2 in source2.
  virtual void InterpolateTuple(vtkIdType i, 
    vtkIdType id1, vtkAbstractArray* source1, 
    vtkIdType id2, vtkAbstractArray* source2, double t) =0;

  // Description:
  // Free any unnecessary memory.
  // Description:
  // Resize object to just fit data requirement. Reclaims extra memory.
  virtual void Squeeze() = 0; 

  // Description:
  // Resize the array while conserving the data.  Returns 1 if
  // resizing succeeded and 0 otherwise.
  virtual int Resize(vtkIdType numTuples) = 0;

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
                            int vtkNotUsed(save)) =0;

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
  virtual unsigned long GetActualMemorySize() = 0;

  // Description:
  // Set/get array's name
  vtkSetStringMacro(Name); 
  vtkGetStringMacro(Name);

  // Description:
  // Get the name of a data type as a string.  
  virtual const char *GetDataTypeAsString( void )
    { return vtkImageScalarTypeNameMacro( this->GetDataType() ); }

  // Description:
  // Creates an array for dataType where dataType is one of
  // VTK_BIT, VTK_CHAR, VTK_UNSIGNED_CHAR, VTK_SHORT,
  // VTK_UNSIGNED_SHORT, VTK_INT, VTK_UNSIGNED_INT, VTK_LONG,
  // VTK_UNSIGNED_LONG, VTK_DOUBLE, VTK_DOUBLE, VTK_ID_TYPE,
  // VTK_STRING.
  // Note that the data array returned has be deleted by the
  // user.
  static vtkAbstractArray* CreateArray(int dataType);

  // Description:
  // This method is here to make backward compatibility easier.  It
  // must return true if and only if an array contains numeric data.
  virtual int IsNumeric() = 0;

  // Description:
  // Subclasses must override this method and provide the right 
  // kind of templated vtkArrayIteratorTemplate.
  virtual vtkArrayIterator* NewIterator() = 0;

  // Description:
  // Returns the size of the data in DataTypeSize units. Thus, the number of bytes
  // for the data can be computed by GetDataSize() * GetDataTypeSize(). Non-contiguous 
  // or variable- size arrays  need to override this method.
  virtual vtkIdType GetDataSize()
    {
    return this->GetNumberOfComponents() * this->GetNumberOfTuples();
    }

  //BTX
  // Description:
  // Return the indices where a specific value appears.
  virtual vtkIdType LookupValue(vtkVariant value) = 0;
  virtual void LookupValue(vtkVariant value, vtkIdList* ids) = 0;

  // Description:
  // Retrieve value from the array as a variant.
  virtual vtkVariant GetVariantValue(vtkIdType idx);

  // Description:
  // Insert a value into the array from a variant.
  virtual void InsertVariantValue(vtkIdType idx, vtkVariant value) = 0;
  //ETX

  // Description:
  // Tell the array explicitly that the data has changed.
  // This is only necessary to call when you modify the array contents
  // without using the array's API (i.e. you retrieve a pointer to the
  // data and modify the array contents).  You need to call this so that
  // the fast lookup will know to rebuild itself.  Otherwise, the lookup
  // functions will give incorrect results.
  virtual void DataChanged() = 0;

  // Description:
  // Delete the associated fast lookup data structure on this array,
  // if it exists.  The lookup will be rebuilt on the next call to a lookup
  // function.
  virtual void ClearLookup() = 0;

  // TODO: Implement these lookup functions also.
  //virtual void LookupRange(vtkVariant min, vtkVariant max, vtkIdList* ids,
  //  bool includeMin = true, bool includeMax = true) = 0;
  //virtual void LookupGreaterThan(vtkVariant min, vtkIdList* ids, bool includeMin = false) = 0;
  //virtual void LookupLessThan(vtkVariant max, vtkIdList* ids, bool includeMax = false) = 0;

  // Description:
  // Get an information object that can be used to annotate the array.
  // This will always return an instance of vtkInformation, if one is
  // not currently associated with the array it will be created.
  vtkInformation* GetInformation();
  // Description:
  // Inquire if this array has an instance of vtkInformation 
  // already associated with it.
  bool HasInformation(){ return this->Information!=0; }
  //BTX
  // Description:
  // Copy information instance. Arrays use information objects
  // in a variety of ways. It is important to have flexibility in
  // this regard because certain keys should not be coppied, while
  // others must be.
  // 
  // NOTE: Subclasses must always call their superclass's CopyInformation
  // method, so that all classes in the hierarchy get a chance to remove
  // keys they do not wish to be coppied. The subclass will not need to
  // explicilty copy the keys as it's handled here.
  virtual int CopyInformation(vtkInformation *infoFrom, int deep=1);
  //ETX

protected:
  // Description:
  // Set an information object that can be used to annotate the array.
  // Use this with caution as array instances depend on persistance of
  // information keys. See CopyInformation.
  virtual void SetInformation( vtkInformation* );

  // Construct object with default tuple dimension (number of components) of 1.
  vtkAbstractArray(vtkIdType numComp=1);
  ~vtkAbstractArray();

  vtkIdType Size;         // allocated size of data
  vtkIdType MaxId;        // maximum index inserted thus far
  int NumberOfComponents; // the number of components per tuple

  char* Name;

  bool RebuildArray;      // whether to rebuild the fast lookup data structure.

  vtkInformation* Information;

private:
  vtkAbstractArray(const vtkAbstractArray&);  // Not implemented.
  void operator=(const vtkAbstractArray&);  // Not implemented.
};

#endif
