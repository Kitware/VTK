// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkMultiProcessStream.h"

#include "vtkArrayDispatch.h"
#include "vtkBitArray.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkEndian.h"
#include "vtkObjectFactory.h"
#include "vtkSocketCommunicator.h" // for vtkSwap8 and vtkSwap4 macros.
#include "vtkStringArray.h"
#include "vtkTypeTraits.h"

#include <cassert>
#include <deque>

VTK_ABI_NAMESPACE_BEGIN
class vtkMultiProcessStream::vtkInternals
{
public:
  typedef std::deque<unsigned char> DataType;
  DataType Data;

  void Push(const unsigned char* data, size_t length)
  {
    for (size_t cc = 0; cc < length; cc++)
    {
      this->Data.push_back(data[cc]);
    }
  }

  void Pop(unsigned char* data, size_t length)
  {
    for (size_t cc = 0; cc < length; cc++)
    {
      data[cc] = this->Data.front();
      this->Data.pop_front();
    }
  }

  void SwapBytes()
  {
    DataType::iterator iter = this->Data.begin();
    while (iter != this->Data.end())
    {
      unsigned char type = *iter;
      int wordSize = 1;
      ++iter;
      switch (type)
      {
        case VTK_CHAR:
        case VTK_SIGNED_CHAR:
        case VTK_UNSIGNED_CHAR:
          wordSize = sizeof(signed char);
          break;
        case VTK_SHORT:
        case VTK_UNSIGNED_SHORT:
          wordSize = sizeof(short);
          break;
        case VTK_INT:
        case VTK_UNSIGNED_INT:
          wordSize = sizeof(int);
          break;
        case VTK_LONG:
        case VTK_UNSIGNED_LONG:
          wordSize = sizeof(long);
          break;
        case VTK_LONG_LONG:
        case VTK_UNSIGNED_LONG_LONG:
          wordSize = sizeof(long long);
          break;
        case VTK_FLOAT:
          wordSize = sizeof(float);
          break;
        case VTK_DOUBLE:
          wordSize = sizeof(double);
          break;
        case VTK_OBJECT:
          wordSize = sizeof(int);
          break;
        case VTK_STRING:
          // We want to bitswap the string size which is an int
          wordSize = sizeof(int);
          break;
      }

      switch (wordSize)
      {
        case 1:
          break;
        case 4:
          vtkSwap4(&(*iter));
          break;
        case 8:
          vtkSwap8(&(*iter));
          break;
      }

      // In case of string we don't need to swap char values
      int nbSkip = 0;
      if (type == VTK_STRING || type == VTK_OBJECT)
      {
        nbSkip = *reinterpret_cast<int*>(&*iter);
      }

      while (wordSize > 0)
      {
        ++iter;
        wordSize--;
      }

      // Skip String chars
      for (int cc = 0; cc < nbSkip; cc++)
      {
        ++iter;
      }
    }
  }
};

//------------------------------------------------------------------------------
vtkMultiProcessStream::vtkMultiProcessStream()
{
  this->Internals = new vtkMultiProcessStream::vtkInternals();
#ifdef VTK_WORDS_BIGENDIAN
  this->Endianness = vtkMultiProcessStream::BigEndian;
#else
  this->Endianness = vtkMultiProcessStream::LittleEndian;
#endif
}

//------------------------------------------------------------------------------
vtkMultiProcessStream::~vtkMultiProcessStream()
{
  delete this->Internals;
  this->Internals = nullptr;
}

//------------------------------------------------------------------------------
vtkMultiProcessStream::vtkMultiProcessStream(const vtkMultiProcessStream& other)
{
  this->Internals = new vtkMultiProcessStream::vtkInternals();
  this->Internals->Data = other.Internals->Data;
  this->Endianness = other.Endianness;
}

//------------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator=(const vtkMultiProcessStream& other)
{
  if (this == &other)
  {
    return *this;
  }

  this->Internals->Data = other.Internals->Data;
  this->Endianness = other.Endianness;
  return (*this);
}

