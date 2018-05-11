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

/**
 * @class   vtkUnicodeStringArray
 * @brief   Subclass of vtkAbstractArray that holds vtkUnicodeStrings
 *
 *
 *
 * @par Thanks:
 * Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.
*/

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
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkTypeBool Allocate(vtkIdType sz, vtkIdType ext=1000) override;
  void Initialize() override;
  int GetDataType() override;
  int GetDataTypeSize() override;
  int GetElementComponentSize() override;
  void SetNumberOfTuples(vtkIdType number) override;
  void SetTuple(vtkIdType i, vtkIdType j,
                vtkAbstractArray* source) override;
  void InsertTuple(vtkIdType i, vtkIdType j,
                   vtkAbstractArray* source) override;
  void InsertTuples(vtkIdList *dstIds, vtkIdList *srcIds,
                    vtkAbstractArray *source) override;
  void InsertTuples(vtkIdType dstStart, vtkIdType n, vtkIdType srcStart,
                    vtkAbstractArray* source) override;
  vtkIdType InsertNextTuple(vtkIdType j, vtkAbstractArray* source) override;
  void* GetVoidPointer(vtkIdType id) override;
  void DeepCopy(vtkAbstractArray* da) override;
  void InterpolateTuple(vtkIdType i, vtkIdList *ptIndices,
    vtkAbstractArray* source,  double* weights) override;
  void InterpolateTuple(vtkIdType i,
    vtkIdType id1, vtkAbstractArray* source1,
    vtkIdType id2, vtkAbstractArray* source2, double t) override;
  void Squeeze() override;
  vtkTypeBool Resize(vtkIdType numTuples) override;
  void SetVoidArray(void *array, vtkIdType size, int save) override;
  void SetVoidArray(void *array, vtkIdType size, int save,
                    int deleteMethod) override;
  void SetArrayFreeFunction(void (*callback)(void *)) override;
  unsigned long GetActualMemorySize() override; // in bytes
  int IsNumeric() override;
  VTK_NEWINSTANCE vtkArrayIterator* NewIterator() override;
  vtkVariant GetVariantValue(vtkIdType idx) override;
  vtkIdType LookupValue(vtkVariant value) override;
  void LookupValue(vtkVariant value, vtkIdList* ids) override;

  void SetVariantValue(vtkIdType idx, vtkVariant value) override;
  void InsertVariantValue(vtkIdType idx, vtkVariant value) override;
  void DataChanged() override;
  void ClearLookup() override;

  vtkIdType InsertNextValue(const vtkUnicodeString&);
  void InsertValue(vtkIdType idx, const vtkUnicodeString&); // Ranged checked
  void SetValue(vtkIdType i, const vtkUnicodeString&); // Not ranged checked
  vtkUnicodeString& GetValue(vtkIdType i);

  void InsertNextUTF8Value(const char*);
  void SetUTF8Value(vtkIdType i, const char*);
  const char* GetUTF8Value(vtkIdType i);

protected:
  vtkUnicodeStringArray();
  ~vtkUnicodeStringArray() override;

private:
  vtkUnicodeStringArray(const vtkUnicodeStringArray&) = delete;
  void operator=(const vtkUnicodeStringArray&) = delete;

  class Implementation;
  Implementation* Internal;

};

#endif
