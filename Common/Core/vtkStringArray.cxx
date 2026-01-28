// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2004 Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

// We do not provide a definition for the copy constructor or
// operator=.  Block the warning.
#ifdef _MSC_VER
#pragma warning(disable : 4661)
#endif

#include "vtkStringArray.h"

#include "vtkArrayIteratorTemplate.h"
#include "vtkCharArray.h"
#include "vtkCommand.h"
#include "vtkIdList.h"
#include "vtkObjectFactory.h"
#include "vtkSMPTools.h"
#include "vtkStdString.h"

#include <algorithm>
#include <utility>
#include <vector>

namespace
{
vtkMallocingFunction DefaultNewFunction = [](size_t size) -> void*
{ return new vtkStdString[size]; };
vtkFreeingFunction DefaultDeleteFunction = [](void* ptr)
{ delete[] static_cast<vtkStdString*>(ptr); };
}

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
class vtkStringArrayLookup
{
public:
  vtkStringArrayLookup()
    : Rebuild(true)
  {
    this->SortedArray = nullptr;
    this->IndexArray = nullptr;
  }
  ~vtkStringArrayLookup()
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
  vtkStringArray* SortedArray;
  vtkIdList* IndexArray;
  bool Rebuild;
};

vtkStandardNewMacro(vtkStringArray);
vtkStandardExtendedNewMacro(vtkStringArray);

//------------------------------------------------------------------------------
vtkStringArray::vtkStringArray()
{
  this->Buffer = vtkBuffer<ValueType>::New();
  this->Buffer->SetMallocFunction(DefaultNewFunction);
  this->Buffer->SetReallocFunction(nullptr);
  this->Buffer->SetFreeFunction(false, DefaultDeleteFunction);
  this->Lookup = nullptr;
}

//------------------------------------------------------------------------------
vtkStringArray::~vtkStringArray()
{
  this->Buffer->Delete();
  delete this->Lookup;
}

//------------------------------------------------------------------------------
bool vtkStringArray::AllocateTuples(vtkIdType numTuples)
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
bool vtkStringArray::ReallocateTuples(vtkIdType numTuples)
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
bool vtkStringArray::EnsureAccessToTuple(vtkIdType tupleIdx)
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
vtkStringArray* vtkStringArray::FastDownCast(vtkAbstractArray* source)
{
  if (source)
  {
    switch (source->GetArrayType())
    {
      case vtkStringArray::ArrayTypeTag::value:
        return static_cast<vtkStringArray*>(source);
      default:
        break;
    }
  }
  return nullptr;
}

//------------------------------------------------------------------------------
vtkArrayIterator* vtkStringArray::NewIterator()
{
  vtkArrayIteratorTemplate<ValueType>* iter = vtkArrayIteratorTemplate<ValueType>::New();
  iter->Initialize(this);
  return iter;
}

//------------------------------------------------------------------------------
// This method lets the user specify data to be held by the array.  The
// array argument is a pointer to the data.  size is the size of
// the array supplied by the user.  Set save to 1 to keep the class
// from deleting the array when it cleans up or reallocates memory.
// The class uses the actual array provided; it does not copy the data
// from the suppled array.
void vtkStringArray::SetArray(ValueType* array, vtkIdType size, int save, int deleteMethod)
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
void vtkStringArray::SetArrayFreeFunction(void (*callback)(void*))
{
  this->Buffer->SetFreeFunction(false, callback);
}

//------------------------------------------------------------------------------
// Allocate memory for this array. Delete old storage only if necessary.

vtkTypeBool vtkStringArray::Allocate(vtkIdType size, vtkIdType vtkNotUsed(ext))
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
// Release storage and reset array to initial state.

void vtkStringArray::Initialize()
{
  this->Resize(0);
  this->DataChanged();
}

