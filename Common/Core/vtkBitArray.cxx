// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkBitArray.h"

#include "vtkBitArrayIterator.h"
#include "vtkCommand.h"
#include "vtkIdList.h"
#include "vtkLookupTable.h"
#include "vtkObjectFactory.h"

#include <algorithm>
#include <cmath>

namespace
{
vtkMallocingFunction DefaultNewFunction = [](size_t size) -> void*
{ return new unsigned char[size]; };
vtkFreeingFunction DefaultDeleteFunction = [](void* ptr)
{ delete[] static_cast<unsigned char*>(ptr); };

constexpr unsigned char InitializationMaskForUnusedBitsOfLastByte[8] = { 0x80, 0xc0, 0xe0, 0xf0,
  0xf8, 0xfc, 0xfe, 0xff };
} // anonymous namespace

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
class vtkBitArrayLookup
{
public:
  vtkBitArrayLookup()
    : Rebuild(true)
  {
    this->ZeroArray = nullptr;
    this->OneArray = nullptr;
  }
  ~vtkBitArrayLookup()
  {
    if (this->ZeroArray)
    {
      this->ZeroArray->Delete();
      this->ZeroArray = nullptr;
    }
    if (this->OneArray)
    {
      this->OneArray->Delete();
      this->OneArray = nullptr;
    }
  }
  vtkIdList* ZeroArray;
  vtkIdList* OneArray;
  bool Rebuild;
};

vtkStandardNewMacro(vtkBitArray);

//------------------------------------------------------------------------------
// Instantiate object.
vtkBitArray::vtkBitArray()
{
  this->Buffer = vtkBuffer<ValueType>::New();
  this->Buffer->SetMallocFunction(DefaultNewFunction);
  this->Buffer->SetReallocFunction(nullptr);
  this->Buffer->SetFreeFunction(false, DefaultDeleteFunction);
  this->LegacyTuple.resize(3); // used for legacy API
  this->Lookup = nullptr;
}

//------------------------------------------------------------------------------
vtkBitArray::~vtkBitArray()
{
  this->Buffer->Delete();
  delete this->Lookup;
}