//------------------------------------------------------------------------------
void vtkMultiProcessStream::Reset()
{
  this->Internals->Data.clear();
}

//------------------------------------------------------------------------------
int vtkMultiProcessStream::Size()
{
  return (static_cast<int>(this->Internals->Data.size()));
}

//------------------------------------------------------------------------------
bool vtkMultiProcessStream::Empty()
{
  return (this->Internals->Data.empty());
}

//------------------------------------------------------------------------------
template <typename T>
void vtkMultiProcessStream::PushArray(T array[], unsigned int size)
{
  assert("pre: array is nullptr!" && (array != nullptr));
  this->Internals->Data.push_back(vtkTypeTraits<T>::VTKTypeID());
  this->Internals->Push(reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int));
  this->Internals->Push(reinterpret_cast<unsigned char*>(array), sizeof(T) * size);
}

//------------------------------------------------------------------------------
void vtkMultiProcessStream::Push(char array[], unsigned int size)
{
  this->PushArray(array, size);
}

//------------------------------------------------------------------------------
void vtkMultiProcessStream::Push(signed char array[], unsigned int size)
{
  this->PushArray(array, size);
}

//------------------------------------------------------------------------------
void vtkMultiProcessStream::Push(unsigned char array[], unsigned int size)
{
  this->PushArray(array, size);
}

//------------------------------------------------------------------------------
void vtkMultiProcessStream::Push(short array[], unsigned int size)
{
  this->PushArray(array, size);
}

//------------------------------------------------------------------------------
void vtkMultiProcessStream::Push(unsigned short array[], unsigned int size)
{
  this->PushArray(array, size);
}

//------------------------------------------------------------------------------
void vtkMultiProcessStream::Push(int array[], unsigned int size)
{
  this->PushArray(array, size);
}

//------------------------------------------------------------------------------
void vtkMultiProcessStream::Push(unsigned int array[], unsigned int size)
{
  this->PushArray(array, size);
}

//------------------------------------------------------------------------------
void vtkMultiProcessStream::Push(long array[], unsigned int size)
{
  this->PushArray(array, size);
}

//------------------------------------------------------------------------------
void vtkMultiProcessStream::Push(unsigned long array[], unsigned int size)
{
  this->PushArray(array, size);
}

//------------------------------------------------------------------------------
void vtkMultiProcessStream::Push(long long array[], unsigned int size)
{
  this->PushArray(array, size);
}

//------------------------------------------------------------------------------
void vtkMultiProcessStream::Push(unsigned long long array[], unsigned int size)
{
  this->PushArray(array, size);
}

//------------------------------------------------------------------------------
void vtkMultiProcessStream::Push(float array[], unsigned int size)
{
  this->PushArray(array, size);
}

//------------------------------------------------------------------------------
void vtkMultiProcessStream::Push(double array[], unsigned int size)
{
  this->PushArray(array, size);
}