//------------------------------------------------------------------------------
bool vtkStringArray::CopyComponent(int dstComponent, vtkAbstractArray* src, int srcComponent)
{
  auto* source = vtkStringArray::SafeDownCast(src);
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
// Deep copy of another string array.

void vtkStringArray::DeepCopy(vtkAbstractArray* aa)
{
  // Do nothing on a nullptr input.
  if (!aa)
  {
    return;
  }
  vtkStringArray* sa = vtkArrayDownCast<vtkStringArray>(aa);
  if (sa == nullptr)
  {
    vtkErrorMacro(<< "Shouldn't Happen: Couldn't downcast array into a vtkStringArray.");
    return;
  }

  // Avoid self-copy.
  if (this != sa)
  {
    this->Superclass::DeepCopy(sa); // copy Information object

    this->SetNumberOfComponents(sa->GetNumberOfComponents());
    this->SetNumberOfTuples(sa->GetNumberOfTuples());

    std::copy_n(sa->Buffer->GetBuffer(), sa->GetNumberOfValues(), this->Buffer->GetBuffer());
    this->DataChanged();
  }
  this->Squeeze();
}

//------------------------------------------------------------------------------
void vtkStringArray::ShallowCopy(vtkAbstractArray* src)
{
  vtkStringArray* o = vtkStringArray::FastDownCast(src);
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
    vtkErrorMacro(<< "Shouldn't Happen: Couldn't downcast array into a vtkStringArray.");
  }
}

//------------------------------------------------------------------------------
// Interpolate array value from other array value given the
// indices and associated interpolation weights.
// This method assumes that the two arrays are of the same time.
void vtkStringArray::InterpolateTuple(
  vtkIdType i, vtkIdList* ptIndices, vtkAbstractArray* source, double* weights)
{
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

  // We use nearest neighbor for interpolating strings.
  // First determine which is the nearest neighbor using the weights-
  // it's the index with maximum weight.
  vtkIdType nearest = ptIndices->GetId(0);
  double max_weight = weights[0];
  for (int k = 1; k < ptIndices->GetNumberOfIds(); k++)
  {
    if (weights[k] > max_weight)
    {
      nearest = ptIndices->GetId(k);
      max_weight = weights[k];
    }
  }

  this->InsertTuple(i, nearest, source);
}

//------------------------------------------------------------------------------
// Interpolate value from the two values, p1 and p2, and an
// interpolation factor, t. The interpolation factor ranges from (0,1),
// with t=0 located at p1. This method assumes that the three arrays are of
// the same type. p1 is value at index id1 in fromArray1, while, p2 is
// value at index id2 in fromArray2.
void vtkStringArray::InterpolateTuple(vtkIdType i, vtkIdType id1, vtkAbstractArray* source1,
  vtkIdType id2, vtkAbstractArray* source2, double t)
{
  if (source1->GetDataType() != vtkStringArray::DataTypeTag::value ||
    source2->GetDataType() != vtkStringArray::DataTypeTag::value)
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
void vtkStringArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->Buffer)
  {
    this->Buffer->PrintSelf(os, indent);
  }
}

//------------------------------------------------------------------------------
vtkTypeBool vtkStringArray::Resize(vtkIdType numTuples)
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
vtkStringArray::ValueType* vtkStringArray::WritePointer(vtkIdType valueIdx, vtkIdType numValues)
{
  vtkIdType newSize = valueIdx + numValues;
  if (newSize > this->Size)
  {
    if (!this->Resize(newSize / this->NumberOfComponents + 1))
    {
      return nullptr;
    }
    this->MaxId = (newSize - 1);
  }

  // For extending the in-use ids but not the size:
  this->MaxId = std::max(this->MaxId, newSize - 1);

  this->DataChanged();
  return this->GetPointer(valueIdx);
}

//------------------------------------------------------------------------------
void vtkStringArray::InsertValue(vtkIdType valueIdx, ValueType value)
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
vtkIdType vtkStringArray::InsertNextValue(ValueType value)
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
int vtkStringArray::GetDataTypeSize() const
{
  return sizeof(ValueType);
}

