// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkFieldData.h"

#include "vtkDataArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkSMPTools.h"
#include "vtkUnsignedCharArray.h"

#include <array>
#include <limits>
#include <tuple>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkFieldData);
vtkStandardExtendedNewMacro(vtkFieldData);

namespace
{
using CachedGhostRangeType = std::tuple<vtkMTimeType, vtkMTimeType, std::vector<double>>;

//------------------------------------------------------------------------------
// This function is used to generalize the call to vtkDataArray::GetRange
// and vtkDataArray::GetFiniteRange without having to copy / paste.
bool GetRangeImpl(vtkFieldData* self, int index, double range[2], int comp,
  std::vector<std::array<::CachedGhostRangeType, 2>>& ranges,
  bool (vtkDataArray::*ComputeVectorRangeMethod)(double*, const unsigned char*, unsigned char),
  bool (vtkDataArray::*ComputeScalarRangeMethod)(double*, const unsigned char*, unsigned char))
{
  auto array = vtkArrayDownCast<vtkDataArray>(self->GetAbstractArray(index));
  if (array && (comp < array->GetNumberOfComponents() || comp == -1))
  {
    if (comp == -1 && array->GetNumberOfComponents() == 1)
    {
      comp = 0;
    }
    CachedGhostRangeType& cache = ranges[index][comp == -1 ? 0 : 1];
    vtkMTimeType& arrayTime = std::get<0>(cache);
    vtkMTimeType& ghostTime = std::get<1>(cache);

    // It is possible that the number of components get changed at some point.
    // If it happens, just update the cache size. The range will be recomputed no matter
    // what thanks to the time stamp
    if (comp != -1)
    {
      std::get<2>(cache).resize(array->GetNumberOfComponents() * 2);
    }
    double* cachedRange = std::get<2>(cache).data();

    vtkUnsignedCharArray* ghosts = self->GetGhostArray();
    bool retVal = true;

    if (arrayTime != array->GetMTime() || ghostTime != (ghosts ? ghosts->GetMTime() : 0))
    {
      if (comp < 0)
      {
        retVal = (array->*ComputeVectorRangeMethod)(cachedRange,
          ghosts ? ghosts->GetPointer(0) : nullptr, ghosts ? self->GetGhostsToSkip() : 0);
      }
      else
      {
        retVal = (array->*ComputeScalarRangeMethod)(cachedRange,
          ghosts ? ghosts->GetPointer(0) : nullptr, ghosts ? self->GetGhostsToSkip() : 0);
      }
      arrayTime = array->GetMTime();
      ghostTime = ghosts ? ghosts->GetMTime() : 0;
    }

    range[0] = cachedRange[std::max(0, comp * 2)];
    range[1] = cachedRange[std::max(1, comp * 2 + 1)];
    return retVal;
  }

  constexpr double NaN = std::numeric_limits<double>::quiet_NaN();
  range[0] = NaN;
  range[1] = NaN;
  return false;
}
} // anonymous namespace

//------------------------------------------------------------------------------
void vtkFieldData::NullData(vtkIdType id)
{
  vtkFieldData::Iterator it(this);
  vtkDataArray* da;
  std::vector<double> tuple(32.0, 0.0);
  for (da = it.Begin(); !it.End(); da = it.Next())
  {
    if (da)
    {
      const size_t numComps = static_cast<size_t>(da->GetNumberOfComponents());
      if (numComps > tuple.size())
      {
        tuple.resize(numComps, 0.0);
      }
      da->InsertTuple(id, tuple.data());
    }
  }
}

//------------------------------------------------------------------------------
vtkFieldData::BasicIterator::BasicIterator(const int* list, unsigned int listSize)
{
  this->Position = 0;

  if (list)
  {
    if (listSize > 0)
    {
      this->List.assign(list, list + listSize);
    }
    else
    {
      this->List.clear();
    }
  }
  else
  {
    this->List.clear();
  }
}

