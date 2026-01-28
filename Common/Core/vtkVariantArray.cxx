// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

// We do not provide a definition for the copy constructor or
// operator=.  Block the warning.
#ifdef _MSC_VER
#pragma warning(disable : 4661)
#endif

#include "vtkVariantArray.h"

#include "vtkArrayIteratorTemplate.h"
#include "vtkCommand.h"
#include "vtkDataArray.h"
#include "vtkIdList.h"
#include "vtkObjectFactory.h"
#include "vtkSMPTools.h"
#include "vtkSortDataArray.h"
#include "vtkStringArray.h"
#include "vtkVariant.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <utility>

namespace
{
vtkMallocingFunction DefaultNewFunction = [](size_t size) -> void* { return new vtkVariant[size]; };
vtkFreeingFunction DefaultDeleteFunction = [](void* ptr)
{ delete[] static_cast<vtkVariant*>(ptr); };
}

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
class vtkVariantArrayLookup
{
public:
  vtkVariantArrayLookup()
    : Rebuild(true)
  {
    this->SortedArray = nullptr;
    this->IndexArray = nullptr;
  }
  ~vtkVariantArrayLookup()
  {
    if (this->SortedArray)
    {
      this->SortedArray->Delete();
      this->SortedArray = nullptr;
    }
    if (this->IndexArray)
    {
      this->IndexArray->Delete();
      this->IndexArray = nullptr;
    }
  }
  vtkVariantArray* SortedArray;
  vtkIdList* IndexArray;
  bool Rebuild;
};

//
// Standard functions
//

vtkStandardNewMacro(vtkVariantArray);
vtkStandardExtendedNewMacro(vtkVariantArray);

//------------------------------------------------------------------------------
vtkVariantArray::vtkVariantArray()
{
  this->Buffer = vtkBuffer<ValueType>::New();
  this->Buffer->SetMallocFunction(DefaultNewFunction);
  this->Buffer->SetReallocFunction(nullptr);
  this->Buffer->SetFreeFunction(false, DefaultDeleteFunction);
  this->Lookup = nullptr;
}

//------------------------------------------------------------------------------
vtkVariantArray::~vtkVariantArray()
{
  this->Buffer->Delete();
  delete this->Lookup;
}

//------------------------------------------------------------------------------
void vtkVariantArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->Buffer)
  {
    this->Buffer->PrintSelf(os, indent);
  }
}