//------------------------------------------------------------------------------
unsigned long vtkStringArray::GetActualMemorySize() const
{
  size_t totalSize = 0;
  size_t numPrims = static_cast<size_t>(this->GetSize());

  for (size_t i = 0; i < numPrims; ++i)
  {
    totalSize += sizeof(ValueType);
    totalSize += this->Buffer->GetBuffer()[i].size() * sizeof(ValueType::value_type);
  }

  return static_cast<unsigned long>(ceil(static_cast<double>(totalSize) / 1024.0)); // kibibytes
}

//------------------------------------------------------------------------------
vtkIdType vtkStringArray::GetDataSize() const
{
  size_t size = 0;
  size_t numStrs = static_cast<size_t>(this->GetMaxId() + 1);
  for (size_t i = 0; i < numStrs; i++)
  {
    size += this->Buffer->GetBuffer()[i].size() + 1;
    // (+1) for termination character.
  }
  return static_cast<vtkIdType>(size);
}

//------------------------------------------------------------------------------
// Set the tuple at the ith location using the jth tuple in the source array.
// This method assumes that the two arrays have the same type
// and structure. Note that range checking and memory allocation is not
// performed; use in conjunction with SetNumberOfTuples() to allocate space.
void vtkStringArray::SetTuple(
  vtkIdType dstTupleIdx, vtkIdType srcTupleIdx, vtkAbstractArray* source)
{
  // First, check for the common case of typeid(source) == typeid(this). This
  // way we don't waste time redoing the other checks in the superclass, and
  // can avoid doing a dispatch for the most common usage of this method.
  vtkStringArray* other = vtkArrayDownCast<vtkStringArray>(source);
  if (!other)
  {
    vtkErrorMacro(<< "Shouldn't Happen: Couldn't downcast array into a vtkStringArray.");
    return;
  }

  int numComps = this->GetNumberOfComponents();
  if (source->GetNumberOfComponents() != numComps)
  {
    vtkErrorMacro("Number of components do not match: Source: "
      << source->GetNumberOfComponents() << " Dest: " << this->GetNumberOfComponents());
    return;
  }

  for (int c = 0; c < numComps; ++c)
  {
    this->SetTypedComponent(dstTupleIdx, c, other->GetTypedComponent(srcTupleIdx, c));
  }
}

//------------------------------------------------------------------------------
// Insert the jth tuple in the source array, at ith location in this array.
// Note that memory allocation is performed as necessary to hold the data.
void vtkStringArray::InsertTuple(vtkIdType i, vtkIdType j, vtkAbstractArray* source)
{
  this->EnsureAccessToTuple(i);
  this->SetTuple(i, j, source);
}

//------------------------------------------------------------------------------
void vtkStringArray::InsertTuples(vtkIdList* dstIds, vtkIdList* srcIds, vtkAbstractArray* source)
{
  // First, check for the common case of typeid(source) == typeid(this). This
  // way we don't waste time redoing the other checks in the superclass, and
  // can avoid doing a dispatch for the most common usage of this method.
  vtkStringArray* other = vtkArrayDownCast<vtkStringArray>(source);
  if (!other)
  {
    vtkErrorMacro(<< "Shouldn't Happen: Couldn't downcast array into a vtkStringArray.");
    return;
  }

  if (dstIds->GetNumberOfIds() == 0)
  {
    return;
  }

  if (dstIds->GetNumberOfIds() != srcIds->GetNumberOfIds())
  {
    vtkErrorMacro("Mismatched number of tuples ids. Source: "
      << srcIds->GetNumberOfIds() << " Dest: " << dstIds->GetNumberOfIds());
    return;
  }

  int numComps = this->GetNumberOfComponents();
  if (other->GetNumberOfComponents() != numComps)
  {
    vtkErrorMacro("Number of components do not match: Source: "
      << other->GetNumberOfComponents() << " Dest: " << this->GetNumberOfComponents());
    return;
  }

  vtkIdType maxSrcTupleId = srcIds->GetId(0);
  vtkIdType maxDstTupleId = dstIds->GetId(0);
  for (int i = 0; i < dstIds->GetNumberOfIds(); ++i)
  {
    // parenthesis around std::max prevent MSVC macro replacement when
    // inlined:
    maxSrcTupleId = (std::max)(maxSrcTupleId, srcIds->GetId(i));
    maxDstTupleId = (std::max)(maxDstTupleId, dstIds->GetId(i));
  }

  if (maxSrcTupleId >= other->GetNumberOfTuples())
  {
    vtkErrorMacro("Source array too small, requested tuple at index "
      << maxSrcTupleId << ", but there are only " << other->GetNumberOfTuples()
      << " tuples in the array.");
    return;
  }

  vtkIdType newSize = (maxDstTupleId + 1) * this->NumberOfComponents;
  if (this->Size < newSize)
  {
    if (!this->Resize(maxDstTupleId + 1))
    {
      vtkErrorMacro("Resize failed.");
      return;
    }
  }

  // parenthesis around std::max prevent MSVC macro replacement when
  // inlined:
  this->MaxId = (std::max)(this->MaxId, newSize - 1);

  vtkIdType numTuples = srcIds->GetNumberOfIds();
  for (vtkIdType t = 0; t < numTuples; ++t)
  {
    vtkIdType srcT = srcIds->GetId(t);
    vtkIdType dstT = dstIds->GetId(t);
    for (int c = 0; c < numComps; ++c)
    {
      this->SetTypedComponent(dstT, c, other->GetTypedComponent(srcT, c));
    }
  }
}