//------------------------------------------------------------------------------
vtkFieldData::Iterator::Iterator(vtkFieldData* dsa, const int* list, unsigned int lSize)
  : vtkFieldData::BasicIterator(list, lSize)
{
  this->Fields = dsa;
  dsa->Register(nullptr);
  if (!list)
  {
    int listSize = dsa->GetNumberOfArrays();
    this->List.reserve(listSize);
    for (int i = 0; i < listSize; ++i)
    {
      this->List.push_back(i);
    }
  }
  this->Detached = 0;
}

//------------------------------------------------------------------------------
vtkFieldData::BasicIterator::BasicIterator(const vtkFieldData::BasicIterator& source)
{
  this->List = source.List;
}

//------------------------------------------------------------------------------
vtkFieldData::Iterator::Iterator(const vtkFieldData::Iterator& source)
  : vtkFieldData::BasicIterator(source)
{
  this->Detached = source.Detached;
  this->Fields = source.Fields;
  if (this->Fields && !this->Detached)
  {
    this->Fields->Register(nullptr);
  }
}

//------------------------------------------------------------------------------
void vtkFieldData::BasicIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "BasicIterator:{";
  size_t listSize = this->List.size();
  if (listSize > 0)
  {
    os << this->List[0];
    for (size_t i = 1; i < listSize; ++i)
    {
      os << ", " << this->List[i];
    }
  }
  os << "}" << endl;
}

//------------------------------------------------------------------------------
vtkFieldData::BasicIterator& vtkFieldData::BasicIterator::operator=(
  const vtkFieldData::BasicIterator& source)
{
  if (this == &source)
  {
    return *this;
  }

  this->List = source.List;

  return *this;
}

//------------------------------------------------------------------------------
vtkFieldData::Iterator& vtkFieldData::Iterator::operator=(const vtkFieldData::Iterator& source)
{
  if (this == &source)
  {
    return *this;
  }
  this->BasicIterator::operator=(source);
  if (this->Fields && !this->Detached)
  {
    this->Fields->UnRegister(nullptr);
  }
  this->Fields = source.Fields;
  this->Detached = source.Detached;
  if (this->Fields && !this->Detached)
  {
    this->Fields->Register(nullptr);
  }
  return *this;
}

//------------------------------------------------------------------------------
vtkFieldData::Iterator::~Iterator()
{
  if (this->Fields && !this->Detached)
  {
    this->Fields->UnRegister(nullptr);
  }
}

//------------------------------------------------------------------------------
void vtkFieldData::Iterator::DetachFieldData()
{
  if (this->Fields && !this->Detached)
  {
    this->Fields->UnRegister(nullptr);
    this->Detached = 1;
  }
}

//------------------------------------------------------------------------------
// Construct object with no data initially.
vtkFieldData::vtkFieldData()
{
  this->NumberOfArrays = 0;
  this->Data = nullptr;
  this->NumberOfActiveArrays = 0;

  this->CopyFieldFlags = nullptr;
  this->NumberOfFieldFlags = 0;

  this->DoCopyAllOn = 1;
  this->DoCopyAllOff = 0;

  this->GhostsToSkip = 0;
  this->GhostArray = nullptr;

  this->CopyAllOn();
}

//------------------------------------------------------------------------------
vtkFieldData::~vtkFieldData()
{
  this->Initialize();
  this->ClearFieldFlags();
}

//------------------------------------------------------------------------------
// Release all data but do not delete object.
void vtkFieldData::InitializeFields()
{
  if (this->Data)
  {
    for (int i = 0; i < this->GetNumberOfArrays(); ++i)
    {
      this->Data[i]->UnRegister(this);
    }

    delete[] this->Data;
    this->Data = nullptr;
  }

  this->GhostArray = nullptr;
  this->NumberOfArrays = 0;
  this->NumberOfActiveArrays = 0;
  this->Modified();
}

//------------------------------------------------------------------------------
// Release all data but do not delete object.
// Also initialize copy flags.
void vtkFieldData::Initialize()
{
  this->InitializeFields();
  this->CopyAllOn();
  this->ClearFieldFlags();
}

