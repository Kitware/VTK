/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrayIteratorTemplate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkArrayIteratorTemplate
 * @brief   Implementation template for a array
 * iterator.
 *
 *
 * This is implementation template for a array iterator. It only works
 * with arrays that have a contiguous internal storage of values (as in
 * vtkDataArray, vtkStringArray).
*/

#ifndef vtkArrayIteratorTemplate_h
#define vtkArrayIteratorTemplate_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkArrayIterator.h"

#include "vtkStdString.h" // For template instantiation
#include "vtkUnicodeString.h" // For template instantiation
#include "vtkVariant.h" // For template instantiation

template <class T>
class VTKCOMMONCORE_EXPORT vtkArrayIteratorTemplate : public vtkArrayIterator
{
public:
  static vtkArrayIteratorTemplate<T>* New();
  vtkTemplateTypeMacro(vtkArrayIteratorTemplate<T>, vtkArrayIterator)
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set the array this iterator will iterate over.
   * After Initialize() has been called, the iterator is valid
   * so long as the Array has not been modified
   * (except using the iterator itself).
   * If the array is modified, the iterator must be re-initialized.
   */
  void Initialize(vtkAbstractArray* array) override;

  /**
   * Get the array.
   */
  vtkAbstractArray* GetArray(){ return this->Array; }


  /**
   * Must be called only after Initialize.
   */
  T* GetTuple(vtkIdType id);

  /**
   * Must be called only after Initialize.
   */
  T& GetValue(vtkIdType id)
    { return this->Pointer[id]; }

  /**
   * Sets the value at the index. This does not verify if the index is
   * valid.  The caller must ensure that id is less than the maximum
   * number of values.
   */
  void SetValue(vtkIdType id, T value)
  {
    this->Pointer[id] = value;
  }

  /**
   * Must be called only after Initialize.
   */
  vtkIdType GetNumberOfTuples();

  /**
   * Must be called only after Initialize.
   */
  vtkIdType GetNumberOfValues();

  /**
   * Must be called only after Initialize.
   */
  int GetNumberOfComponents();

  /**
   * Get the data type from the underlying array.
   */
  int GetDataType() override;

  /**
   * Get the data type size from the underlying array.
   */
  int GetDataTypeSize();

  /**
   * This is the data type for the value.
   */
  typedef T ValueType;

protected:
  vtkArrayIteratorTemplate();
  ~vtkArrayIteratorTemplate() override;

  T* Pointer;
private:
  vtkArrayIteratorTemplate(const vtkArrayIteratorTemplate&) = delete;
  void operator=(const vtkArrayIteratorTemplate&) = delete;

 void SetArray(vtkAbstractArray*);
 vtkAbstractArray* Array;
};

#ifdef VTK_USE_EXTERN_TEMPLATE
#ifndef vtkArrayIteratorTemplateInstantiate_cxx
#ifdef _MSC_VER
#pragma warning (push)
// The following is needed when the vtkArrayIteratorTemplate is declared
// dllexport and is used from another class in vtkCommonCore
#pragma warning (disable: 4910) // extern and dllexport incompatible
#endif
vtkInstantiateTemplateMacro(
  extern template class VTKCOMMONCORE_EXPORT vtkArrayIteratorTemplate)
extern template class VTKCOMMONCORE_EXPORT
  vtkArrayIteratorTemplate<vtkStdString>;
extern template class VTKCOMMONCORE_EXPORT
  vtkArrayIteratorTemplate<vtkUnicodeString>;
extern template class VTKCOMMONCORE_EXPORT
  vtkArrayIteratorTemplate<vtkVariant>;
#ifdef _MSC_VER
#pragma warning (pop)
#endif
#endif
#endif // VTK_USE_EXTERN_TEMPLATE

#endif

// VTK-HeaderTest-Exclude: vtkArrayIteratorTemplate.h