//------------------------------------------------------------------------------
void vtkStringArray::InsertTuplesStartingAt(
  vtkIdType dstStart, vtkIdList* srcIds, vtkAbstractArray* source)
{
  if (!srcIds->GetNumberOfIds())
  {
    return;
  }

  // First, check for the common case of typeid(source) == typeid(this). This
  // way we don't waste time redoing the other checks in the superclass, and
  // can avoid doing a dispatch for the most common usage of this method.
  vtkStringArray* other = vtkArrayDownCast<vtkStringArray>(source);
  if (!other)
  {
    vtkErrorMacro(<< "Shouldn't Happen: Couldn't downcast array into a vtkStringArray.");
    return;
  }

  int numComps = this->GetNumberOfComponents();
  if (other->GetNumberOfComponents() != numComps)
  {
    vtkErrorMacro("Number of components do not match: Source: "
      << other->GetNumberOfComponents() << " Dest: " << this->GetNumberOfComponents());
    return;
  }

  vtkIdType maxSrcTupleId = srcIds->GetId(0);
  vtkIdType maxDstTupleId = dstStart + srcIds->GetNumberOfIds() - 1;
  for (int i = 0; i < srcIds->GetNumberOfIds(); ++i)
  {
    // parenthesis around std::max prevent MSVC macro replacement when
    // inlined:
    maxSrcTupleId = (std::max)(maxSrcTupleId, srcIds->GetId(i));
  }

  if (maxSrcTupleId >= other->GetNumberOfTuples())
  {
    vtkErrorMacro("Source array too small, requested tuple at index "
      << maxSrcTupleId << ", but there are only " << other->GetNumberOfTuples()
      << " tuples in the array.");
    return;
  }

  vtkIdType newSize = (maxDstTupleId + 1) * this->NumberOfComponents;
  if (this->Size < newSize)
  {
    if (!this->Resize(maxDstTupleId + 1))
    {
      vtkErrorMacro("Resize failed.");
      return;
    }
  }

  // parenthesis around std::max prevent MSVC macro replacement when
  // inlined:
  this->MaxId = (std::max)(this->MaxId, newSize - 1);

  vtkIdType numTuples = srcIds->GetNumberOfIds();
  for (vtkIdType t = 0; t < numTuples; ++t)
  {
    vtkIdType srcT = srcIds->GetId(t);
    vtkIdType dstT = dstStart + t;
    for (int c = 0; c < numComps; ++c)
    {
      this->SetTypedComponent(dstT, c, other->GetTypedComponent(srcT, c));
    }
  }
}