//------------------------------------------------------------------------------
void vtkFieldData::SetGhostsToSkip(unsigned char ghostsToSkip)
{
  if (this->GhostsToSkip != ghostsToSkip)
  {
    this->GhostsToSkip = ghostsToSkip;
    // We need to wipe the cached ranges.
    // We reset the MTime of the ghost array so the field data acts as if the ghost array was
    // changed.
    for (auto& components : this->Ranges)
    {
      for (CachedGhostRangeType& cache : components)
      {
        std::get<1>(cache) = 0;
      }
    }
    for (auto& components : this->FiniteRanges)
    {
      for (CachedGhostRangeType& cache : components)
      {
        std::get<1>(cache) = 0;
      }
    }
    this->Modified();
  }
}

//------------------------------------------------------------------------------
// Allocate data for each array.
vtkTypeBool vtkFieldData::Allocate(vtkIdType sz, vtkIdType ext)
{
  int status = 0;

  for (int i = 0; i < this->GetNumberOfArrays(); ++i)
  {
    if ((status = this->Data[i]->Allocate(sz, ext)) == 0)
    {
      break;
    }
  }

  return status;
}

//------------------------------------------------------------------------------
void vtkFieldData::CopyStructure(vtkFieldData* r)
{
  // Free old fields.
  this->InitializeFields();

  // Allocate new fields.
  this->AllocateArrays(r->GetNumberOfArrays());
  this->NumberOfActiveArrays = r->GetNumberOfArrays();

  // Copy the data array's structure (ie nTups,nComps,name, and info)
  // don't copy their data.
  for (int i = 0; i < r->GetNumberOfArrays(); ++i)
  {
    vtkAbstractArray* data = r->Data[i]->NewInstance();
    int numComponents = r->Data[i]->GetNumberOfComponents();
    data->SetNumberOfComponents(numComponents);
    data->SetName(r->Data[i]->GetName());
    for (vtkIdType j = 0; j < numComponents; ++j)
    {
      data->SetComponentName(j, r->Data[i]->GetComponentName(j));
    }
    if (r->Data[i]->HasInformation())
    {
      data->CopyInformation(r->Data[i]->GetInformation(), /*deep=*/1);
    }
    this->SetArray(i, data);
    data->Delete();
  }
}

//------------------------------------------------------------------------------
// Set the number of arrays used to define the field.
void vtkFieldData::AllocateArrays(int num)
{
  if (num < 0)
  {
    num = 0;
  }

  if (num == this->NumberOfArrays)
  {
    return;
  }

  if (num == 0)
  {
    this->Initialize();
  }
  else if (num < this->NumberOfArrays)
  {
    for (int i = num; i < this->NumberOfArrays; ++i)
    {
      if (this->Data[i])
      {
        this->Data[i]->UnRegister(this);
      }
    }
    this->NumberOfArrays = num;
  }
  else // num > this->NumberOfArrays
  {
    vtkAbstractArray** data = new vtkAbstractArray*[num];
    this->Ranges.resize(num);
    this->FiniteRanges.resize(num);
    // copy the original data
    for (int i = 0; i < this->NumberOfArrays; ++i)
    {
      data[i] = this->Data[i];
    }

    // initialize the new arrays
    for (int i = this->NumberOfArrays; i < num; ++i)
    {
      data[i] = nullptr;
    }

    // get rid of the old data
    delete[] this->Data;

    // update object
    this->Data = data;
    this->NumberOfArrays = num;
  }
  this->Modified();
}

