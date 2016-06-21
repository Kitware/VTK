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
// .NAME vtkArrayIteratorTemplate - Implementation template for a array
// iterator.
//
// .SECTION Description
// This is implementation template for a array iterator. It only works
// with arrays that have a contiguous internal storage of values (as in
// vtkDataArray, vtkStringArray).

#ifndef vtkArrayIteratorTemplate_h
#define vtkArrayIteratorTemplate_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkArrayIterator.h"

template <class T>
class VTKCOMMONCORE_EXPORT vtkArrayIteratorTemplate : public vtkArrayIterator
{
public:
  static vtkArrayIteratorTemplate<T>* New();
  vtkTemplateTypeMacro(vtkArrayIteratorTemplate<T>, vtkArrayIterator)
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the array this iterator will iterate over.
  // After Initialize() has been called, the iterator is valid
  // so long as the Array has not been modified
  // (except using the iterator itself).
  // If the array is modified, the iterator must be re-intialized.
  virtual void Initialize(vtkAbstractArray* array);

  // Description:
  // Get the array.
  vtkAbstractArray* GetArray(){ return this->Array; }


  // Description:
  // Must be called only after Initialize.
  T* GetTuple(vtkIdType id);

  // Description:
  // Must be called only after Initialize.
  T& GetValue(vtkIdType id)
    { return this->Pointer[id]; }

  // Description:
  // Sets the value at the index. This does not verify if the index is
  // valid.  The caller must ensure that id is less than the maximum
  // number of values.
  void SetValue(vtkIdType id, T value)
    {
    this->Pointer[id] = value;
    }

  // Description:
  // Must be called only after Initialize.
  vtkIdType GetNumberOfTuples();

  // Description:
  // Must be called only after Initialize.
  vtkIdType GetNumberOfValues();

  // Description:
  // Must be called only after Initialize.
  int GetNumberOfComponents();

  // Description:
  // Get the data type from the underlying array.
  int GetDataType();

  // Description:
  // Get the data type size from the underlying array.
  int GetDataTypeSize();

  // Description:
  // This is the data type for the value.
  typedef T ValueType;
protected:
  vtkArrayIteratorTemplate();
  ~vtkArrayIteratorTemplate();

  T* Pointer;
private:
  vtkArrayIteratorTemplate(const vtkArrayIteratorTemplate&) VTK_DELETE_FUNCTION;
  void operator=(const vtkArrayIteratorTemplate&) VTK_DELETE_FUNCTION;

 void SetArray(vtkAbstractArray*);
 vtkAbstractArray* Array;
};

#define VTK_ARRAY_ITERATOR_TEMPLATE_INSTANTIATE(T) \
  template class VTKCOMMONCORE_EXPORT vtkArrayIteratorTemplate< T >

#endif // !defined(vtkArrayIteratorTemplate_h)

// This portion must be OUTSIDE the include blockers.  Each
// vtkArrayIteratorTemplate subclass uses this to give its instantiation
// of this template a DLL interface.
#if defined(VTK_ARRAY_ITERATOR_TEMPLATE_TYPE)
# if defined(VTK_BUILD_SHARED_LIBS) && defined(_MSC_VER)
#  pragma warning (push)
#  pragma warning (disable: 4091) // warning C4091: 'extern ' :
   // ignored on left of 'int' when no variable is declared
#  pragma warning (disable: 4231) // Compiler-specific extension warning.
   // Use an "extern explicit instantiation" to give the class a DLL
   // interface.  This is a compiler-specific extension.
   extern VTK_ARRAY_ITERATOR_TEMPLATE_INSTANTIATE(VTK_ARRAY_ITERATOR_TEMPLATE_TYPE);
#  pragma warning (pop)
# endif
# undef VTK_ARRAY_ITERATOR_TEMPLATE_TYPE
#endif

// VTK-HeaderTest-Exclude: vtkArrayIteratorTemplate.h
