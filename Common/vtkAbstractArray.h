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

class vtkIdList;
class vtkIdTypeArray;
class vtkDataArray;

#define VTK_MAXIMUM_NUMBER_OF_CACHED_COMPONENT_RANGES 11

class VTK_COMMON_EXPORT vtkAbstractArray : public vtkObject 
{
public:
  vtkTypeRevisionMacro(vtkAbstractArray,vtkObject);
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
  // Return the size of the underlying data type.  For a bit, 0 is
  // returned.  XXX FIXME How will this method behave for
  // variably-sized objects?
  virtual int GetDataTypeSize() = 0;
  static unsigned long GetDataTypeSize(int type);

  // Description:
  // Given a list of indices, return an array of values.  The caller
  // must ensure that enough room has been allocated within the output
  // array to hold the data and that the data types match well enough
  // to allow any necessary conversions.
  virtual void GetValues(vtkIdList *indices, vtkAbstractArray *output) = 0;

  // Description:
  // Get the values for the range of indices specified (i.e., p1->p2
  // inclusive). You must insure that the output array has been
  // previously allocated with enough space to hold the data and that
  // the type of the output array is compatible with the type of this
  // array.
  virtual void GetValues(vtkIdType p1, vtkIdType p2, vtkAbstractArray *output) = 0;

  // Description:
  // Return the number of components in a single element of the array.
  // For vtkDataArray and its subclasses, this is the number of
  // components in a tuple.  Arrays with variable-length elements
  // (such as vtkStringArray and vtkCellArray) should return 0.
  virtual int GetNumberOfElementComponents() = 0;

  // Description:
  // Return the size, in bytes, of the lowest-level element of an
  // array.  For vtkDataArray and subclasses this is the size of the
  // data type.  For vtkStringArray, this is
  // sizeof(vtkStdString::value_type), which winds up being
  // sizeof(char).  
  virtual int GetElementComponentSize() = 0;

  // Description:
  // Return a void pointer. For image pipeline interface and other 
  // special pointer manipulation.
  virtual void *GetVoidPointer(vtkIdType id) = 0;

  // Description:
  // Deep copy of data. Implementation left to subclasses, which
  // should support as many type conversions as possible given the
  // data type.
  virtual void DeepCopy(vtkAbstractArray *da) = 0;

  // Description:
  // Copy an element from one array into an element on this array.  
  virtual void CopyValue(int toIndex, int fromIndex,
                         vtkAbstractArray *sourceArray) = 0;

  // Description:
  // Free any unnecessary memory.
  // Description:
  // Resize object to just fit data requirement. Reclaims extra memory.
  virtual void Squeeze() = 0; 

  // Description:
  // Resize the array while conserving the data.
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
                            int vtkNotUsed(save)) {};

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
  void SetName(const char* name);
  const char* GetName();

  // Description:
  // Get the name of a data type as a string.  
  // XXX FIXME Find this macro and move it into vtkTypeNames.h or
  // something.
  virtual const char *GetDataTypeAsString( void )
  { return vtkImageScalarTypeNameMacro( this->GetDataType() ); }


  // This function will only make sense once vtkDataArray is
  // re-parented to be a subclass of vtkAbstractArray.  It is
  // commented out for now.

  // Description:
  // Creates an array for dataType where dataType is one of
  // VTK_BIT, VTK_CHAR, VTK_UNSIGNED_CHAR, VTK_SHORT,
  // VTK_UNSIGNED_SHORT, VTK_INT, VTK_UNSIGNED_INT, VTK_LONG,
  // VTK_UNSIGNED_LONG, VTK_DOUBLE, VTK_DOUBLE, VTK_ID_TYPE,
  // VTK_STRING, VTK_CELL.
  // Note that the data array returned has be deleted by the
  // user.

//  static vtkAbstractArray* CreateArray(int dataType);

  // Description:
  // This method is here to make backward compatibility easier.  It
  // must return true if and only if an array contains numeric data.
  virtual bool IsNumeric() = 0;

  // Description:
  // Flatten an arbitrary array into two separate numeric arrays.  The
  // first contains all the data in the source array; the second, the
  // index of the end of each element.  This function is meant to
  // assist reading and writing arrays with variable-length elements
  // such as vtkStringArray and vtkCellArray.  The arrays will be
  // created within the method body: the caller is responsible for
  // deleting them when no longer needed.
  //
  // <p>
  //
  // For example, a string array with the elements "This" "Is" "A"
  // "Test" would be converted into the following two arrays:
  //
  // Data: (unsigned char array) ThisIsATest
  // Offsets: 3 5 6 10
  //
  // vtkDataArray also implements this method, although in practice
  // you shouldn't need it.  It ignores the offsets array and takes
  // its tuple-size information from the data array.
  virtual void ConvertToContiguous(vtkDataArray **Data, vtkIdTypeArray **Offsets) = 0;

  // Description:
  // This is the inverse of ConvertToContiguous(), above.  
  virtual void ConvertFromContiguous(vtkDataArray *Data, vtkIdTypeArray *Offsets) = 0;

protected:
  // Construct object with default tuple dimension (number of components) of 1.
  vtkAbstractArray(vtkIdType numComp=1);
  ~vtkAbstractArray();

  vtkIdType Size;      // allocated size of data
  vtkIdType MaxId;     // maximum index inserted thus far

  char* Name;

  int DataType; // uses constants in vtkSystemIncludes.h
                                          
  
private:
  vtkAbstractArray(const vtkAbstractArray&);  // Not implemented.
  void operator=(const vtkAbstractArray&);  // Not implemented.
};

#endif