//------------------------------------------------------------------------------
void vtkStringArray::InsertTuples(
  vtkIdType dstStart, vtkIdType n, vtkIdType srcStart, vtkAbstractArray* source)
{
  // First, check for the common case of typeid(source) == typeid(this). This
  // way we don't waste time redoing the other checks in the superclass, and
  // can avoid doing a dispatch for the most common usage of this method.
  vtkStringArray* other = vtkArrayDownCast<vtkStringArray>(source);
  if (!other)
  {
    // Let the superclass handle dispatch/fallback.
    vtkErrorMacro(<< "Shouldn't Happen: Couldn't downcast array into a vtkStringArray.");
    return;
  }

  if (n == 0)
  {
    return;
  }

  int numComps = this->GetNumberOfComponents();
  if (other->GetNumberOfComponents() != numComps)
  {
    vtkErrorMacro("Number of components do not match: Source: "
      << other->GetNumberOfComponents() << " Dest: " << this->GetNumberOfComponents());
    return;
  }

  vtkIdType maxSrcTupleId = srcStart + n - 1;
  vtkIdType maxDstTupleId = dstStart + n - 1;

  if (maxSrcTupleId >= other->GetNumberOfTuples())
  {
    vtkErrorMacro("Source array too small, requested tuple at index "
      << maxSrcTupleId << ", but there are only " << other->GetNumberOfTuples()
      << " tuples in the array.");
    return;
  }

  vtkIdType newSize = (maxDstTupleId + 1) * this->NumberOfComponents;
  if (this->Size < newSize)
  {
    if (!this->Resize(maxDstTupleId + 1))
    {
      vtkErrorMacro("Resize failed.");
      return;
    }
  }

  this->MaxId = std::max(this->MaxId, newSize - 1);

  ValueType* srcBegin = other->GetPointer(srcStart * numComps);
  ValueType* srcEnd = srcBegin + (n * numComps);
  ValueType* dstBegin = this->GetPointer(dstStart * numComps);

  std::copy(srcBegin, srcEnd, dstBegin);
}

//------------------------------------------------------------------------------
// Insert the jth tuple in the source array, at the end in this array.
// Note that memory allocation is performed as necessary to hold the data.
// Returns the location at which the data was inserted.
vtkIdType vtkStringArray::InsertNextTuple(vtkIdType srcTupleIdx, vtkAbstractArray* source)
{
  vtkIdType nextTuple = this->GetNumberOfTuples();
  this->InsertTuple(nextTuple, srcTupleIdx, source);
  return nextTuple;
}

//------------------------------------------------------------------------------
vtkStringArray::ValueType vtkStringArray::GetTypedComponent(vtkIdType tupleIdx, int comp) const
{
  return this->Buffer->GetBuffer()[this->NumberOfComponents * tupleIdx + comp];
}

//------------------------------------------------------------------------------
void vtkStringArray::SetTypedComponent(vtkIdType tupleIdx, int comp, ValueType value)
{
  this->Buffer->GetBuffer()[this->NumberOfComponents * tupleIdx + comp] = value;
  this->DataChanged();
}

//------------------------------------------------------------------------------
void vtkStringArray::GetTypedTuple(vtkIdType tupleIdx, ValueType* tuple) const
{
  const vtkIdType valueIdx = tupleIdx * this->NumberOfComponents;
  std::copy_n(this->Buffer->GetBuffer() + valueIdx, this->NumberOfComponents, tuple);
}

//------------------------------------------------------------------------------
void vtkStringArray::SetTypedTuple(vtkIdType tupleIdx, const ValueType* tuple)
  VTK_EXPECTS(0 <= tupleIdx && tupleIdx < GetNumberOfTuples())
{
  const vtkIdType valueIdx = tupleIdx * this->NumberOfComponents;
  std::copy_n(tuple, this->NumberOfComponents, this->Buffer->GetBuffer() + valueIdx);
  this->DataChanged();
}

//------------------------------------------------------------------------------
const vtkStringArray::ValueType& vtkStringArray::GetValue(vtkIdType valueIdx) const
{
  return this->Buffer->GetBuffer()[valueIdx];
}