//------------------------------------------------------------------------------
struct vtkMultiProcessStreamPushArray
{
  template <typename ValueType>
  void operator()(vtkAOSDataArrayTemplate<ValueType>* array, vtkMultiProcessStream* stream)
  {
    stream->operator<<(array->GetDataType());
    stream->Push(array->GetPointer(0), static_cast<unsigned int>(array->GetNumberOfValues()));
  }
  void operator()(vtkBitArray* array, vtkMultiProcessStream* stream)
  {
    stream->operator<<(array->GetDataType());
    stream->Push(array->GetPointer(0), static_cast<unsigned int>(array->GetNumberOfValues()));
  }
  template <class DerivedType, typename ValueType, int ArrayType>
  void operator()(
    vtkGenericDataArray<DerivedType, ValueType, ArrayType>* array, vtkMultiProcessStream* stream)
  {
    stream->operator<<(array->GetDataType());
    stream->operator<<(static_cast<unsigned int>(array->GetNumberOfValues()));
    auto range = vtk::DataArrayTupleRange(array);
    for (auto tuple : range)
    {
      for (ValueType comp : tuple)
      {
        stream->Internals->Push(reinterpret_cast<unsigned char*>(&comp), sizeof(ValueType));
      }
    }
  }
  template <typename ValueType>
  void operator()(vtkDataArray* array, ValueType vtkNotUsed(value), vtkMultiProcessStream* stream)
  {
    stream->operator<<(array->GetDataType());
    stream->operator<<(static_cast<unsigned int>(array->GetNumberOfValues()));
    auto range = vtk::DataArrayTupleRange(array);
    for (auto tuple : range)
    {
      for (auto comp : tuple)
      {
        ValueType value = static_cast<ValueType>(comp);
        stream->Internals->Push(reinterpret_cast<unsigned char*>(&value), sizeof(ValueType));
      }
    }
  }
  void operator()(vtkStringArray* array, vtkMultiProcessStream* stream)
  {
    stream->operator<<(array->GetDataType());
    stream->operator<<(static_cast<unsigned int>(array->GetNumberOfValues()));
    for (vtkIdType i = 0; i < array->GetNumberOfValues(); ++i)
    {
      const auto& value = array->GetValue(i);
      // Find the real string size
      int size = static_cast<int>(value.size());
      // Set the string size
      stream->Internals->Push(reinterpret_cast<unsigned char*>(&size), sizeof(int));
      // Set the string content
      for (int idx = 0; idx < size; idx++)
      {
        stream->Internals->Push(reinterpret_cast<const unsigned char*>(&value[idx]), sizeof(char));
      }
    }
  }
};

//------------------------------------------------------------------------------
void vtkMultiProcessStream::Push(vtkDataArray* array)
{
  assert("pre: array is nullptr!" && (array != nullptr));
  this->operator<<(std::string(array->GetName() ? array->GetName() : ""));
  this->operator<<(array->GetNumberOfComponents());
  this->operator<<(array->GetNumberOfTuples());
  using AllArrays = vtkTypeList::Append<vtkArrayDispatch::Arrays, vtkBitArray>::Result;
  vtkMultiProcessStreamPushArray functor;
  if (!vtkArrayDispatch::DispatchByArray<AllArrays>::Execute(array, functor, this))
  {
    switch (array->GetDataType())
    {
      vtkTemplateMacro(functor(array, VTK_TT(0), this));
      default:
        vtkGenericWarningMacro("Push: Unknown data type " << array->GetDataType());
        break;
    }
  }
}

//------------------------------------------------------------------------------
void vtkMultiProcessStream::Push(vtkStringArray* array)
{
  assert("pre: array is nullptr!" && (array != nullptr));
  this->operator<<(std::string(array->GetName() ? array->GetName() : ""));
  this->operator<<(array->GetNumberOfComponents());
  this->operator<<(array->GetNumberOfTuples());
  this->operator<<(array->GetDataType());
  vtkMultiProcessStreamPushArray functor;
  functor(array, this);
}

//------------------------------------------------------------------------------
template <typename T>
void vtkMultiProcessStream::PopArray(T*& array, unsigned int& size)
{
  assert("pre: stream data must be the right type" &&
    this->Internals->Data.front() == vtkTypeTraits<T>::VTKTypeID());
  this->Internals->Data.pop_front();

  if (array == nullptr)
  {
    // Get the size of the array
    this->Internals->Pop(reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int));

    // Allocate array
    array = new T[size];
    assert("ERROR: cannot allocate array" && (array != nullptr));
  }
  else
  {
    unsigned int sz;

    // Get the size of the array
    this->Internals->Pop(reinterpret_cast<unsigned char*>(&sz), sizeof(unsigned int));
    assert("ERROR: input array size does not match size of data" && (sz == size));
  }

  // Pop the array data
  this->Internals->Pop(reinterpret_cast<unsigned char*>(array), sizeof(T) * size);
}