//------------------------------------------------------------------------------
// Set an array to define the field.
void vtkFieldData::SetArray(int i, vtkAbstractArray* data)
{
  if (!data || (i > this->NumberOfActiveArrays))
  {
    vtkWarningMacro("Can not set array " << i << " to " << data << endl);
    return;
  }

  if (i < 0)
  {
    vtkWarningMacro("Array index should be >= 0");
    return;
  }
  else if (i >= this->NumberOfArrays)
  {
    this->AllocateArrays(i + 1);
    this->NumberOfActiveArrays = i + 1;
  }

  const char* name = data->GetName();
  if (name && strcmp(name, vtkDataSetAttributes::GhostArrayName()) == 0)
  {
    this->GhostArray = vtkArrayDownCast<vtkUnsignedCharArray>(data);
  }

  if (this->Data[i] != data)
  {
    if (this->Data[i] != nullptr)
    {
      this->Data[i]->UnRegister(this);
    }
    this->Data[i] = data;
    if (this->Data[i] != nullptr)
    {
      // range[0] -> cached range for comp == -1
      // range[1] -> cached range for comp in [0, number of components - 1]
      // std::get<0> -> cached array MTime
      // std::get<1> -> cached ghost array MTime
      // std::get<2> -> cached range buffer
      auto& finiteRange = this->FiniteRanges[i];
      std::get<0>(finiteRange[0]) = 0;
      std::get<1>(finiteRange[0]) = 0;
      std::get<2>(finiteRange[0]).resize(2);
      std::get<0>(finiteRange[1]) = 0;
      std::get<1>(finiteRange[1]) = 0;
      std::get<2>(finiteRange[1]).resize(2 * data->GetNumberOfComponents());
      auto& range = this->Ranges[i];
      std::get<0>(range[0]) = 0;
      std::get<1>(range[0]) = 0;
      std::get<2>(range[0]).resize(2);
      std::get<0>(range[1]) = 0;
      std::get<1>(range[1]) = 0;
      std::get<2>(range[1]).resize(2 * data->GetNumberOfComponents());
      this->Data[i]->Register(this);
    }
    this->Modified();
  }
}

//------------------------------------------------------------------------------
// Return the ith array in the field. A nullptr is returned if the index i is out
// of range.
vtkDataArray* vtkFieldData::GetArray(int i)
{
  return vtkArrayDownCast<vtkDataArray>(this->GetAbstractArray(i));
}

//------------------------------------------------------------------------------
// Return the ith array in the field. A nullptr is returned if the index i is out
// of range.
vtkAbstractArray* vtkFieldData::GetAbstractArray(int i)
{
  if (i < 0 || i >= this->GetNumberOfArrays() || this->Data == nullptr)
  {
    return nullptr;
  }
  return this->Data[i];
}

//------------------------------------------------------------------------------
// Copy a field by creating new data arrays
void vtkFieldData::DeepCopy(vtkFieldData* f)
{
  this->SetGhostsToSkip(this->GetGhostsToSkip());

  this->AllocateArrays(f->GetNumberOfArrays());
  for (int i = 0; i < f->GetNumberOfArrays(); ++i)
  {
    vtkAbstractArray* data = f->GetAbstractArray(i);
    vtkAbstractArray* newData = data->NewInstance(); // instantiate same type of object
    newData->DeepCopy(data);
    newData->SetName(data->GetName());
    if (data->HasInformation())
    {
      newData->CopyInformation(data->GetInformation(), /*deep=*/1);
    }
    this->AddArray(newData);
    newData->Delete();
  }
}

//------------------------------------------------------------------------------
// Copy a field by reference counting the data arrays.
void vtkFieldData::ShallowCopy(vtkFieldData* f)
{
  this->AllocateArrays(f->GetNumberOfArrays());
  this->NumberOfActiveArrays = 0;

  this->GhostsToSkip = f->GetGhostsToSkip();
  this->GhostArray = f->GetGhostArray();

  for (int i = 0; i < f->GetNumberOfArrays(); ++i)
  {
    this->NumberOfActiveArrays++;
    this->SetArray(i, f->GetAbstractArray(i));
  }
  this->CopyFlags(f);
}