vtkStringArray::ValueType& vtkStringArray::GetValue(vtkIdType valueIdx)
{
  return this->Buffer->GetBuffer()[valueIdx];
}

//------------------------------------------------------------------------------
void vtkStringArray::GetTuples(vtkIdList* tupleIds, vtkAbstractArray* aa)
{
  // First, check for the common case of typeid(source) == typeid(this). This
  // way we don't waste time redoing the other checks in the superclass, and
  // can avoid doing a dispatch for the most common usage of this method.
  vtkStringArray* other = vtkArrayDownCast<vtkStringArray>(aa);
  if (!other)
  {
    vtkErrorMacro(<< "Shouldn't Happen: Couldn't downcast array into a vtkStringArray.");
    return;
  }

  int numComps = this->GetNumberOfComponents();
  if (other->GetNumberOfComponents() != numComps)
  {
    vtkErrorMacro("Number of components for input and output do not match.\n"
                  "Source: "
      << this->GetNumberOfComponents()
      << "\n"
         "Destination: "
      << other->GetNumberOfComponents());
    return;
  }

  vtkIdType* srcTuple = tupleIds->GetPointer(0);
  vtkIdType* srcTupleEnd = tupleIds->GetPointer(tupleIds->GetNumberOfIds());
  vtkIdType dstTuple = 0;

  while (srcTuple != srcTupleEnd)
  {
    for (int c = 0; c < numComps; ++c)
    {
      other->SetTypedComponent(dstTuple, c, this->GetTypedComponent(*srcTuple, c));
    }
    ++srcTuple;
    ++dstTuple;
  }
}

//------------------------------------------------------------------------------
void vtkStringArray::GetTuples(vtkIdType p1, vtkIdType p2, vtkAbstractArray* aa)
{
  // First, check for the common case of typeid(source) == typeid(this). This
  // way we don't waste time redoing the other checks in the superclass, and
  // can avoid doing a dispatch for the most common usage of this method.
  vtkStringArray* other = vtkArrayDownCast<vtkStringArray>(aa);
  if (!other)
  {
    vtkErrorMacro(<< "Shouldn't Happen: Couldn't downcast array into a vtkStringArray.");
    return;
  }

  int numComps = this->GetNumberOfComponents();
  if (other->GetNumberOfComponents() != numComps)
  {
    vtkErrorMacro("Number of components for input and output do not match.\n"
                  "Source: "
      << this->GetNumberOfComponents()
      << "\n"
         "Destination: "
      << other->GetNumberOfComponents());
    return;
  }

  // p1-p2 are inclusive
  for (vtkIdType srcT = p1, dstT = 0; srcT <= p2; ++srcT, ++dstT)
  {
    for (int c = 0; c < numComps; ++c)
    {
      other->SetTypedComponent(dstT, c, this->GetTypedComponent(srcT, c));
    }
  }
}

//------------------------------------------------------------------------------
void vtkStringArray::UpdateLookup()
{
  if (!this->Lookup)
  {
    this->Lookup = new vtkStringArrayLookup();
    this->Lookup->SortedArray = vtkStringArray::New();
    this->Lookup->IndexArray = vtkIdList::New();
  }
  if (this->Lookup->Rebuild)
  {
    int numComps = this->GetNumberOfComponents();
    vtkIdType numTuples = this->GetNumberOfTuples();
    this->Lookup->SortedArray->Initialize();
    this->Lookup->SortedArray->SetNumberOfComponents(numComps);
    this->Lookup->SortedArray->SetNumberOfTuples(numTuples);
    this->Lookup->IndexArray->SetNumberOfIds(numComps * numTuples);
    std::vector<std::pair<ValueType, vtkIdType>> v;
    v.reserve(numComps * numTuples);
    for (vtkIdType i = 0; i < numComps * numTuples; i++)
    {
      v.emplace_back(this->Buffer->GetBuffer()[i], i);
    }
    std::sort(v.begin(), v.end());
    for (vtkIdType i = 0; i < numComps * numTuples; i++)
    {
      this->Lookup->SortedArray->SetValue(i, v[i].first);
      this->Lookup->IndexArray->SetId(i, v[i].second);
    }
    this->Lookup->Rebuild = false;
  }
}

