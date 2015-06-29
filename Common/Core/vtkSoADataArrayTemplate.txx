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
#include <cassert>

//-----------------------------------------------------------------------------
template<class ScalarType>
vtkSoADataArrayTemplate<ScalarType>*
vtkSoADataArrayTemplate<ScalarType>::New()
{
  VTK_STANDARD_NEW_BODY(vtkSoADataArrayTemplate<ScalarType>);
}

//-----------------------------------------------------------------------------
template<class ScalarType>
vtkSoADataArrayTemplate<ScalarType>::vtkSoADataArrayTemplate()
  : Resizeable(true)
{
}

//-----------------------------------------------------------------------------
template<class ScalarType>
vtkSoADataArrayTemplate<ScalarType>::~vtkSoADataArrayTemplate()
{
  for (int cc=0; cc < this->GetNumberOfComponents(); ++cc)
    {
    this->SetArray(cc, NULL, 0);
    }
}

//-----------------------------------------------------------------------------
template<class ScalarType>
void vtkSoADataArrayTemplate<ScalarType>::SetNumberOfComponents(int val)
{
  this->GenericDataArrayType::SetNumberOfComponents(val);
  size_t numComps = static_cast<size_t>(this->GetNumberOfComponents());
  assert(numComps >= 1);
  while (this->Data.size() > numComps)
    {
    this->SetArray(static_cast<int>(this->Data.size()) - 1, NULL, 0);
    this->Data.pop_back();
    }
  this->Data.resize(numComps);
}

//-----------------------------------------------------------------------------
template<class ScalarType>
void vtkSoADataArrayTemplate<ScalarType>::SetArray(
  int comp, ScalarType* array, vtkIdType size, bool save, int deleteMethod)
{
  const int numComps = this->GetNumberOfComponents();
  if (comp >= numComps || comp < 0)
    {
    vtkErrorMacro("Invalid component number '" << comp << "' specified. "
      "Use `SetNumberOfComponents` first to set the number of components.");
    return;
    }

  DataItem& item = this->Data[comp];
  if (item.Pointer == array) { return; }
  if (item.Pointer)
    {
    if (item.Save == false)
      {
      if (item.DeleteMethod == VTK_DATA_ARRAY_FREE)
        {
        free(item.Pointer);
        }
      else
        {
        delete [] item.Pointer;
        }
      }
    }
  item.Pointer = array;
  item.Size = size;
  item.Save = save;
  item.DeleteMethod = deleteMethod;
  this->DataChanged();
  // FIXME: Should we update MaxId like vtkDataArrayTemplate does? If so, how?
}

//-----------------------------------------------------------------------------
template<class ScalarType>
bool vtkSoADataArrayTemplate<ScalarType>::AllocateTuples(vtkIdType numTuples)
{
  if (!this->Resizeable)
    {
    vtkIdType minTuples = VTK_ID_MAX;
    for (int cc=0, max=this->GetNumberOfComponents(); cc < max; cc++)
      {
      minTuples = std::min(minTuples, this->Data[cc].Size);
      }
    if (numTuples <= minTuples)
      {
      return true;
      }
    else
      {
      vtkErrorMacro("AllocateTuples cannot be called on a non-resizeable array!");
      return false;
      }
    }

  for (int cc=0, max=this->GetNumberOfComponents(); cc < max; ++cc)
    {
    // Release old memory.
    this->SetArray(cc, NULL, 0);
    if (numTuples > 0)
      {
      ScalarType* newArray = static_cast<ScalarType*>(malloc(numTuples*sizeof(ScalarType)));
      if (!newArray)
        {
        return false;
        }
      this->SetArray(cc, newArray, numTuples, false, VTK_DATA_ARRAY_FREE);
      }
    }
  return true;
}

//-----------------------------------------------------------------------------
template<class ScalarType>
bool vtkSoADataArrayTemplate<ScalarType>::ReallocateTuples(vtkIdType numTuples)
{
  if (!this->Resizeable)
    {
    vtkIdType minTuples = VTK_ID_MAX;
    for (int cc=0, max=this->GetNumberOfComponents(); cc < max; cc++)
      {
      minTuples = std::min(minTuples, this->Data[cc].Size);
      }
    if (numTuples <= minTuples)
      {
      return true;
      }
    vtkErrorMacro("Resize attempted on a non-resizable array!");
    return false;
    }

  // Trivial case:
  if (numTuples == 0)
    {
    return this->AllocateTuples(0);
    }

  assert(numTuples > 0);

  // OS X's realloc does not free memory if the new block is smaller.  This
  // is a very serious problem and causes huge amount of memory to be
  // wasted. Do not use realloc on the Mac.
  bool dontUseRealloc=false;
  #if defined __APPLE__
  dontUseRealloc=true;
  #endif

  for (int cc=0, max=this->GetNumberOfComponents(); cc < max; ++cc)
    {
    DataItem& item = this->Data[cc];
    if (item.Pointer &&
      (item.Save || item.DeleteMethod == VTK_DATA_ARRAY_DELETE || dontUseRealloc))
      {
      ScalarType* newArray = static_cast<ScalarType*>(malloc(numTuples*sizeof(ScalarType)));
      if (!newArray)
        {
        return false;
        }
      std::copy(item.Pointer, item.Pointer + std::min(item.Size, numTuples), newArray);
      // now save the new array and release the old one too.
      this->SetArray(cc, newArray, numTuples, false, VTK_DATA_ARRAY_FREE);
      }
    else
      {
      // Try to reallocate with minimal memory usage and possibly avoid
      // copying.
      ScalarType* newArray = static_cast<ScalarType*>(realloc(item.Pointer, numTuples*sizeof(ScalarType)));
      if (!newArray)
        {
        return false;
        }
      item.Pointer = newArray;
      item.Size = numTuples;
      }
    }
  return true;
}