//------------------------------------------------------------------------------
// Squeezes each data array in the field (Squeeze() reclaims unused memory.)
void vtkFieldData::Squeeze()
{
  for (int i = 0; i < this->GetNumberOfArrays(); ++i)
  {
    this->Data[i]->Squeeze();
  }
}

//------------------------------------------------------------------------------
// Resets each data array in the field (Reset() does not release memory but
// it makes the arrays look like they are empty.)
void vtkFieldData::Reset()
{
  for (int i = 0; i < this->GetNumberOfArrays(); ++i)
  {
    this->Data[i]->Reset();
  }
}

//------------------------------------------------------------------------------
// Get a field from a list of ids. Supplied field f should have same
// types and number of data arrays as this one (i.e., like
// CopyStructure() creates).
void vtkFieldData::GetField(vtkIdList* ptIds, vtkFieldData* f)
{
  int numIds = ptIds->GetNumberOfIds();

  for (int i = 0; i < numIds; ++i)
  {
    f->InsertTuple(i, ptIds->GetId(i), this);
  }
}

//------------------------------------------------------------------------------
// Return the array containing the ith component of the field. The return value
// is an integer number n 0<=n<this->NumberOfArrays. Also, an integer value is
// returned indicating the component in the array is returned. Method returns
// -1 if specified component is not in field.
int vtkFieldData::GetArrayContainingComponent(int i, int& arrayComp)
{
  int count = 0;

  for (int j = 0; j < this->GetNumberOfArrays(); ++j)
  {
    if (this->Data[j] != nullptr)
    {
      int numComp = this->Data[j]->GetNumberOfComponents();
      if (i < (numComp + count))
      {
        arrayComp = i - count;
        return j;
      }
      count += numComp;
    }
  }
  return -1;
}

//------------------------------------------------------------------------------
vtkDataArray* vtkFieldData::GetArray(const char* arrayName, int& index)
{
  int i;
  vtkDataArray* da = vtkArrayDownCast<vtkDataArray>(this->GetAbstractArray(arrayName, i));
  index = (da) ? i : -1;
  return da;
}

//------------------------------------------------------------------------------
vtkAbstractArray* vtkFieldData::GetAbstractArray(const char* arrayName, int& index)
{
  index = -1;
  if (!arrayName)
  {
    return nullptr;
  }
  for (int i = 0; i < this->GetNumberOfArrays(); ++i)
  {
    const char* name = this->GetArrayName(i);
    if (name && (strcmp(name, arrayName) == 0))
    {
      index = i;
      return this->GetAbstractArray(i);
    }
  }
  return nullptr;
}

//------------------------------------------------------------------------------
int vtkFieldData::AddArray(vtkAbstractArray* array)
{
  if (!array)
  {
    return -1;
  }

  int index;
  this->GetAbstractArray(array->GetName(), index);

  if (index == -1)
  {
    index = this->NumberOfActiveArrays;
    this->NumberOfActiveArrays++;
  }
  this->SetArray(index, array);
  return index;
}

//------------------------------------------------------------------------------
bool vtkFieldData::GetRange(const char* name, double range[2], int comp)
{
  int index;
  this->GetAbstractArray(name, index);
  if (index == -1)
  {
    constexpr double NaN = std::numeric_limits<double>::quiet_NaN();
    range[0] = NaN;
    range[1] = NaN;
    return false;
  }
  return this->GetRange(index, range, comp);
}

//------------------------------------------------------------------------------
bool vtkFieldData::GetRange(int index, double range[2], int comp)
{
  return ::GetRangeImpl(this, index, range, comp, this->Ranges, &vtkDataArray::ComputeVectorRange,
    &vtkDataArray::ComputeScalarRange);
}

//------------------------------------------------------------------------------
bool vtkFieldData::GetFiniteRange(const char* name, double range[2], int comp)
{
  int index;
  this->GetAbstractArray(name, index);
  if (index == -1)
  {
    constexpr double NaN = std::numeric_limits<double>::quiet_NaN();
    range[0] = NaN;
    range[1] = NaN;
    return false;
  }
  return this->GetFiniteRange(index, range, comp);
}

