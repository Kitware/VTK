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
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  int Allocate(vtkIdType sz, vtkIdType ext=1000) VTK_OVERRIDE;
  void Initialize() VTK_OVERRIDE;
  int GetDataType() VTK_OVERRIDE;
  int GetDataTypeSize() VTK_OVERRIDE;
  int GetElementComponentSize() VTK_OVERRIDE;
  void SetNumberOfTuples(vtkIdType number) VTK_OVERRIDE;
  void SetTuple(vtkIdType i, vtkIdType j,
                vtkAbstractArray* source) VTK_OVERRIDE;
  void InsertTuple(vtkIdType i, vtkIdType j,
                   vtkAbstractArray* source) VTK_OVERRIDE;
  void InsertTuples(vtkIdList *dstIds, vtkIdList *srcIds,
                    vtkAbstractArray *source) VTK_OVERRIDE;
  void InsertTuples(vtkIdType dstStart, vtkIdType n, vtkIdType srcStart,
                    vtkAbstractArray* source) VTK_OVERRIDE;
  vtkIdType InsertNextTuple(vtkIdType j, vtkAbstractArray* source) VTK_OVERRIDE;
  void* GetVoidPointer(vtkIdType id) VTK_OVERRIDE;
  void DeepCopy(vtkAbstractArray* da) VTK_OVERRIDE;
  void InterpolateTuple(vtkIdType i, vtkIdList *ptIndices,
    vtkAbstractArray* source,  double* weights) VTK_OVERRIDE;
  void InterpolateTuple(vtkIdType i,
    vtkIdType id1, vtkAbstractArray* source1,
    vtkIdType id2, vtkAbstractArray* source2, double t) VTK_OVERRIDE;
  void Squeeze() VTK_OVERRIDE;
  int Resize(vtkIdType numTuples) VTK_OVERRIDE;
  void SetVoidArray(void *array, vtkIdType size, int save) VTK_OVERRIDE;
  void SetVoidArray(void *array, vtkIdType size, int save,
                    int deleteMethod) VTK_OVERRIDE;
  unsigned long GetActualMemorySize() VTK_OVERRIDE; // in bytes
  int IsNumeric() VTK_OVERRIDE;
  VTK_NEWINSTANCE vtkArrayIterator* NewIterator() VTK_OVERRIDE;
  vtkVariant GetVariantValue(vtkIdType idx) VTK_OVERRIDE;
  vtkIdType LookupValue(vtkVariant value) VTK_OVERRIDE;
  void LookupValue(vtkVariant value, vtkIdList* ids) VTK_OVERRIDE;

  void SetVariantValue(vtkIdType idx, vtkVariant value) VTK_OVERRIDE;
  void InsertVariantValue(vtkIdType idx, vtkVariant value) VTK_OVERRIDE;
  void DataChanged() VTK_OVERRIDE;
  void ClearLookup() VTK_OVERRIDE;

  vtkIdType InsertNextValue(const vtkUnicodeString&);
  void InsertValue(vtkIdType idx, const vtkUnicodeString&); // Ranged checked
  void SetValue(vtkIdType i, const vtkUnicodeString&); // Not ranged checked
  vtkUnicodeString& GetValue(vtkIdType i);

  void InsertNextUTF8Value(const char*);
  void SetUTF8Value(vtkIdType i, const char*);
  const char* GetUTF8Value(vtkIdType i);

protected:
  vtkUnicodeStringArray();
  ~vtkUnicodeStringArray() VTK_OVERRIDE;

private:
  vtkUnicodeStringArray(const vtkUnicodeStringArray&) VTK_DELETE_FUNCTION;
  void operator=(const vtkUnicodeStringArray&) VTK_DELETE_FUNCTION;

  class Implementation;
  Implementation* Internal;

};

#endif