//------------------------------------------------------------------------------
bool vtkBitArray::AllocateTuples(vtkIdType numTuples)
{
  vtkIdType numBits = numTuples * this->GetNumberOfComponents();
  vtkIdType numBytes = (numBits + 7) / 8; // Round up to nearest byte

  if (this->Buffer->Allocate(numBytes))
  {
    this->Size = numBits; // Size in bits, not bytes
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
bool vtkBitArray::ReallocateTuples(vtkIdType numTuples)
{
  vtkIdType numBits = numTuples * this->GetNumberOfComponents();
  if (numBits == this->Size)
  {
    return true;
  }

  vtkIdType numBytes = (numBits + 7) / 8; // Round up to nearest byte
  if (this->Buffer->Reallocate(numBytes))
  {
    this->Size = numBits; // Size in bits, not bytes
    // Notify observers that the buffer may have changed
    this->InvokeEvent(vtkCommand::BufferChangedEvent);
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
vtkBitArray* vtkBitArray::FastDownCast(vtkAbstractArray* source)
{
  if (source)
  {
    switch (source->GetArrayType())
    {
      case vtkBitArray::ArrayTypeTag::value:
        return static_cast<vtkBitArray*>(source);
      default:
        break;
    }
  }
  return nullptr;
}

void vtkBitArray::InitializeUnusedBitsInLastByte()
{
  if (this->MaxId > -1)
  {
    auto div = std::div(this->MaxId, static_cast<vtkIdType>(8));
    this->Buffer->GetBuffer()[div.quot] &= InitializationMaskForUnusedBitsOfLastByte[div.rem];
  }
}

//------------------------------------------------------------------------------
vtkBitArray::ValueType* vtkBitArray::WritePointer(vtkIdType valueIdx, vtkIdType numValues)
{
  vtkIdType newSize = valueIdx + numValues;
  if (newSize > this->Size)
  {
    if (!this->Resize(newSize / this->NumberOfComponents + 1))
    {
      return nullptr;
    }
  }

  // For extending the in-use ids but not the size:
  if (newSize - 1 > this->MaxId)
  {
    this->MaxId = newSize - 1;
    this->InitializeUnusedBitsInLastByte();
  }

  this->DataChanged();
  return this->GetPointer(valueIdx); // Return pointer to byte containing bit at valueIdx
}

//------------------------------------------------------------------------------
// This method lets the user specify data to be held by the array.  The
// array argument is a pointer to the data.  size is the size of
// the array supplied by the user.  Set save to 1 to keep the class
// from deleting the array when it cleans up or reallocates memory.
// The class uses the actual array provided; it does not copy the data
// from the supplied array.
void vtkBitArray::SetArray(ValueType* array, vtkIdType size, int save, int deleteMethod)
{
  this->Buffer->SetBuffer(array, size);

  if (deleteMethod == VTK_DATA_ARRAY_DELETE)
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
  else if (deleteMethod == VTK_DATA_ARRAY_USER_DEFINED || deleteMethod == VTK_DATA_ARRAY_FREE)
  {
    this->Buffer->SetFreeFunction(save != 0, free);
  }

  this->Size = size;
  this->MaxId = this->Size - 1;
  this->InitializeUnusedBitsInLastByte();
  this->DataChanged();
}

//------------------------------------------------------------------------------
void vtkBitArray::SetArrayFreeFunction(void (*callback)(void*))
{
  this->Buffer->SetFreeFunction(false, callback);
}

//------------------------------------------------------------------------------
vtkBitArray::ValueType vtkBitArray::GetTypedComponent(vtkIdType tupleIdx, int comp) const
{
  vtkIdType id = this->NumberOfComponents * tupleIdx + comp;
  return this->GetValue(id);
}

//------------------------------------------------------------------------------
void vtkBitArray::SetTypedComponent(vtkIdType tupleIdx, int comp, ValueType value)
{
  vtkIdType id = this->NumberOfComponents * tupleIdx + comp;
  this->SetValue(id, value);
}

//------------------------------------------------------------------------------
void vtkBitArray::GetTypedTuple(vtkIdType tupleIdx, ValueType* tuple) const
{
  vtkIdType id = this->NumberOfComponents * tupleIdx;
  for (int i = 0; i < this->NumberOfComponents; i++)
  {
    tuple[i] = this->GetValue(id + i);
  }
}

//------------------------------------------------------------------------------
void vtkBitArray::SetTypedTuple(vtkIdType tupleIdx, const ValueType* tuple)
{
  vtkIdType id = this->NumberOfComponents * tupleIdx;
  for (int i = 0; i < this->NumberOfComponents; i++)
  {
    this->SetValue(id + i, tuple[i]);
  }
}

//------------------------------------------------------------------------------
// Get the data at a particular index.
int vtkBitArray::GetValue(vtkIdType id) const
{
  const auto div = std::div(id, static_cast<vtkIdType>(8));
  return (this->Buffer->GetBuffer()[div.quot] & (0x80 >> (div.rem))) != 0;
}

//------------------------------------------------------------------------------
// Allocate memory for this array. Delete old storage only if necessary.
vtkTypeBool vtkBitArray::Allocate(vtkIdType size, vtkIdType vtkNotUsed(ext))
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
void vtkBitArray::Initialize()
{
  this->Resize(0);
  this->DataChanged();
}

//------------------------------------------------------------------------------
// Deep copy of another bit array.
void vtkBitArray::DeepCopy(vtkDataArray* da)
{
  // Do nothing on a nullptr input.
  if (!da || this == da)
  {
    return;
  }

  if (da->GetDataType() == vtkBitArray::DataTypeTag::value)
  {
    // NOLINTNEXTLINE(bugprone-parent-virtual-call)
    this->vtkAbstractArray::DeepCopy(da);

    auto bitArray = vtkBitArray::FastDownCast(da);
    this->SetNumberOfComponents(da->GetNumberOfComponents());
    this->SetNumberOfTuples(da->GetNumberOfTuples());
    vtkIdType numBits = this->GetNumberOfValues();
    vtkIdType numBytes = (numBits + 7) / 8;
    std::copy_n(bitArray->Buffer->GetBuffer(), numBytes, this->Buffer->GetBuffer());

    this->SetLookupTable(nullptr);
    if (da->GetLookupTable())
    {
      this->SetLookupTable(da->GetLookupTable()->NewInstance());
      this->LookupTable->DeepCopy(da->GetLookupTable());
    }
  }
  else
  {
    this->Superclass::DeepCopy(da); // copy tuples, including Information object
  }
  this->DataChanged();
}

//------------------------------------------------------------------------------
void vtkBitArray::ShallowCopy(vtkDataArray* da)
{
  vtkBitArray* o = vtkBitArray::FastDownCast(da);
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
    this->Superclass::ShallowCopy(da);
  }
}

//------------------------------------------------------------------------------
void vtkBitArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->Buffer)
  {
    this->Buffer->PrintSelf(os, indent);
  }
}

//------------------------------------------------------------------------------
vtkTypeBool vtkBitArray::Resize(vtkIdType numTuples)
{
  int numComps = this->GetNumberOfComponents();
  vtkIdType curNumTuples = this->Size / (numComps > 0 ? numComps : 1);
  vtkIdType prevSize = this->Size;
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

  if (this->Size < prevSize)
  {
    this->MaxId = this->Size - 1;
    this->InitializeUnusedBitsInLastByte();
  }
  this->DataChanged();

  return 1;
}

//------------------------------------------------------------------------------
// Set the number of n-tuples in the array.
void vtkBitArray::SetNumberOfTuples(vtkIdType number)
{
  this->SetNumberOfValues(number * this->NumberOfComponents);
}

//------------------------------------------------------------------------------
bool vtkBitArray::SetNumberOfValues(vtkIdType number)
{
  if (!this->Superclass::SetNumberOfValues(number))
  {
    return false;
  }
  this->InitializeUnusedBitsInLastByte();
  return true;
}

//------------------------------------------------------------------------------
// Description:
// Set the tuple at the ith location using the jth tuple in the source array.
// This method assumes that the two arrays have the same type
// and structure. Note that range checking and memory allocation is not
// performed; use in conjunction with SetNumberOfTuples() to allocate space.
void vtkBitArray::SetTuple(vtkIdType i, vtkIdType j, vtkAbstractArray* source)
{
  vtkBitArray* ba = vtkArrayDownCast<vtkBitArray>(source);
  if (!ba)
  {
    vtkWarningMacro("Input and output arrays types do not match.");
    return;
  }

  vtkIdType loci = i * this->NumberOfComponents;
  vtkIdType locj = j * ba->GetNumberOfComponents();
  for (vtkIdType cur = 0; cur < this->NumberOfComponents; cur++)
  {
    this->SetValue(loci + cur, ba->GetValue(locj + cur));
  }
}

//------------------------------------------------------------------------------
// Description:
// Insert the jth tuple in the source array, at ith location in this array.
// Note that memory allocation is performed as necessary to hold the data.
void vtkBitArray::InsertTuple(vtkIdType i, vtkIdType j, vtkAbstractArray* source)
{
  vtkBitArray* other = vtkArrayDownCast<vtkBitArray>(source);
  if (!other)
  {
    vtkErrorMacro(<< "Shouldn't Happen: Couldn't downcast array into a vtkBitArray.");
    return;
  }

  vtkIdType loci = i * this->NumberOfComponents;
  vtkIdType locj = j * other->GetNumberOfComponents();
  for (vtkIdType cur = 0; cur < this->NumberOfComponents; cur++)
  {
    this->InsertValue(loci + cur, other->GetValue(locj + cur));
  }
}

//------------------------------------------------------------------------------
void vtkBitArray::InsertTuplesStartingAt(
  vtkIdType dstStart, vtkIdList* srcIds, vtkAbstractArray* source)
{
  if (!srcIds->GetNumberOfIds())
  {
    return;
  }
  vtkBitArray* other = vtkArrayDownCast<vtkBitArray>(source);
  if (!other)
  {
    vtkErrorMacro(<< "Shouldn't Happen: Couldn't downcast array into a vtkBitArray.");
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

  for (vtkIdType idIndex = 0; idIndex < srcIds->GetNumberOfIds(); ++idIndex)
  {
    vtkIdType numComp = this->NumberOfComponents;
    vtkIdType srcLoc = srcIds->GetId(idIndex) * this->NumberOfComponents;
    vtkIdType dstLoc = (dstStart + idIndex) * this->NumberOfComponents;
    while (numComp-- > 0)
    {
      this->InsertValue(dstLoc++, other->GetValue(srcLoc++));
    }
  }
}

//------------------------------------------------------------------------------
void vtkBitArray::InsertTuples(vtkIdList* dstIds, vtkIdList* srcIds, vtkAbstractArray* source)
{
  // First, check for the common case of typeid(source) == typeid(this). This
  // way we don't waste time redoing the other checks in the superclass, and
  // can avoid doing a dispatch for the most common usage of this method.
  vtkBitArray* other = vtkArrayDownCast<vtkBitArray>(source);
  if (!other)
  {
    vtkErrorMacro(<< "Shouldn't Happen: Couldn't downcast array into a vtkBitArray.");
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

  vtkIdType numIds = dstIds->GetNumberOfIds();
  for (vtkIdType idIndex = 0; idIndex < numIds; ++idIndex)
  {
    vtkIdType numComp = this->NumberOfComponents;
    vtkIdType srcLoc = srcIds->GetId(idIndex) * this->NumberOfComponents;
    vtkIdType dstLoc = dstIds->GetId(idIndex) * this->NumberOfComponents;
    while (numComp-- > 0)
    {
      this->InsertValue(dstLoc++, other->GetValue(srcLoc++));
    }
  }
}

//------------------------------------------------------------------------------
void vtkBitArray::InsertTuples(
  vtkIdType dstStart, vtkIdType n, vtkIdType srcStart, vtkAbstractArray* source)
{
  // First, check for the common case of typeid(source) == typeid(this). This
  // way we don't waste time redoing the other checks in the superclass, and
  // can avoid doing a dispatch for the most common usage of this method.
  vtkBitArray* other = vtkArrayDownCast<vtkBitArray>(source);
  if (!other)
  {
    // Let the superclass handle dispatch/fallback.
    vtkErrorMacro(<< "Shouldn't Happen: Couldn't downcast array into a vtkBitArray.");
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

  if (maxSrcTupleId >= other->GetNumberOfTuples())
  {
    vtkErrorMacro("Source array too small, requested tuple at index "
      << maxSrcTupleId << ", but there are only " << other->GetNumberOfTuples()
      << " tuples in the array.");
    return;
  }

  for (vtkIdType i = 0; i < n; ++i)
  {
    vtkIdType numComp = this->NumberOfComponents;
    vtkIdType srcLoc = (srcStart + i) * this->NumberOfComponents;
    vtkIdType dstLoc = (dstStart + i) * this->NumberOfComponents;
    while (numComp-- > 0)
    {
      this->InsertValue(dstLoc++, other->GetValue(srcLoc++));
    }
  }
}

//------------------------------------------------------------------------------
// Description:
// Insert the jth tuple in the source array, at the end in this array.
// Note that memory allocation is performed as necessary to hold the data.
// Returns the location at which the data was inserted.
vtkIdType vtkBitArray::InsertNextTuple(vtkIdType srcTupleIdx, vtkAbstractArray* source)
{
  vtkIdType nextTuple = this->GetNumberOfTuples();
  this->InsertTuple(nextTuple, srcTupleIdx, source);
  return nextTuple;
}

//------------------------------------------------------------------------------
// Get a pointer to a tuple at the ith location. This is a dangerous method
// (it is not thread safe since a pointer is returned).
double* vtkBitArray::GetTuple(vtkIdType i)
{
  this->LegacyTuple.resize(this->NumberOfComponents);
  this->GetTuple(i, this->LegacyTuple.data());
  return this->LegacyTuple.data();
}

//------------------------------------------------------------------------------
// Copy the tuple value into a user-provided array.
void vtkBitArray::GetTuple(vtkIdType i, double* tuple)
{
  vtkIdType loc = this->NumberOfComponents * i;
  for (int j = 0; j < this->NumberOfComponents; j++)
  {
    tuple[j] = static_cast<double>(this->GetValue(loc + j));
  }
}

//------------------------------------------------------------------------------
// Set the tuple value at the ith location in the array.
void vtkBitArray::SetTuple(vtkIdType i, const float* tuple)
{
  vtkIdType loc = i * this->NumberOfComponents;
  for (int j = 0; j < this->NumberOfComponents; j++)
  {
    this->SetValue(loc + j, static_cast<int>(tuple[j]));
  }
}

//------------------------------------------------------------------------------
void vtkBitArray::SetTuple(vtkIdType i, const double* tuple)
{
  vtkIdType loc = i * this->NumberOfComponents;
  for (int j = 0; j < this->NumberOfComponents; j++)
  {
    this->SetValue(loc + j, static_cast<int>(tuple[j]));
  }
}

//------------------------------------------------------------------------------
// Insert (memory allocation performed) the tuple into the ith location
// in the array.
void vtkBitArray::InsertTuple(vtkIdType tupleIdx, const float* tuple)
{
  vtkIdType loc = this->NumberOfComponents * tupleIdx;
  for (int j = 0; j < this->NumberOfComponents; j++)
  {
    this->InsertValue(loc + j, static_cast<int>(tuple[j]));
  }
}

//------------------------------------------------------------------------------
void vtkBitArray::InsertTuple(vtkIdType tupleIdx, const double* tuple)
{
  vtkIdType loc = this->NumberOfComponents * tupleIdx;
  for (int j = 0; j < this->NumberOfComponents; j++)
  {
    this->InsertValue(loc + j, static_cast<int>(tuple[j]));
  }
}

//------------------------------------------------------------------------------
// Insert (memory allocation performed) the tuple onto the end of the array.
vtkIdType vtkBitArray::InsertNextTuple(const float* tuple)
{
  vtkIdType nextTuple = this->GetNumberOfTuples();
  this->InsertTuple(nextTuple, tuple);
  return nextTuple;
}

//------------------------------------------------------------------------------
vtkIdType vtkBitArray::InsertNextTuple(const double* tuple)
{
  vtkIdType nextTuple = this->GetNumberOfTuples();
  this->InsertTuple(nextTuple, tuple);
  return nextTuple;
}

//------------------------------------------------------------------------------
void vtkBitArray::InsertComponent(vtkIdType i, int j, double c)
{
  this->InsertValue(i * this->NumberOfComponents + j, static_cast<int>(c));
}

//----------------------------------------------------------------------------
double vtkBitArray::GetComponent(vtkIdType tupleIdx, int compIdx)
{
  return static_cast<double>(this->GetTypedComponent(tupleIdx, compIdx));
}

//------------------------------------------------------------------------------
// Set the data component at the ith tuple and jth component location.
// Note that i<NumberOfTuples and j<NumberOfComponents. Make sure enough
// memory has been allocated (use SetNumberOfTuples() and
// SetNumberOfComponents()).
void vtkBitArray::SetComponent(vtkIdType i, int j, double c)
{
  this->SetValue(i * this->NumberOfComponents + j, static_cast<int>(c));
}

//------------------------------------------------------------------------------
void vtkBitArray::RemoveTuple(vtkIdType id)
{
  if (id < 0 || id >= this->GetNumberOfTuples())
  {
    // Nothing to be done
    return;
  }
  if (id == this->GetNumberOfTuples() - 1)
  {
    // To remove last item, just decrease the size by one
    this->RemoveLastTuple();
    return;
  }
  vtkErrorMacro("Not yet implemented...");
}

//------------------------------------------------------------------------------
void vtkBitArray::RemoveFirstTuple()
{
  vtkErrorMacro("Not yet implemented...");
  this->RemoveTuple(0);
}

//------------------------------------------------------------------------------
void vtkBitArray::RemoveLastTuple()
{
  this->Resize(this->GetNumberOfTuples() - 1);
}

//------------------------------------------------------------------------------
vtkArrayIterator* vtkBitArray::NewIterator()
{
  vtkArrayIterator* iter = vtkBitArrayIterator::New();
  iter->Initialize(this);
  return iter;
}

//------------------------------------------------------------------------------
void vtkBitArray::UpdateLookup()
{
  if (!this->Lookup)
  {
    this->Lookup = new vtkBitArrayLookup();
    this->Lookup->ZeroArray = vtkIdList::New();
    this->Lookup->OneArray = vtkIdList::New();
  }
  if (this->Lookup->Rebuild)
  {
    int numComps = this->GetNumberOfComponents();
    vtkIdType numTuples = this->GetNumberOfTuples();
    this->Lookup->ZeroArray->Allocate(numComps * numTuples);
    this->Lookup->OneArray->Allocate(numComps * numTuples);
    for (vtkIdType i = 0; i < numComps * numTuples; i++)
    {
      if (this->GetValue(i))
      {
        this->Lookup->OneArray->InsertNextId(i);
      }
      else
      {
        this->Lookup->ZeroArray->InsertNextId(i);
      }
    }
    this->Lookup->Rebuild = false;
  }
}

//------------------------------------------------------------------------------
vtkIdType vtkBitArray::LookupValue(vtkVariant var)
{
  return this->LookupValue(var.ToInt());
}

//------------------------------------------------------------------------------
void vtkBitArray::LookupValue(vtkVariant var, vtkIdList* ids)
{
  this->LookupValue(var.ToInt(), ids);
}

//------------------------------------------------------------------------------
vtkIdType vtkBitArray::LookupValue(int value)
{
  this->UpdateLookup();

  if (value == 1 && this->Lookup->OneArray->GetNumberOfIds() > 0)
  {
    return this->Lookup->OneArray->GetId(0);
  }
  else if (value == 0 && this->Lookup->ZeroArray->GetNumberOfIds() > 0)
  {
    return this->Lookup->ZeroArray->GetId(0);
  }
  return -1;
}

//------------------------------------------------------------------------------
void vtkBitArray::LookupValue(int value, vtkIdList* ids)
{
  this->UpdateLookup();

  if (value == 1)
  {
    ids->DeepCopy(this->Lookup->OneArray);
  }
  else if (value == 0)
  {
    ids->DeepCopy(this->Lookup->ZeroArray);
  }
  else
  {
    ids->Reset();
  }
}

//------------------------------------------------------------------------------
void vtkBitArray::DataChanged()
{
  if (this->Lookup)
  {
    this->Lookup->Rebuild = true;
  }
}

//------------------------------------------------------------------------------
void vtkBitArray::ClearLookup()
{
  delete this->Lookup;
  this->Lookup = nullptr;
}
VTK_ABI_NAMESPACE_END
