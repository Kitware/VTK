/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSoAArrayTemplate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSoAArrayTemplate
// .SECTION Description

#ifndef vtkSoAArrayTemplate_h
#define vtkSoAArrayTemplate_h

#include "vtkAgnosticArray.h"
#include <vector>


template <class ArrayType>
class vtkResizeableSoAArrayAllocator
{
public:
  vtkResizeableSoAArrayAllocator()
    {
    }
  virtual ~vtkResizeableSoAArrayAllocator()
    {
    }
  virtual int Allocate(ArrayType* array, vtkIdType size, vtkIdType)
    {
    array->MaxId = -1;
    if (array->Size < size)
      {
      this->DeleteArray(array);

      int numComps = array->GetNumberOfComponents() > 0? array->GetNumberOfComponents() : 1;
      vtkIdType compSize = size / numComps;
      array->Data.resize(numComps);
      for (int cc=0; cc < numComps && compSize > 0; cc++)
        {
        array->Data[cc] = new typename ArrayType::ScalarType[compSize];
        }
      array->DataChanged();
      array->Size = numComps*compSize;
      }
    return 1;
    }
  virtual int Resize(ArrayType* array, vtkIdType numTuples)
    {
    if (this->Allocate(array, numTuples*array->GetNumberOfComponents(), 0))
      {
      vtkIdType newSize = numTuples * array->GetNumberOfComponents();
      array->MaxId = (newSize-1);
      return 1;
      }
    return 0;
    }
  virtual void DeleteArray(ArrayType* array)
    {
    for (size_t cc=0; cc < array->Data.size(); ++cc)
      {
      delete[] array->Data[cc];
      }
    array->Data.clear();
    array->MaxId = -1;
    array->Size = 0;
    }
};

template <class ScalarTypeT>
class vtkSoAArrayTemplate : public vtkTypeTemplate<
                            vtkSoAArrayTemplate<ScalarTypeT>,
                            vtkAgnosticArray<vtkSoAArrayTemplate<ScalarTypeT>,
                              ScalarTypeT,
                              std::vector<ScalarTypeT>,
                              vtkAgnosticArrayInputIterator<vtkSoAArrayTemplate<ScalarTypeT> >,
                              vtkResizeableSoAArrayAllocator<vtkSoAArrayTemplate<ScalarTypeT> > >
                            >
{
  typedef vtkAgnosticArray<vtkSoAArrayTemplate<ScalarTypeT>,
      ScalarTypeT,
      std::vector<ScalarTypeT>,
      vtkAgnosticArrayInputIterator<vtkSoAArrayTemplate<ScalarTypeT> >,
      vtkResizeableSoAArrayAllocator<vtkSoAArrayTemplate<ScalarTypeT> > > AgnosticArrayType;
public:
  typedef vtkSoAArrayTemplate<ScalarTypeT> SelfType;
  typedef typename AgnosticArrayType::ScalarType ScalarType;
  typedef typename AgnosticArrayType::TupleType TupleType;
  typedef typename AgnosticArrayType::ScalarReturnType ScalarReturnType;
  typedef typename AgnosticArrayType::IteratorType IteratorType;

  static SelfType* New();

  inline ScalarReturnType GetComponentFast(vtkIdType index, int comp) const
    {
    return this->Data[comp][index];
    }
  inline TupleType GetTupleFast(vtkIdType index) const
    {
    TupleType tuple (this->NumberOfComponents>0? this->NumberOfComponents : 1);
    for (int cc=0; cc < this->NumberOfComponents; ++cc)
      {
      tuple[cc] = this->Data[cc][index];
      }
    return tuple;
    }

protected:
  vtkSoAArrayTemplate();
  ~vtkSoAArrayTemplate();

  std::vector<ScalarTypeT*> Data;
private:
  vtkSoAArrayTemplate(const vtkSoAArrayTemplate&); // Not implemented.
  void operator=(const vtkSoAArrayTemplate&); // Not implemented.

  friend class vtkResizeableSoAArrayAllocator<SelfType>;
};

#include "vtkSoAArrayTemplate.txx"
#endif