//------------------------------------------------------------------------------
void vtkMultiProcessStream::Pop(char*& array, unsigned int& size)
{
  this->PopArray(array, size);
}

//------------------------------------------------------------------------------
void vtkMultiProcessStream::Pop(signed char*& array, unsigned int& size)
{
  this->PopArray(array, size);
}

//------------------------------------------------------------------------------
void vtkMultiProcessStream::Pop(unsigned char*& array, unsigned int& size)
{
  this->PopArray(array, size);
}

//------------------------------------------------------------------------------
void vtkMultiProcessStream::Pop(short*& array, unsigned int& size)
{
  this->PopArray(array, size);
}

//------------------------------------------------------------------------------
void vtkMultiProcessStream::Pop(unsigned short*& array, unsigned int& size)
{
  this->PopArray(array, size);
}

//------------------------------------------------------------------------------
void vtkMultiProcessStream::Pop(int*& array, unsigned int& size)
{
  this->PopArray(array, size);
}

//------------------------------------------------------------------------------
void vtkMultiProcessStream::Pop(unsigned int*& array, unsigned int& size)
{
  this->PopArray(array, size);
}

//------------------------------------------------------------------------------
void vtkMultiProcessStream::Pop(long*& array, unsigned int& size)
{
  this->PopArray(array, size);
}

//------------------------------------------------------------------------------
void vtkMultiProcessStream::Pop(unsigned long*& array, unsigned int& size)
{
  this->PopArray(array, size);
}

//------------------------------------------------------------------------------
void vtkMultiProcessStream::Pop(long long*& array, unsigned int& size)
{
  this->PopArray(array, size);
}

//------------------------------------------------------------------------------
void vtkMultiProcessStream::Pop(unsigned long long*& array, unsigned int& size)
{
  this->PopArray(array, size);
}

//------------------------------------------------------------------------------
void vtkMultiProcessStream::Pop(float*& array, unsigned int& size)
{
  this->PopArray(array, size);
}

//------------------------------------------------------------------------------
void vtkMultiProcessStream::Pop(double*& array, unsigned int& size)
{
  this->PopArray(array, size);
}

struct vtkMultiProcessStreamPopArray
{
  template <typename ValueType>
  void operator()(vtkAOSDataArrayTemplate<ValueType>* array, vtkMultiProcessStream* stream)
  {
    unsigned int size = static_cast<unsigned int>(array->GetNumberOfValues());
    ValueType* ptr = array->GetPointer(0);
    stream->Pop(ptr, size);
  }
  void operator()(vtkBitArray* array, vtkMultiProcessStream* stream)
  {
    unsigned int size = static_cast<unsigned int>(array->GetNumberOfValues());
    unsigned char* ptr = array->GetPointer(0);
    stream->Pop(ptr, size);
  }
  template <class DerivedType, typename ValueType, int ArrayType>
  void operator()(
    vtkGenericDataArray<DerivedType, ValueType, ArrayType>* array, vtkMultiProcessStream* stream)
  {
    unsigned int size;
    stream->operator>>(size);
    auto range = vtk::DataArrayTupleRange(array);
    for (auto tuple : range)
    {
      for (auto comp : tuple)
      {
        ValueType value;
        stream->Internals->Pop(reinterpret_cast<unsigned char*>(&value), sizeof(ValueType));
        comp = value;
      }
    }
  }
  template <typename ValueType>
  void operator()(vtkDataArray* array, ValueType vtkNotUsed(value), vtkMultiProcessStream* stream)
  {
    unsigned int size;
    stream->operator>>(size);
    auto range = vtk::DataArrayTupleRange(array);
    for (auto tuple : range)
    {
      for (auto comp : tuple)
      {
        ValueType value;
        stream->Internals->Pop(reinterpret_cast<unsigned char*>(&value), sizeof(ValueType));
        comp = value;
      }
    }
  }
  void operator()(vtkStringArray* array, vtkMultiProcessStream* stream)
  {
    vtkIdType numValues;
    stream->operator>>(numValues);
    for (vtkIdType i = 0; i < numValues; ++i)
    {
      auto& value = array->GetValue(i);
      value = "";
      int stringSize;
      stream->Internals->Pop(reinterpret_cast<unsigned char*>(&stringSize), sizeof(int));
      char c_value;
      for (int idx = 0; idx < stringSize; idx++)
      {
        stream->Internals->Pop(reinterpret_cast<unsigned char*>(&c_value), sizeof(char));
        value += c_value;
      }
    }
  }
};