//------------------------------------------------------------------------------
bool vtkVariantArray::AllocateTuples(vtkIdType numTuples)
{
  vtkIdType numValues = numTuples * this->GetNumberOfComponents();
  if (this->Buffer->Allocate(numValues))
  {
    this->Size = this->Buffer->GetSize();
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
bool vtkVariantArray::ReallocateTuples(vtkIdType numTuples)
{
  vtkIdType newSize = numTuples * this->GetNumberOfComponents();
  if (newSize == this->Size)
  {
    return true;
  }

  if (this->Buffer->Reallocate(newSize))
  {
    this->Size = this->Buffer->GetSize();
    // Notify observers that the buffer may have changed
    this->InvokeEvent(vtkCommand::BufferChangedEvent);
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
bool vtkVariantArray::EnsureAccessToTuple(vtkIdType tupleIdx)
{
  if (tupleIdx < 0)
  {
    return false;
  }
  vtkIdType minSize = (1 + tupleIdx) * this->NumberOfComponents;
  vtkIdType expectedMaxId = minSize - 1;
  if (this->MaxId < expectedMaxId)
  {
    if (this->Size < minSize)
    {
      if (!this->Resize(tupleIdx + 1))
      {
        return false;
      }
    }
    this->MaxId = expectedMaxId;
  }
  return true;
}

//------------------------------------------------------------------------------
vtkVariantArray* vtkVariantArray::FastDownCast(vtkAbstractArray* source)
{
  if (source)
  {
    switch (source->GetArrayType())
    {
      case vtkVariantArray::ArrayTypeTag::value:
        return static_cast<vtkVariantArray*>(source);
      default:
        break;
    }
  }
  return nullptr;
}

//
//
// Functions required by vtkAbstractArray
//
//

//------------------------------------------------------------------------------
vtkTypeBool vtkVariantArray::Allocate(vtkIdType size, vtkIdType vtkNotUsed(ext))
{
  // Allocator must update this->MaxId properly.
  this->MaxId = -1;
  if (size > this->Size || size == 0)
  {
    this->Size = 0;

    // let's keep the size an integral multiple of the number of components.
    size = size < 0 ? 0 : size;
    int numComps = this->GetNumberOfComponents() > 0 ? this->GetNumberOfComponents() : 1;
    double ceilNum = ceil(static_cast<double>(size) / static_cast<double>(numComps));
    vtkIdType numTuples = static_cast<vtkIdType>(ceilNum);
    // NOTE: if numTuples is 0, AllocateTuples is expected to release the
    // memory.
    if (this->AllocateTuples(numTuples) == false)
    {
      vtkErrorMacro(
        "Unable to allocate " << size << " elements of size " << sizeof(ValueType) << " bytes. ");
#if !defined VTK_DONT_THROW_BAD_ALLOC
      // We can throw something that has universal meaning
      throw std::bad_alloc();
#else
      // We indicate that alloc failed by return
      return 0;
#endif
    }
    this->Size = numTuples * numComps;
  }
  this->DataChanged();
  return 1;
}

//------------------------------------------------------------------------------
void vtkVariantArray::Initialize()
{
  this->Resize(0);
  this->DataChanged();
}

//------------------------------------------------------------------------------
bool vtkVariantArray::CopyComponent(int dstComponent, vtkAbstractArray* src, int srcComponent)
{
  auto* source = vtkVariantArray::SafeDownCast(src);
  if (!source || source->GetNumberOfTuples() != this->GetNumberOfTuples() || srcComponent < 0 ||
    srcComponent >= source->GetNumberOfComponents() || dstComponent < 0 ||
    dstComponent >= this->GetNumberOfComponents())
  {
    return false;
  }

  vtkIdType nn = this->GetNumberOfTuples();
  vtkSMPTools::For(0, nn,
    [this, dstComponent, source, srcComponent](vtkIdType begin, vtkIdType end)
    {
      vtkIdType ndc = this->GetNumberOfComponents();
      vtkIdType nsc = source->GetNumberOfComponents();
      for (vtkIdType ii = begin; ii < end; ++ii)
      {
        this->SetValue(ii * ndc + dstComponent, source->GetValue(ii * nsc + srcComponent));
      }
    });
  return true;
}

//------------------------------------------------------------------------------
int vtkVariantArray::GetDataType() const
{
  return vtkVariantArray::DataTypeTag::value;
}

//------------------------------------------------------------------------------
int vtkVariantArray::GetDataTypeSize() const
{
  return sizeof(ValueType);
}

//------------------------------------------------------------------------------
int vtkVariantArray::GetElementComponentSize() const
{
  return this->GetDataTypeSize();
}

//------------------------------------------------------------------------------
void vtkVariantArray::SetNumberOfTuples(vtkIdType number)
{
  this->SetNumberOfValues(this->NumberOfComponents * number);
}

//------------------------------------------------------------------------------
void vtkVariantArray::SetTuple(vtkIdType i, vtkIdType j, vtkAbstractArray* source)
{
  if (source->GetNumberOfComponents() != this->GetNumberOfComponents())
  {
    vtkErrorMacro("Number of components do not match: Source: "
      << source->GetNumberOfComponents() << " Dest: " << this->GetNumberOfComponents());
    return;
  }
  if (source->IsA("vtkVariantArray"))
  {
    vtkVariantArray* a = vtkArrayDownCast<vtkVariantArray>(source);
    vtkIdType loci = i * this->NumberOfComponents;
    vtkIdType locj = j * a->GetNumberOfComponents();
    for (vtkIdType cur = 0; cur < this->NumberOfComponents; cur++)
    {
      this->SetValue(loci + cur, a->GetValue(locj + cur));
    }
  }
  else if (source->IsA("vtkDataArray"))
  {
    vtkDataArray* a = vtkArrayDownCast<vtkDataArray>(source);
    vtkIdType loci = i * this->NumberOfComponents;
    vtkIdType locj = j * a->GetNumberOfComponents();
    switch (a->GetDataType())
    {
      vtkTemplateMacro(for (vtkIdType cur = 0; cur < this->NumberOfComponents; cur++) {
        vtkIdType tuple = (locj + cur) / a->GetNumberOfComponents();
        int component = static_cast<int>((locj + cur) % a->GetNumberOfComponents());
        this->SetValue(
          loci + cur, ValueType(static_cast<VTK_TT>(a->GetComponent(tuple, component))));
      });
    }
  }
  else if (source->IsA("vtkStringArray"))
  {
    vtkStringArray* a = vtkArrayDownCast<vtkStringArray>(source);
    vtkIdType loci = i * this->NumberOfComponents;
    vtkIdType locj = j * a->GetNumberOfComponents();
    for (vtkIdType cur = 0; cur < this->NumberOfComponents; cur++)
    {
      this->SetValue(loci + cur, ValueType(a->GetValue(locj + cur)));
    }
  }
  else
  {
    vtkWarningMacro("Unrecognized type is incompatible with vtkVariantArray.");
  }
}

//------------------------------------------------------------------------------
void vtkVariantArray::InsertTuple(vtkIdType i, vtkIdType j, vtkAbstractArray* source)
{
  this->EnsureAccessToTuple(i);
  this->SetTuple(i, j, source);
}

//------------------------------------------------------------------------------
void vtkVariantArray::InsertTuplesStartingAt(
  vtkIdType dstStart, vtkIdList* srcIds, vtkAbstractArray* source)
{
  if (!srcIds->GetNumberOfIds())
  {
    return;
  }

  if (this->GetNumberOfComponents() != source->GetNumberOfComponents())
  {
    vtkWarningMacro("Input and output component sizes do not match.");
    return;
  }

  vtkIdType numIds = srcIds->GetNumberOfIds();
  if (vtkVariantArray* va = vtkArrayDownCast<vtkVariantArray>(source))
  {
    for (vtkIdType idIndex = 0; idIndex < numIds; ++idIndex)
    {
      vtkIdType numComp = this->NumberOfComponents;
      vtkIdType srcLoc = srcIds->GetId(idIndex) * this->NumberOfComponents;
      vtkIdType dstLoc = (dstStart + idIndex) * this->NumberOfComponents;
      while (numComp-- > 0)
      {
        this->InsertValue(dstLoc++, va->GetValue(srcLoc++));
      }
    }
  }
  else if (vtkDataArray* da = vtkDataArray::FastDownCast(source))
  {
    for (vtkIdType idIndex = 0; idIndex < numIds; ++idIndex)
    {
      vtkIdType numComp = this->NumberOfComponents;
      vtkIdType srcLoc = srcIds->GetId(idIndex) * this->NumberOfComponents;
      vtkIdType dstLoc = (dstStart + idIndex) * this->NumberOfComponents;
      while (numComp-- > 0)
      {
        this->InsertValue(dstLoc++, da->GetVariantValue(srcLoc++));
      }
    }
  }
  else if (vtkStringArray* sa = vtkArrayDownCast<vtkStringArray>(source))
  {
    for (vtkIdType idIndex = 0; idIndex < numIds; ++idIndex)
    {
      vtkIdType numComp = this->NumberOfComponents;
      vtkIdType srcLoc = srcIds->GetId(idIndex) * this->NumberOfComponents;
      vtkIdType dstLoc = (dstStart + idIndex) * this->NumberOfComponents;
      while (numComp-- > 0)
      {
        this->InsertValue(dstLoc++, ValueType(sa->GetValue(srcLoc++)));
      }
    }
  }
  else
  {
    vtkWarningMacro("Unrecognized type is incompatible with vtkVariantArray.");
  }
}

//------------------------------------------------------------------------------
void vtkVariantArray::InsertTuples(vtkIdList* dstIds, vtkIdList* srcIds, vtkAbstractArray* source)
{

  if (this->GetNumberOfComponents() != source->GetNumberOfComponents())
  {
    vtkWarningMacro("Input and output component sizes do not match.");
    return;
  }

  vtkIdType numIds = dstIds->GetNumberOfIds();
  if (srcIds->GetNumberOfIds() != numIds)
  {
    vtkWarningMacro("Input and output id array sizes do not match.");
    return;
  }

  if (vtkVariantArray* va = vtkArrayDownCast<vtkVariantArray>(source))
  {
    for (vtkIdType idIndex = 0; idIndex < numIds; ++idIndex)
    {
      vtkIdType numComp = this->NumberOfComponents;
      vtkIdType srcLoc = srcIds->GetId(idIndex) * this->NumberOfComponents;
      vtkIdType dstLoc = dstIds->GetId(idIndex) * this->NumberOfComponents;
      while (numComp-- > 0)
      {
        this->InsertValue(dstLoc++, va->GetValue(srcLoc++));
      }
    }
  }
  else if (vtkDataArray* da = vtkDataArray::FastDownCast(source))
  {
    for (vtkIdType idIndex = 0; idIndex < numIds; ++idIndex)
    {
      vtkIdType numComp = this->NumberOfComponents;
      vtkIdType srcLoc = srcIds->GetId(idIndex) * this->NumberOfComponents;
      vtkIdType dstLoc = dstIds->GetId(idIndex) * this->NumberOfComponents;
      while (numComp-- > 0)
      {
        this->InsertValue(dstLoc++, da->GetVariantValue(srcLoc++));
      }
    }
  }
  else if (vtkStringArray* sa = vtkArrayDownCast<vtkStringArray>(source))
  {
    for (vtkIdType idIndex = 0; idIndex < numIds; ++idIndex)
    {
      vtkIdType numComp = this->NumberOfComponents;
      vtkIdType srcLoc = srcIds->GetId(idIndex) * this->NumberOfComponents;
      vtkIdType dstLoc = dstIds->GetId(idIndex) * this->NumberOfComponents;
      while (numComp-- > 0)
      {
        this->InsertValue(dstLoc++, ValueType(sa->GetValue(srcLoc++)));
      }
    }
  }
  else
  {
    vtkWarningMacro("Unrecognized type is incompatible with vtkVariantArray.");
  }
}

//------------------------------------------------------------------------------
void vtkVariantArray::InsertTuples(
  vtkIdType dstStart, vtkIdType n, vtkIdType srcStart, vtkAbstractArray* source)
{
  if (this->GetNumberOfComponents() != source->GetNumberOfComponents())
  {
    vtkWarningMacro("Input and output component sizes do not match.");
    return;
  }

  vtkIdType srcEnd = srcStart + n;
  if (srcEnd > source->GetNumberOfTuples())
  {
    vtkWarningMacro("Source range exceeds array size (srcStart="
      << srcStart << ", n=" << n << ", numTuples=" << source->GetNumberOfTuples() << ").");
    return;
  }

  for (vtkIdType i = 0; i < n; ++i)
  {
    vtkIdType numComp = this->NumberOfComponents;
    vtkIdType srcLoc = (srcStart + i) * this->NumberOfComponents;
    vtkIdType dstLoc = (dstStart + i) * this->NumberOfComponents;
    while (numComp-- > 0)
    {
      this->InsertValue(dstLoc++, source->GetVariantValue(srcLoc++));
    }
  }
}

//------------------------------------------------------------------------------
vtkIdType vtkVariantArray::InsertNextTuple(vtkIdType srcTupleIdx, vtkAbstractArray* source)
{
  vtkIdType nextTuple = this->GetNumberOfTuples();
  this->InsertTuple(nextTuple, srcTupleIdx, source);
  return nextTuple;
}

//------------------------------------------------------------------------------
void* vtkVariantArray::GetVoidPointer(vtkIdType id)
{
  return this->GetPointer(id);
}

//------------------------------------------------------------------------------
void vtkVariantArray::DeepCopy(vtkAbstractArray* aa)
{
  // Do nothing on a nullptr input.
  if (!aa)
  {
    return;
  }
  vtkVariantArray* va = vtkArrayDownCast<vtkVariantArray>(aa);
  if (va == nullptr)
  {
    vtkErrorMacro(<< "Shouldn't Happen: Couldn't downcast array into a vtkVariantArray.");
    return;
  }

  // Avoid self-copy.
  if (this != va)
  {
    this->Superclass::DeepCopy(va); // copy Information object

    this->SetNumberOfComponents(va->GetNumberOfComponents());
    this->SetNumberOfTuples(va->GetNumberOfTuples());

    std::copy_n(va->Buffer->GetBuffer(), va->GetNumberOfValues(), this->Buffer->GetBuffer());
    this->DataChanged();
  }
  this->Squeeze();
}

//------------------------------------------------------------------------------
void vtkVariantArray::ShallowCopy(vtkAbstractArray* src)
{
  vtkVariantArray* o = vtkVariantArray::FastDownCast(src);
  if (o)
  {
    this->Size = o->Size;
    this->MaxId = o->MaxId;
    this->SetName(o->Name);
    this->SetNumberOfComponents(o->NumberOfComponents);
    this->CopyComponentNames(o);

    if (this->Buffer != o->Buffer)
    {
      this->Buffer->Delete();
      this->Buffer = o->Buffer;
      this->Buffer->Register(nullptr);
    }
    this->DataChanged();
  }
  else
  {
    vtkErrorMacro(<< "Shouldn't Happen: Couldn't downcast array into a vtkVariantArray.");
  }
}

//------------------------------------------------------------------------------
void vtkVariantArray::InterpolateTuple(
  vtkIdType i, vtkIdList* ptIndices, vtkAbstractArray* source, double* weights)
{
  // Note: Something much more fancy could be done here, allowing
  // the source array be any data type.
  if (this->GetDataType() != source->GetDataType())
  {
    vtkErrorMacro("Cannot CopyValue from array of type " << source->GetDataTypeAsString());
    return;
  }

  if (ptIndices->GetNumberOfIds() == 0)
  {
    // nothing to do.
    return;
  }

  // We use nearest neighbour for interpolating variants.
  // First determine which is the nearest neighbour using the weights-
  // it's the index with maximum weight.
  vtkIdType nearest = ptIndices->GetId(0);
  double max_weight = weights[0];
  for (int k = 1; k < ptIndices->GetNumberOfIds(); k++)
  {
    if (weights[k] > max_weight)
    {
      nearest = k;
    }
  }

  this->InsertTuple(i, nearest, source);
}

//------------------------------------------------------------------------------
void vtkVariantArray::InterpolateTuple(vtkIdType i, vtkIdType id1, vtkAbstractArray* source1,
  vtkIdType id2, vtkAbstractArray* source2, double t)
{
  // Note: Something much more fancy could be done here, allowing
  // the source array to be any data type.
  if (source1->GetDataType() != vtkVariantArray::DataTypeTag::value ||
    source2->GetDataType() != vtkVariantArray::DataTypeTag::value)
  {
    vtkErrorMacro("All arrays to InterpolateValue() must be of same type.");
    return;
  }

  if (t >= 0.5)
  {
    // Use p2
    this->InsertTuple(i, id2, source2);
  }
  else
  {
    // Use p1.
    this->InsertTuple(i, id1, source1);
  }
}

//------------------------------------------------------------------------------
vtkTypeBool vtkVariantArray::Resize(vtkIdType numTuples)
{
  int numComps = this->GetNumberOfComponents();
  vtkIdType curNumTuples = this->Size / (numComps > 0 ? numComps : 1);
  if (numTuples > curNumTuples)
  {
    // Requested size is bigger than current size.  Allocate enough
    // memory to fit the requested size and be more than double the
    // currently allocated memory.
    numTuples = curNumTuples + numTuples;
  }
  else if (numTuples == curNumTuples)
  {
    return 1;
  }
  else
  {
    // Requested size is smaller than current size.  Squeeze the
    // memory.
    this->DataChanged();
  }

  assert(numTuples >= 0);

  if (!this->ReallocateTuples(numTuples))
  {
    vtkErrorMacro("Unable to allocate " << numTuples * numComps << " elements of size "
                                        << sizeof(ValueType) << " bytes. ");
#if !defined NDEBUG
    // We're debugging, crash here preserving the stack
    abort();
#elif !defined VTK_DONT_THROW_BAD_ALLOC
    // We can throw something that has universal meaning
    throw std::bad_alloc();
#else
    // We indicate that malloc failed by return
    return 0;
#endif
  }

  // Allocation was successful. Save it.
  this->Size = numTuples * numComps;

  // Update MaxId if we truncated:
  this->MaxId = std::min(this->Size - 1, this->MaxId);

  return 1;
}

//------------------------------------------------------------------------------
unsigned long vtkVariantArray::GetActualMemorySize() const
{
  // NOTE: Currently does not take into account the "pointed to" data.
  size_t totalSize = 0;
  size_t numPrims = static_cast<size_t>(this->GetSize());

  totalSize = numPrims * sizeof(ValueType);

  return static_cast<unsigned long>(ceil(static_cast<double>(totalSize) / 1024.0)); // kibibytes
}

//------------------------------------------------------------------------------
int vtkVariantArray::IsNumeric() const
{
  return 0;
}

//------------------------------------------------------------------------------
vtkArrayIterator* vtkVariantArray::NewIterator()
{
  vtkArrayIteratorTemplate<ValueType>* iter = vtkArrayIteratorTemplate<ValueType>::New();
  iter->Initialize(this);
  return iter;
}

//
//
// Additional functions
//
//

//------------------------------------------------------------------------------
vtkVariantArray::ValueType vtkVariantArray::GetTypedComponent(vtkIdType tupleIdx, int comp) const
{
  return this->Buffer->GetBuffer()[this->NumberOfComponents * tupleIdx + comp];
}

//------------------------------------------------------------------------------
void vtkVariantArray::SetTypedComponent(vtkIdType tupleIdx, int comp, ValueType value)
{
  this->Buffer->GetBuffer()[this->NumberOfComponents * tupleIdx + comp] = value;
  this->DataChanged();
}

//------------------------------------------------------------------------------
void vtkVariantArray::GetTypedTuple(vtkIdType tupleIdx, ValueType* tuple) const
{
  const vtkIdType valueIdx = tupleIdx * this->NumberOfComponents;
  std::copy_n(this->Buffer->GetBuffer() + valueIdx, this->NumberOfComponents, tuple);
}

//------------------------------------------------------------------------------
void vtkVariantArray::SetTypedTuple(vtkIdType tupleIdx, const ValueType* tuple)
  VTK_EXPECTS(0 <= tupleIdx && tupleIdx < GetNumberOfTuples())
{
  const vtkIdType valueIdx = tupleIdx * this->NumberOfComponents;
  std::copy_n(tuple, this->NumberOfComponents, this->Buffer->GetBuffer() + valueIdx);
  this->DataChanged();
}

//------------------------------------------------------------------------------
vtkVariantArray::ValueType& vtkVariantArray::GetValue(vtkIdType valueIdx) const
{
  return this->Buffer->GetBuffer()[valueIdx];
}

//------------------------------------------------------------------------------
void vtkVariantArray::SetValue(vtkIdType valueIdx, ValueType value)
{
  this->Buffer->GetBuffer()[valueIdx] = value;
  this->DataChanged();
}

//------------------------------------------------------------------------------
void vtkVariantArray::InsertValue(vtkIdType valueIdx, ValueType value)
{
  vtkIdType tuple = valueIdx / this->NumberOfComponents;
  // Update MaxId to the inserted component (not the complete tuple) for
  // compatibility with InsertNextValue.
  vtkIdType newMaxId = valueIdx > this->MaxId ? valueIdx : this->MaxId;
  if (this->EnsureAccessToTuple(tuple))
  {
    assert("Sufficient space allocated." && this->MaxId >= newMaxId);
    this->MaxId = newMaxId;
    this->SetValue(valueIdx, value);
  }
}

//------------------------------------------------------------------------------
vtkVariant vtkVariantArray::GetVariantValue(vtkIdType id)
{
  return this->GetValue(id);
}

//------------------------------------------------------------------------------
void vtkVariantArray::SetVariantValue(vtkIdType id, ValueType value)
{
  this->SetValue(id, value);
}

//------------------------------------------------------------------------------
void vtkVariantArray::InsertVariantValue(vtkIdType id, ValueType value)
{
  this->InsertValue(id, value);
}

//------------------------------------------------------------------------------
vtkIdType vtkVariantArray::InsertNextValue(ValueType value)
{
  vtkIdType nextValueIdx = this->MaxId + 1;
  if (nextValueIdx >= this->Size)
  {
    vtkIdType tuple = nextValueIdx / this->NumberOfComponents;
    this->EnsureAccessToTuple(tuple);
    // Since EnsureAccessToTuple will update the MaxId to point to the last
    // component in the last tuple, we move it back to support this method on
    // multi-component arrays.
    this->MaxId = nextValueIdx;
  }

  // Extending array without needing to reallocate:
  this->MaxId = std::max(this->MaxId, nextValueIdx);

  this->SetValue(nextValueIdx, value);
  return nextValueIdx;
}

//------------------------------------------------------------------------------
vtkVariantArray::ValueType* vtkVariantArray::GetPointer(vtkIdType id)
{
  return this->Buffer->GetBuffer() + id;
}

//------------------------------------------------------------------------------
void vtkVariantArray::SetArray(ValueType* array, vtkIdType size, int save, int deleteMethod)
{
  this->Buffer->SetBuffer(array, size);

  if (deleteMethod == VTK_DATA_ARRAY_DELETE || deleteMethod == VTK_DATA_ARRAY_USER_DEFINED)
  {
    this->Buffer->SetFreeFunction(save != 0, DefaultDeleteFunction);
  }
  else if (deleteMethod == VTK_DATA_ARRAY_ALIGNED_FREE)
  {
#ifdef _WIN32
    this->Buffer->SetFreeFunction(save != 0, _aligned_free);
#else
    this->Buffer->SetFreeFunction(save != 0, free);
#endif
  }
  else if (deleteMethod == VTK_DATA_ARRAY_FREE)
  {
    this->Buffer->SetFreeFunction(save != 0, free);
  }

  this->Size = size;
  this->MaxId = this->Size - 1;
  this->DataChanged();
}

//------------------------------------------------------------------------------
void vtkVariantArray::SetArrayFreeFunction(void (*callback)(void*))
{
  this->Buffer->SetFreeFunction(false, callback);
}

//------------------------------------------------------------------------------
void vtkVariantArray::UpdateLookup()
{
  if (!this->Lookup)
  {
    this->Lookup = new vtkVariantArrayLookup();
    this->Lookup->SortedArray = vtkVariantArray::New();
    this->Lookup->IndexArray = vtkIdList::New();
  }
  if (this->Lookup->Rebuild)
  {
    int numComps = this->GetNumberOfComponents();
    vtkIdType numTuples = this->GetNumberOfTuples();
    this->Lookup->SortedArray->DeepCopy(this);
    this->Lookup->IndexArray->SetNumberOfIds(numComps * numTuples);
    for (vtkIdType i = 0; i < numComps * numTuples; i++)
    {
      this->Lookup->IndexArray->SetId(i, i);
    }
    vtkSortDataArray::Sort(this->Lookup->SortedArray, this->Lookup->IndexArray);
    this->Lookup->Rebuild = false;
  }
}

//------------------------------------------------------------------------------
vtkIdType vtkVariantArray::LookupValue(ValueType value)
{
  this->UpdateLookup();

  // Perform a binary search of the sorted array using STL equal_range.
  int numComps = this->Lookup->SortedArray->GetNumberOfComponents();
  vtkIdType numTuples = this->Lookup->SortedArray->GetNumberOfTuples();
  ValueType* ptr = this->Lookup->SortedArray->GetPointer(0);
  ValueType* ptrEnd = ptr + numComps * numTuples;
  ValueType* found = std::lower_bound(ptr, ptrEnd, value, vtkVariantLessThan());

  // Find an index with a matching value. Non-matching values might
  // show up here when the underlying value at that index has been
  // changed (so the sorted array is out-of-date).
  vtkIdType offset = static_cast<vtkIdType>(found - ptr);
  while (found != ptrEnd)
  {
    // Check whether we still have a value equivalent to what we're
    // looking for.
    if (value == *found)
    {
      // Check that the value in the original array hasn't changed.
      vtkIdType index = this->Lookup->IndexArray->GetId(offset);
      ValueType currentValue = this->GetValue(index);
      if (value == currentValue)
      {
        return index;
      }
    }
    else
    {
      break;
    }

    ++found;
    ++offset;
  }

  return -1;
}

//------------------------------------------------------------------------------
void vtkVariantArray::LookupValue(ValueType value, vtkIdList* ids)
{
  this->UpdateLookup();
  ids->Reset();

  // Perform a binary search of the sorted array using STL equal_range.
  int numComps = this->GetNumberOfComponents();
  vtkIdType numTuples = this->GetNumberOfTuples();
  ValueType* ptr = this->Lookup->SortedArray->GetPointer(0);
  ValueType* ptrEnd = ptr + numComps * numTuples;
  std::pair<ValueType*, ValueType*> found =
    std::equal_range(ptr, ptrEnd, value, vtkVariantLessThan());

  // Add the indices of the found items to the ID list.
  vtkIdType offset = static_cast<vtkIdType>(found.first - ptr);
  while (found.first != found.second)
  {
    // Check that the value in the original array hasn't changed.
    vtkIdType index = this->Lookup->IndexArray->GetId(offset);
    ValueType currentValue = this->GetValue(index);
    if (*(found.first) == currentValue)
    {
      ids->InsertNextId(index);
    }

    ++found.first;
    ++offset;
  }
}

//------------------------------------------------------------------------------
void vtkVariantArray::DataChanged()
{
  if (this->Lookup)
  {
    this->Lookup->Rebuild = true;
  }
}

//------------------------------------------------------------------------------
void vtkVariantArray::DataElementChanged(vtkIdType vtkNotUsed(id))
{
  if (this->Lookup)
  {
    if (this->Lookup->Rebuild)
    {
      // We're already going to rebuild the lookup table. Do nothing.
      return;
    }
    this->Lookup->Rebuild = true;
  }
}

//------------------------------------------------------------------------------
void vtkVariantArray::ClearLookup()
{
  delete this->Lookup;
  this->Lookup = nullptr;
}
VTK_ABI_NAMESPACE_END