//------------------------------------------------------------------------------
bool vtkFieldData::GetFiniteRange(int index, double range[2], int comp)
{
  return ::GetRangeImpl(this, index, range, comp, this->FiniteRanges,
    &vtkDataArray::ComputeFiniteVectorRange, &vtkDataArray::ComputeFiniteScalarRange);
}

//------------------------------------------------------------------------------
void vtkFieldData::RemoveArray(const char* name)
{
  int i;
  this->GetAbstractArray(name, i);
  this->RemoveArray(i);
}

//------------------------------------------------------------------------------
void vtkFieldData::RemoveArray(int index)
{
  if ((index < 0) || (index >= this->NumberOfActiveArrays))
  {
    return;
  }
  if (this->Data[index] == this->GhostArray)
  {
    this->GhostArray = nullptr;
  }
  this->Data[index]->UnRegister(this);
  this->Data[index] = nullptr;
  this->NumberOfActiveArrays--;
  for (int i = index; i < this->NumberOfActiveArrays; ++i)
  {
    this->Data[i] = this->Data[i + 1];
    this->Ranges[i] = std::move(this->Ranges[i + 1]);
    this->FiniteRanges[i] = std::move(this->FiniteRanges[i + 1]);
  }
  this->Ranges[this->NumberOfActiveArrays] = std::array<CachedGhostRangeType, 2>();
  this->FiniteRanges[this->NumberOfActiveArrays] = std::array<CachedGhostRangeType, 2>();
  this->Data[this->NumberOfActiveArrays] = nullptr;
  this->Modified();
}

namespace
{
//------------------------------------------------------------------------------
struct IsAnyBitSetFunctor
{
  unsigned char* BitSet;
  int BitFlag;
  int IsAnyBit;
  vtkSMPThreadLocal<unsigned char> TLIsAnyBit;

  IsAnyBitSetFunctor(vtkUnsignedCharArray* bitSet, int bitFlag)
    : BitSet(bitSet->GetPointer(0))
    , BitFlag(bitFlag)
  {
  }

  void Initialize() { this->TLIsAnyBit.Local() = 0; }

  void operator()(vtkIdType begin, vtkIdType end)
  {
    if (this->TLIsAnyBit.Local())
    {
      return;
    }
    for (vtkIdType i = begin; i < end; ++i)
    {
      if (this->BitSet[i] & this->BitFlag)
      {
        this->TLIsAnyBit.Local() = 1;
        return;
      }
    }
  }

  void Reduce()
  {
    this->IsAnyBit = 0;
    for (auto& isAnyBit : this->TLIsAnyBit)
    {
      if (isAnyBit)
      {
        this->IsAnyBit = 1;
        break;
      }
    }
  }
};
} // anonymous namespace

//------------------------------------------------------------------------------
bool vtkFieldData::HasAnyGhostBitSet(int bitFlag)
{
  if (this->GhostArray)
  {
    IsAnyBitSetFunctor isAnyBitSetFunctor(this->GhostArray, bitFlag);
    vtkSMPTools::For(0, this->GhostArray->GetNumberOfValues(), isAnyBitSetFunctor);
    return isAnyBitSetFunctor.IsAnyBit;
  }
  return false;
}

//------------------------------------------------------------------------------
unsigned long vtkFieldData::GetActualMemorySize()
{
  unsigned long size = 0;

  for (int i = 0; i < this->GetNumberOfArrays(); ++i)
  {
    if (this->Data[i] != nullptr)
    {
      size += this->Data[i]->GetActualMemorySize();
    }
  }

  return size;
}

//------------------------------------------------------------------------------
vtkMTimeType vtkFieldData::GetMTime()
{
  vtkMTimeType mTime = this->MTime;

  for (int i = 0; i < this->NumberOfActiveArrays; ++i)
  {
    vtkAbstractArray* aa = this->Data[i];
    if (aa)
    {
      vtkMTimeType otherMTime = aa->GetMTime();
      if (otherMTime > mTime)
      {
        mTime = otherMTime;
      }
    }
  }

  return mTime;
}