//------------------------------------------------------------------------------
void vtkMultiProcessStream::Pop(vtkDataArray*& array)
{
  std::string name;
  int numComp, dataType;
  vtkIdType numTuples;
  this->operator>>(name);
  this->operator>>(numComp);
  this->operator>>(numTuples);
  this->operator>>(dataType);
  if (!array)
  {
    array = vtkDataArray::CreateDataArray(dataType);
    array->SetName(name.c_str());
    array->SetNumberOfComponents(numComp);
    array->SetNumberOfTuples(numTuples);
  }
  else
  {
    assert("pre: input array has wrong name" && name == array->GetName());
    assert("pre: input array has wrong number of components" &&
      numComp == array->GetNumberOfComponents());
    assert(
      "pre: input array has wrong number of tuples" && numTuples == array->GetNumberOfTuples());
    assert("pre: input array has wrong data type" && dataType == array->GetDataType());
  }

  vtkMultiProcessStreamPopArray functor;
  using AllArrays = vtkTypeList::Append<vtkArrayDispatch::Arrays, vtkBitArray>::Result;
  if (!vtkArrayDispatch::DispatchByArray<AllArrays>::Execute(array, functor, this))
  {
    switch (array->GetDataType())
    {
      vtkTemplateMacro(functor(array, VTK_TT(0), this));
      default:
        vtkGenericWarningMacro("Push: Unknown data type " << array->GetDataType());
        break;
    }
  }
}

//------------------------------------------------------------------------------
void vtkMultiProcessStream::Pop(vtkStringArray*& array)
{
  std::string name;
  int numComp, dataType;
  vtkIdType numTuples;
  this->operator>>(name);
  this->operator>>(numComp);
  this->operator>>(numTuples);
  this->operator>>(dataType);
  if (!array)
  {
    array = vtkStringArray::New();
    array->SetName(name.c_str());
    array->SetNumberOfComponents(numComp);
    array->SetNumberOfTuples(numTuples);
  }
  else
  {
    assert("pre: input array has wrong name" && name == array->GetName());
    assert("pre: input array has wrong number of components" &&
      numComp == array->GetNumberOfComponents());
    assert(
      "pre: input array has wrong number of tuples" && numTuples == array->GetNumberOfTuples());
    assert("pre: input array has wrong data type" && dataType == array->GetDataType());
  }
  vtkMultiProcessStreamPopArray functor;
  functor(array, this);
}

//------------------------------------------------------------------------------
template <typename T>
vtkMultiProcessStream& vtkMultiProcessStream::OperatorPush(T value)
{
  this->Internals->Data.push_back(vtkTypeTraits<T>::VTK_TYPE_ID);
  this->Internals->Push(reinterpret_cast<unsigned char*>(&value), sizeof(T));
  return (*this);
}

//------------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator<<(bool value)
{
  return this->OperatorPush<char>(value);
}

//------------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator<<(char value)
{
  return this->OperatorPush(value);
}

//------------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator<<(signed char value)
{
  return this->OperatorPush(value);
}

//------------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator<<(unsigned char value)
{
  return this->OperatorPush(value);
}

//------------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator<<(short value)
{
  return this->OperatorPush(value);
}

//------------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator<<(unsigned short value)
{
  return this->OperatorPush(value);
}

//------------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator<<(int value)
{
  return this->OperatorPush(value);
}

//------------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator<<(unsigned int value)
{
  return this->OperatorPush(value);
}

