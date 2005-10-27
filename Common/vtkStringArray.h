/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStringArray.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright 2004 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

=========================================================================*/

// .NAME vtkStringArray - Subclass of vtkAbstractArray that holds vtkStdStrings
//
// .SECTION Description
// Points and cells may sometimes have associated data that are stored
// as strings, e.g. many information visualization projects.  This
// class provides a reasonably clean way to store and access those.
//
//
// .SECTION Caveats
// 
// Wrapping support for the Set/Get/Insert methods is a little
// strange.  The Tcl/Python/Java wrappers treat vtkStdString as const
// char * right now instead of dealing with native strings.  This is
// why there are two versions of every Set method: one (which you
// should use in C++ code) taking a vtkStdString argument and one
// (used by the wrapper) taking a const char * argument.
//
// If you'd like to look into modifying the wrappers to handle this
// properly, go right ahead...
//
// .SECTION Thanks
// Andy Wilson (atwilso@sandia.gov) wrote this class.

#ifndef __vtkStringArray_h
#define __vtkStringArray_h

#include "vtkAbstractArray.h"
#include "vtkStdString.h" // needed for vtkStdString definition

class VTK_COMMON_EXPORT vtkStringArray : public vtkAbstractArray
{
public:
  static vtkStringArray* New();
  vtkTypeRevisionMacro(vtkStringArray,vtkAbstractArray);
  void PrintSelf(ostream& os, vtkIndent indent);

  //
  // 
  // Functions required by vtkAbstractArray
  //
  //

  // Description:
  // Get the data type.
  int GetDataType()
    { return VTK_STRING; }

  bool IsNumeric() { return false; } 

  // Description:
  // Release storage and reset array to initial state.
  void Initialize();

  // Description:
  // Return the size of the data type.  WARNING: This may not mean
  // what you expect with strings.  It will return
  // sizeof(vtkstd::string) and not take into account the data
  // included in any particular string.
  int GetDataTypeSize();

  // Description:
  // Free any unnecessary memory.
  // Resize object to just fit data requirement. Reclaims extra memory.
  void Squeeze() { this->ResizeAndExtend (this->MaxId+1); }

  // Description:
  // Resize the array while conserving the data.
  int Resize(vtkIdType numTuples);

  // Description:
  // Given a list of indices, return an array of values.  You must
  // insure that the output array has been previously allocated with
  // enough space to hold the data and that the types match
  // sufficiently to allow conversion (if necessary).
  void GetValues(vtkIdList *ptIds, vtkAbstractArray *output);

  // Description:
  // Get the values for the range of indices specified (i.e.,
  // p1->p2 inclusive). You must insure that the output array has been
  // previously allocated with enough space to hold the data and that
  // the type of the output array is compatible with the type of this
  // array.
  void GetValues(vtkIdType p1, vtkIdType p2, vtkAbstractArray *output);

  // Description:
  // Copy a value from a given source array into this array.
  void CopyValue(int toIndex, int fromIndex, vtkAbstractArray *sourceArray);

  // Description:
  // Allocate memory for this array. Delete old storage only if necessary.
  // Note that ext is no longer used.
  int Allocate( vtkIdType sz, vtkIdType ext=1000 );

  // Description:
  // Get the data at a particular index.
  vtkStdString &GetValue(vtkIdType id);

  // Description:
  // Set the data at a particular index. Does not do range checking. Make sure
  // you use the method SetNumberOfValues() before inserting data.
//BTX
  void SetValue(vtkIdType id, vtkStdString value)
    { this->Array[id] = value; }
//ETX
  void SetValue(vtkIdType id, const char *value);

  // Description:
  // Specify the number of values for this object to hold. Does an
  // allocation as well as setting the MaxId ivar. Used in conjunction with
  // SetValue() method for fast insertion.
  void SetNumberOfValues(vtkIdType number);

  int GetNumberOfValues() { return this->MaxId + 1; }

  int GetNumberOfElementComponents() { return 0; }
  int GetElementComponentSize() { return static_cast<int>(sizeof(vtkStdString::value_type)); }

  // Description:
  // Insert data at a specified position in the array.
//BTX
  void InsertValue(vtkIdType id, vtkStdString f);
//ETX
  void InsertValue(vtkIdType id, const char *val);

  // Description:
  // Insert data at the end of the array. Return its location in the array.
//BTX
  vtkIdType InsertNextValue(vtkStdString f);
//ETX
  vtkIdType InsertNextValue(const char *f);

//BTX
  // Description:
  // Get the address of a particular data index. Make sure data is allocated
  // for the number of items requested. Set MaxId according to the number of
  // data values requested.
  vtkStdString* WritePointer(vtkIdType id, vtkIdType number);
//ETX

//BTX
  // Description:
  // Get the address of a particular data index. Performs no checks
  // to verify that the memory has been allocated etc.
  vtkStdString* GetPointer(vtkIdType id) { return this->Array + id; }
  void* GetVoidPointer(vtkIdType id) { return this->GetPointer(id); }
//BTX

  // Description:
  // Deep copy of another string array.  Will complain and change nothing
  // if the array passed in is not a vtkStringArray.
  void DeepCopy( vtkAbstractArray* aa );


//BTX
  // Description:
  // This method lets the user specify data to be held by the array.  The
  // array argument is a pointer to the data.  size is the size of
  // the array supplied by the user.  Set save to 1 to keep the class
  // from deleting the array when it cleans up or reallocates memory.
  // The class uses the actual array provided; it does not copy the data
  // from the suppled array.
  void SetArray(vtkStdString* array, vtkIdType size, int save);
  void SetVoidArray(void* array, vtkIdType size, int save)
    { this->SetArray(static_cast<vtkStdString*>(array), size, save); }
//ETX

  // Description:
  // Return the memory in kilobytes consumed by this data array. Used to
  // support streaming and reading/writing data. The value returned is
  // guaranteed to be greater than or equal to the memory required to
  // actually represent the data represented by this object. The 
  // information returned is valid only after the pipeline has 
  // been updated.
  //
  // This function takes into account the size of the contents of the
  // strings as well as the string containers themselves.
  unsigned long GetActualMemorySize();

  void ConvertToContiguous(vtkDataArray **Data, vtkIdTypeArray **Offsets);
  void ConvertFromContiguous(vtkDataArray *Data, vtkIdTypeArray *Offsets);

protected:
  vtkStringArray(vtkIdType numComp=1);
  ~vtkStringArray();

  vtkStdString* Array;   // pointer to data
  vtkStdString* ResizeAndExtend(vtkIdType sz);  // function to resize data

  int SaveUserArray;

private:
  vtkStringArray(const vtkStringArray&);  // Not implemented.
  void operator=(const vtkStringArray&);  // Not implemented.
};



#endif