//------------------------------------------------------------------------------
void vtkFieldData::CopyFieldOnOff(const char* field, int onOff)
{
  if (!field)
  {
    return;
  }

  // If the array is in the list, simply set IsCopied to onOff
  int index = this->FindFlag(field);
  if (index != -1)
  {
    if (this->CopyFieldFlags[index].IsCopied != onOff)
    {
      this->CopyFieldFlags[index].IsCopied = onOff;
      this->Modified();
    }
  }
  else
  {
    // We need to reallocate the list of fields
    vtkFieldData::CopyFieldFlag* newFlags =
      new vtkFieldData::CopyFieldFlag[this->NumberOfFieldFlags + 1];
    // Copy old flags (pointer copy for name)
    for (int i = 0; i < this->NumberOfFieldFlags; ++i)
    {
      newFlags[i].ArrayName = this->CopyFieldFlags[i].ArrayName;
      newFlags[i].IsCopied = this->CopyFieldFlags[i].IsCopied;
    }
    // Copy new flag (strcpy)
    char* newName = new char[strlen(field) + 1];
    strcpy(newName, field);
    newFlags[this->NumberOfFieldFlags].ArrayName = newName;
    newFlags[this->NumberOfFieldFlags].IsCopied = onOff;
    this->NumberOfFieldFlags++;
    delete[] this->CopyFieldFlags;
    this->CopyFieldFlags = newFlags;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
// Turn on copying of all data.
void vtkFieldData::CopyAllOn(int vtkNotUsed(ctype))
{
  if (!DoCopyAllOn || DoCopyAllOff)
  {
    this->DoCopyAllOn = 1;
    this->DoCopyAllOff = 0;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
// Turn off copying of all data.
void vtkFieldData::CopyAllOff(int vtkNotUsed(ctype))
{
  if (DoCopyAllOn || !DoCopyAllOff)
  {
    this->DoCopyAllOn = 0;
    this->DoCopyAllOff = 1;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
// Deallocate and clear the list of fields.
void vtkFieldData::ClearFieldFlags()
{
  if (this->NumberOfFieldFlags > 0)
  {
    for (int i = 0; i < this->NumberOfFieldFlags; ++i)
    {
      delete[] this->CopyFieldFlags[i].ArrayName;
    }
  }
  delete[] this->CopyFieldFlags;
  this->CopyFieldFlags = nullptr;
  this->NumberOfFieldFlags = 0;
}

//------------------------------------------------------------------------------
// Find if field is in CopyFieldFlags.
// If it is, it returns the index otherwise it returns -1
int vtkFieldData::FindFlag(const char* field)
{
  if (!field)
    return -1;
  for (int i = 0; i < this->NumberOfFieldFlags; ++i)
  {
    if (this->CopyFieldFlags[i].ArrayName &&
      (strcmp(field, this->CopyFieldFlags[i].ArrayName) == 0))
    {
      return i;
    }
  }
  return -1;
}

//------------------------------------------------------------------------------
// If there is no flag for this array, return -1.
// If there is one: return 0 if off, 1 if on
int vtkFieldData::GetFlag(const char* field)
{
  int index = this->FindFlag(field);
  if (index == -1)
  {
    return -1;
  }
  else
  {
    return this->CopyFieldFlags[index].IsCopied;
  }
}

//------------------------------------------------------------------------------
// Copy the fields list (with strcpy)
void vtkFieldData::CopyFlags(const vtkFieldData* source)
{
  this->ClearFieldFlags();
  this->NumberOfFieldFlags = source->NumberOfFieldFlags;
  if (this->NumberOfFieldFlags > 0)
  {
    this->CopyFieldFlags = new vtkFieldData::CopyFieldFlag[this->NumberOfFieldFlags];
    for (int i = 0; i < this->NumberOfFieldFlags; ++i)
    {
      this->CopyFieldFlags[i].ArrayName = new char[strlen(source->CopyFieldFlags[i].ArrayName) + 1];
      strcpy(this->CopyFieldFlags[i].ArrayName, source->CopyFieldFlags[i].ArrayName);
    }
  }
  else
  {
    this->CopyFieldFlags = nullptr;
  }
}

//------------------------------------------------------------------------------
void vtkFieldData::PassData(vtkFieldData* fd)
{
  for (int i = 0; i < fd->GetNumberOfArrays(); ++i)
  {
    const char* arrayName = fd->GetArrayName(i);
    // If there is no blocker for the given array
    // and both CopyAllOff and CopyOn for that array are not true
    if ((this->GetFlag(arrayName) != 0) &&
      !(this->DoCopyAllOff && (this->GetFlag(arrayName) != 1)) && fd->GetAbstractArray(i))
    {
      this->AddArray(fd->GetAbstractArray(i));
    }
  }
}

//------------------------------------------------------------------------------
void vtkFieldData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Number Of Arrays: " << this->GetNumberOfArrays() << "\n";
  for (int i = 0; i < this->GetNumberOfArrays(); ++i)
  {
    if (this->GetArrayName(i))
    {
      os << indent << "Array " << i << " name = " << this->GetArrayName(i) << "\n";
    }
    else
    {
      os << indent << "Array " << i << " name = nullptr\n";
    }
  }
  os << indent << "Number Of Components: " << this->GetNumberOfComponents() << "\n";
  os << indent << "Number Of Tuples: " << this->GetNumberOfTuples() << "\n";
}

//------------------------------------------------------------------------------
// Get the number of components in the field. This is determined by adding
// up the components in each non-nullptr array.
int vtkFieldData::GetNumberOfComponents()
{
  int numComp = 0;

  for (int i = 0; i < this->GetNumberOfArrays(); ++i)
  {
    if (this->Data[i])
    {
      numComp += this->Data[i]->GetNumberOfComponents();
    }
  }

  return numComp;
}

//------------------------------------------------------------------------------
// Get the number of tuples in the field.
vtkIdType vtkFieldData::GetNumberOfTuples()
{
  vtkAbstractArray* da = this->GetAbstractArray(0);
  if (da)
  {
    return da->GetNumberOfTuples();
  }
  else
  {
    return 0;
  }
}

//------------------------------------------------------------------------------
// Set the number of tuples for each data array in the field.
void vtkFieldData::SetNumberOfTuples(vtkIdType number)
{
  for (int i = 0; i < this->GetNumberOfArrays(); ++i)
  {
    this->Data[i]->SetNumberOfTuples(number);
  }
}

//------------------------------------------------------------------------------
// Set the jth tuple in source field data at the ith location.
// Set operations
// means that no range checking is performed, so they're faster.
void vtkFieldData::SetTuple(vtkIdType i, vtkIdType j, vtkFieldData* source)
{
  for (int k = 0; k < this->GetNumberOfArrays(); ++k)
  {
    this->Data[k]->SetTuple(i, j, source->Data[k]);
  }
}

//------------------------------------------------------------------------------
// Insert the tuple value at the ith location. Range checking is
// performed and memory allocates as necessary.
void vtkFieldData::InsertTuple(vtkIdType i, vtkIdType j, vtkFieldData* source)
{
  for (int k = 0; k < this->GetNumberOfArrays(); ++k)
  {
    this->Data[k]->InsertTuple(i, j, source->GetAbstractArray(k));
  }
}

//------------------------------------------------------------------------------
// Insert the tuple value at the end of the tuple matrix. Range
// checking is performed and memory is allocated as necessary.
vtkIdType vtkFieldData::InsertNextTuple(vtkIdType j, vtkFieldData* source)
{
  vtkIdType id = this->GetNumberOfTuples();
  this->InsertTuple(id, j, source);
  return id;
}
VTK_ABI_NAMESPACE_END