//------------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator<<(long value)
{
  return this->OperatorPush(value);
}

//------------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator<<(unsigned long value)
{
  return this->OperatorPush(value);
}

//------------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator<<(long long value)
{
  return this->OperatorPush(value);
}

//------------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator<<(unsigned long long value)
{
  return this->OperatorPush(value);
}

//------------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator<<(float value)
{
  return this->OperatorPush(value);
}

//------------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator<<(double value)
{
  return this->OperatorPush(value);
}

//------------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator<<(const char* value)
{
  this->operator<<(std::string(value));
  return *this;
}

//------------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator<<(const std::string& value)
{
  // Set the type
  this->Internals->Data.push_back(VTK_STRING);

  // Find the real string size
  int size = static_cast<int>(value.size());

  // Set the string size
  this->Internals->Push(reinterpret_cast<unsigned char*>(&size), sizeof(int));

  // Set the string content
  for (int idx = 0; idx < size; idx++)
  {
    this->Internals->Push(reinterpret_cast<const unsigned char*>(&value[idx]), sizeof(char));
  }
  return (*this);
}

//------------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator<<(const vtkMultiProcessStream& value)
{
  unsigned int size = static_cast<unsigned int>(value.Internals->Data.size());
  size += 1;
  this->Internals->Data.push_back(VTK_OBJECT);
  this->Internals->Push(reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int));
  this->Internals->Data.push_back(value.Endianness);
  this->Internals->Data.insert(
    this->Internals->Data.end(), value.Internals->Data.begin(), value.Internals->Data.end());
  return (*this);
}

//------------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator>>(vtkMultiProcessStream& value)
{
  assert(this->Internals->Data.front() == VTK_OBJECT);
  this->Internals->Data.pop_front();

  unsigned int size;
  this->Internals->Pop(reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int));

  assert(size >= 1);
  this->Internals->Pop(&value.Endianness, 1);
  size--;

  value.Internals->Data.resize(size);
  this->Internals->Pop(&value.Internals->Data[0], size);
  return (*this);
}

//------------------------------------------------------------------------------
template <typename T>
inline vtkMultiProcessStream& vtkMultiProcessStream::OperatorPop(T& value)
{
  assert(this->Internals->Data.front() == vtkTypeTraits<T>::VTK_TYPE_ID);
  this->Internals->Data.pop_front();
  this->Internals->Pop(reinterpret_cast<unsigned char*>(&value), sizeof(T));
  return (*this);
}

//------------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator>>(bool& v)
{
  char value;
  this->OperatorPop(value);
  v = (value != 0);
  return (*this);
}

//------------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator>>(char& value)
{
  return this->OperatorPop(value);
}

//------------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator>>(signed char& value)
{
  return this->OperatorPop(value);
}

//------------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator>>(unsigned char& value)
{
  return this->OperatorPop(value);
}

//------------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator>>(short& value)
{
  return this->OperatorPop(value);
}

//------------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator>>(unsigned short& value)
{
  return this->OperatorPop(value);
}

//------------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator>>(int& value)
{
  // Automatically convert 64 bit values in case we are trying to transfer
  // vtkIdType with processes compiled with 32/64 values.
  if (this->Internals->Data.front() == VTK_TYPE_INT64)
  {
    long long value64;
    (*this) >> value64;
    value = static_cast<int>(value64);
    return (*this);
  }
  return this->OperatorPop(value);
}

//------------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator>>(unsigned int& value)
{
  return this->OperatorPop(value);
}

//------------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator>>(long& value)
{
  // Automatically convert 64 bit values in case we are trying to transfer
  // vtkIdType with processes compiled with 32/64 values.
#if VTK_SIZEOF_LONG == 4
  if (this->Internals->Data.front() == VTK_TYPE_INT64)
  {
    long long value64;
    (*this) >> value64;
    value = static_cast<int>(value64);
    return (*this);
  }
#else
  if (this->Internals->Data.front() == VTK_TYPE_INT32)
  {
    int value32;
    (*this) >> value32;
    value = value32;
    return (*this);
  }
#endif
  return this->OperatorPop(value);
}

