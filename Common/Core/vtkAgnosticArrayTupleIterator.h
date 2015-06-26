/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAgnosticArrayTupleIterator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAgnosticArrayTupleIterator
// .SECTION Description

#ifndef vtkAgnosticArrayTupleIterator_h
#define vtkAgnosticArrayTupleIterator_h

#include "vtkObject.h"

template <class ArrayTypeT>
class vtkAgnosticArrayTupleIterator
{
public:
  typedef vtkAgnosticArrayTupleIterator<ArrayTypeT> SelfType;

  typedef ArrayTypeT ArrayType;
  typedef typename ArrayType::ScalarType ScalarType;
  typedef typename ArrayType::ScalarReturnType ScalarReturnType;
  typedef typename ArrayType::TupleType TupleType;

  vtkAgnosticArrayTupleIterator(const ArrayType& associatedArray, const vtkIdType& index=0) :
    AssociatedArray(associatedArray), Index(index)
  {
  }
  vtkAgnosticArrayTupleIterator(const SelfType& other) :
    AssociatedArray(other.AssociatedArray), Index(other.Index)
  {
  }
  inline const vtkIdType& GetIndex() const { return this->Index; }
  inline void operator++() { ++this->Index; }
  inline void operator++(int) { this->Index++; }
  inline bool operator==(const SelfType& other) const
    {
    return (this->Index == other.Index);
    }
  inline bool operator!=(const SelfType& other) const
    {
    return (this->Index != other.Index);
    }
  inline ScalarReturnType operator[](int component) const
    {
    return this->AssociatedArray.GetComponentFast(this->Index, component);
    }
  inline TupleType operator*() const
    {
    return this->AssociatedArray.GetTupleFast(this->Index);
    }
private:
  const ArrayType& AssociatedArray;
  vtkIdType Index;
};
#endif
