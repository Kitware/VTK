/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnicodeStringArray.h

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

// .NAME vtkUnicodeStringArray - Subclass of vtkAbstractArray that holds vtkUnicodeStrings
//
// .SECTION Description
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

#ifndef vtkUnicodeStringArray_h
#define vtkUnicodeStringArray_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkAbstractArray.h"
#include "vtkUnicodeString.h" // For value type

class VTKCOMMONCORE_EXPORT vtkUnicodeStringArray :
  public vtkAbstractArray
{
public:
  static vtkUnicodeStringArray* New();
  vtkTypeMacro(vtkUnicodeStringArray,vtkAbstractArray);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual int Allocate(vtkIdType sz, vtkIdType ext=1000);
  virtual void Initialize();
  virtual int GetDataType();
  virtual int GetDataTypeSize();
  virtual int GetElementComponentSize();
  virtual void SetNumberOfTuples(vtkIdType number);
  virtual void SetTuple(vtkIdType i, vtkIdType j, vtkAbstractArray* source);
  virtual void InsertTuple(vtkIdType i, vtkIdType j, vtkAbstractArray* source);
  virtual void InsertTuples(vtkIdList *dstIds, vtkIdList *srcIds,
                            vtkAbstractArray *source);
  virtual void InsertTuples(vtkIdType dstStart, vtkIdType n, vtkIdType srcStart,
                            vtkAbstractArray* source);
  virtual vtkIdType InsertNextTuple(vtkIdType j, vtkAbstractArray* source);
  virtual void* GetVoidPointer(vtkIdType id);
  virtual void DeepCopy(vtkAbstractArray* da);
  virtual void InterpolateTuple(vtkIdType i, vtkIdList *ptIndices,
    vtkAbstractArray* source,  double* weights);
  virtual void InterpolateTuple(vtkIdType i,
    vtkIdType id1, vtkAbstractArray* source1,
    vtkIdType id2, vtkAbstractArray* source2, double t);
  virtual void Squeeze();
  virtual int Resize(vtkIdType numTuples);
  virtual void SetVoidArray(void *array, vtkIdType size, int save);
  virtual unsigned long GetActualMemorySize();
  virtual int IsNumeric();
  virtual vtkArrayIterator* NewIterator();
  virtual vtkVariant GetVariantValue(vtkIdType idx);
  virtual vtkIdType LookupValue(vtkVariant value);
  virtual void LookupValue(vtkVariant value, vtkIdList* ids);

  virtual void SetVariantValue(vtkIdType idx, vtkVariant value);
  virtual void DataChanged();
  virtual void ClearLookup();

  vtkIdType InsertNextValue(const vtkUnicodeString&);
  void InsertValue(vtkIdType idx, const vtkUnicodeString&); // Ranged checked
  void SetValue(vtkIdType i, const vtkUnicodeString&); // Not ranged checked
  vtkUnicodeString& GetValue(vtkIdType i);

  void InsertNextUTF8Value(const char*);
  void SetUTF8Value(vtkIdType i, const char*);
  const char* GetUTF8Value(vtkIdType i);

protected:
  vtkUnicodeStringArray();
  ~vtkUnicodeStringArray();

private:
  vtkUnicodeStringArray(const vtkUnicodeStringArray&);  // Not implemented.
  void operator=(const vtkUnicodeStringArray&);  // Not implemented.

//BTX
  class Implementation;
  Implementation* Internal;
//ETX
};

#endif