//------------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator>>(unsigned long& value)
{
  return this->OperatorPop(value);
}

//------------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator>>(long long& value)
{
  // Automatically convert 64 bit values in case we are trying to transfer
  // vtkIdType with processes compiled with 32/64 values.
  if (this->Internals->Data.front() == VTK_TYPE_INT32)
  {
    int value32;
    (*this) >> value32;
    value = value32;
    return (*this);
  }
  return this->OperatorPop(value);
}

//------------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator>>(unsigned long long& value)
{
  return this->OperatorPop(value);
}

//------------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator>>(float& value)
{
  return this->OperatorPop(value);
}

//------------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator>>(double& value)
{
  return this->OperatorPop(value);
}

//------------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator>>(std::string& value)
{
  value = "";
  assert(this->Internals->Data.front() == VTK_STRING);
  this->Internals->Data.pop_front();
  int stringSize;
  this->Internals->Pop(reinterpret_cast<unsigned char*>(&stringSize), sizeof(int));
  value.resize(stringSize);
  char c_value;
  for (int idx = 0; idx < stringSize; idx++)
  {
    this->Internals->Pop(reinterpret_cast<unsigned char*>(&c_value), sizeof(char));
    value[idx] = c_value;
  }
  return (*this);
}

//------------------------------------------------------------------------------
std::vector<unsigned char> vtkMultiProcessStream::GetRawData() const
{
  std::vector<unsigned char> data;
  this->GetRawData(data);
  return data;
}

//------------------------------------------------------------------------------
void vtkMultiProcessStream::GetRawData(std::vector<unsigned char>& data) const
{
  data.clear();
  data.push_back(this->Endianness);
  data.resize(1 + this->Internals->Data.size());
  vtkInternals::DataType::iterator iter;
  int cc = 1;
  for (iter = this->Internals->Data.begin(); iter != this->Internals->Data.end(); ++iter, ++cc)
  {
    data[cc] = (*iter);
  }
}

//------------------------------------------------------------------------------
void vtkMultiProcessStream::SetRawData(const std::vector<unsigned char>& data)
{
  this->Internals->Data.clear();
  unsigned char endianness = data.front();
  std::vector<unsigned char>::const_iterator iter = data.begin();
  ++iter;
  this->Internals->Data.resize(data.size() - 1);
  int cc = 0;
  for (; iter != data.end(); ++iter, ++cc)
  {
    this->Internals->Data[cc] = *iter;
  }
  if (this->Endianness != endianness)
  {
    this->Internals->SwapBytes();
  }
}

//------------------------------------------------------------------------------
void vtkMultiProcessStream::GetRawData(unsigned char*& data, unsigned int& size)
{
  delete[] data;

  size = this->Size() + 1;
  data = new unsigned char[size + 1];
  assert("pre: cannot allocate raw data buffer" && (data != nullptr));

  data[0] = this->Endianness;
  vtkInternals::DataType::iterator iter = this->Internals->Data.begin();
  for (int idx = 1; iter != this->Internals->Data.end(); ++iter, ++idx)
  {
    data[idx] = *iter;
  }
}

//------------------------------------------------------------------------------
void vtkMultiProcessStream::SetRawData(const unsigned char* data, unsigned int size)
{
  this->Internals->Data.clear();
  if (size > 0)
  {
    unsigned char endianness = data[0];
    this->Internals->Data.resize(size - 1);
    int cc = 0;
    for (; cc < static_cast<int>(size - 1); cc++)
    {
      this->Internals->Data[cc] = data[cc + 1];
    }
    if (this->Endianness != endianness)
    {
      this->Internals->SwapBytes();
    }
  }
}
VTK_ABI_NAMESPACE_END
