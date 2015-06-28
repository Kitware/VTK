/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSoADataArrayTemplate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSoADataArrayTemplate
// .SECTION Description
// vtkSoADataArrayTemplate is the counterpart of vtkDataArrayTemplate.

#ifndef vtkSoADataArrayTemplate_h
#define vtkSoADataArrayTemplate_h

#include "vtkGenericDataArray.h"
#include <vector>

template <class ScalarTypeT>
class vtkSoADataArrayTemplate : public vtkTypeTemplate<
                            vtkSoADataArrayTemplate<ScalarTypeT>,
                            vtkGenericDataArray<vtkSoADataArrayTemplate<ScalarTypeT>, ScalarTypeT, std::vector<ScalarTypeT> >
                            >
{
  typedef vtkGenericDataArray<vtkSoADataArrayTemplate<ScalarTypeT>,
      ScalarTypeT,
      std::vector<ScalarTypeT> > GenericDataArrayType;
public:
  typedef vtkSoADataArrayTemplate<ScalarTypeT> SelfType;
  typedef typename GenericDataArrayType::ScalarType ScalarType;
  typedef typename GenericDataArrayType::TupleType TupleType;
  typedef typename GenericDataArrayType::ScalarReturnType ScalarReturnType;

  static SelfType* New();

  // **************************************************************************
  // Methods that are needed to be implemented by every vtkGenericDataArray
  // subclass.
  // **************************************************************************
  inline ScalarReturnType GetComponentFast(vtkIdType index, int comp) const
    {
    return this->Data[comp].Pointer[index];
    }
  inline TupleType GetTupleFast(vtkIdType index) const
    {
    TupleType tuple (this->NumberOfComponents>0? this->NumberOfComponents : 1);
    for (int cc=0; cc < this->NumberOfComponents; ++cc)
      {
      tuple[cc] = this->Data[cc].Pointer[index];
      }
    return tuple;
    }
  // **************************************************************************

  enum DeleteMethod
    {
    VTK_DATA_ARRAY_FREE,
    VTK_DATA_ARRAY_DELETE
    };

  // Description:
  // Use this API to pass externally allocated memory to this instance. Since
  // vtkSoADataArrayTemplate uses separate contiguous regions for each component,
  // use this API to add arrays for each of the component.
  // \c save: When set to true, vtkSoADataArrayTemplate will not release or realloc the memory
  // even when the AllocatorType is set to RESIZABLE. If needed it will simply
  // allow new memory buffers and "forget" the supplied pointers. When save is
  // set to false, this will be the \c deleteMethod specified to release the
  // array.
  // \c size is specified in number of elements of ScalarType.
  void SetArray(int comp, ScalarType* array, vtkIdType size,
    bool save=false, int deleteMethod=VTK_DATA_ARRAY_FREE);

  // Description:
  // Overridden to allocate pointer for each component.
  virtual void SetNumberOfComponents(int);

  // Description:
  // Call this method before using any of the methods on this array that affect
  // memory allocation. When set to false, any attempt to grow the arrays will
  // raise runtime exceptions. Any attempt to shrink the arrays will have no
  // effect.
  vtkSetMacro(Resizeable, bool);
  vtkGetMacro(Resizeable, bool);
  vtkBooleanMacro(Resizeable, bool);

protected:
  vtkSoADataArrayTemplate();
  ~vtkSoADataArrayTemplate();

  // **************************************************************************
  // Methods that are needed to be implemented by every vtkGenericDataArray
  // subclass.
  // **************************************************************************
  // Implement the memory management interface.
  bool AllocateTuples(vtkIdType numTuples);
  bool ReallocateTuples(vtkIdType numTuples);
  // **************************************************************************

  struct DataItem
    {
    ScalarType* Pointer;
    vtkIdType Size;
    bool Save;
    int DeleteMethod;
    DataItem() : Pointer(NULL), Size(0), DeleteMethod(VTK_DATA_ARRAY_FREE) {}
    };
  std::vector<DataItem> Data;
  bool Resizeable;
private:
  vtkSoADataArrayTemplate(const vtkSoADataArrayTemplate&); // Not implemented.
  void operator=(const vtkSoADataArrayTemplate&); // Not implemented.

  friend GenericDataArrayType;
};

#include "vtkSoADataArrayTemplate.txx"
#endif