//------------------------------------------------------------------------------
vtkIdType vtkStringArray::LookupValue(vtkVariant var)
{
  return this->LookupValue(var.ToString());
}

//------------------------------------------------------------------------------
void vtkStringArray::LookupValue(vtkVariant var, vtkIdList* ids)
{
  this->LookupValue(var.ToString(), ids);
}

//------------------------------------------------------------------------------
vtkIdType vtkStringArray::LookupValue(const ValueType& value)
{
  this->UpdateLookup();

  int numComps = this->Lookup->SortedArray->GetNumberOfComponents();
  vtkIdType numTuples = this->Lookup->SortedArray->GetNumberOfTuples();
  ValueType* ptr = this->Lookup->SortedArray->GetPointer(0);
  ValueType* ptrEnd = ptr + numComps * numTuples;
  ValueType* found = std::lower_bound(ptr, ptrEnd, value);

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
void vtkStringArray::LookupValue(const ValueType& value, vtkIdList* ids)
{
  this->UpdateLookup();
  ids->Reset();

  // Perform a binary search of the sorted array using STL equal_range.
  int numComps = this->GetNumberOfComponents();
  vtkIdType numTuples = this->GetNumberOfTuples();
  ValueType* ptr = this->Lookup->SortedArray->GetPointer(0);
  std::pair<ValueType*, ValueType*> found =
    std::equal_range(ptr, ptr + numComps * numTuples, value);

  // Add the indices of the found items to the ID list.
  vtkIdType offset = static_cast<vtkIdType>(found.first - ptr);
  while (found.first != found.second)
  {
    // Check that the value in the original array hasn't changed.
    vtkIdType index = this->Lookup->IndexArray->GetId(offset);
    ValueType currentValue = this->GetValue(index);
    if (*found.first == currentValue)
    {
      ids->InsertNextId(index);
    }

    ++found.first;
    ++offset;
  }
}

//------------------------------------------------------------------------------
void vtkStringArray::DataChanged()
{
  if (this->Lookup)
  {
    this->Lookup->Rebuild = true;
  }
}

//------------------------------------------------------------------------------
void vtkStringArray::DataElementChanged(vtkIdType vtkNotUsed(id))
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
void vtkStringArray::ClearLookup()
{
  delete this->Lookup;
  this->Lookup = nullptr;
}

//------------------------------------------------------------------------------

//
//
// Below here are interface methods to allow values to be inserted as
// const char * instead of ValueType.  Yes, they're trivial.  The
// wrapper code needs them.
//
//

void vtkStringArray::SetValue(vtkIdType id, const char* value)
{
  if (value)
  {
    this->SetValue(id, ValueType(value));
  }
}

void vtkStringArray::InsertValue(vtkIdType id, const char* value)
{
  if (value)
  {
    this->InsertValue(id, ValueType(value));
  }
}

vtkVariant vtkStringArray::GetVariantValue(vtkIdType id)
{
  return vtkVariant(this->GetValue(id));
}

void vtkStringArray::SetVariantValue(vtkIdType id, vtkVariant value)
{
  this->SetValue(id, value.ToString());
}

void vtkStringArray::InsertVariantValue(vtkIdType id, vtkVariant value)
{
  this->InsertValue(id, value.ToString());
}

vtkIdType vtkStringArray::InsertNextValue(const char* value)
{
  if (value)
  {
    return this->InsertNextValue(ValueType(value));
  }
  return this->MaxId;
}

vtkIdType vtkStringArray::LookupValue(const char* value)
{
  if (value)
  {
    return this->LookupValue(ValueType(value));
  }
  return -1;
}

void vtkStringArray::LookupValue(const char* value, vtkIdList* ids)
{
  if (value)
  {
    this->LookupValue(ValueType(value), ids);
    return;
  }
  ids->Reset();
}
VTK_ABI_NAMESPACE_END
