/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ADIOSWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <cstring> // memset

#include <limits>
#include <iterator>
#include <sstream>
#include <iostream>
#include <vector>
#include <map>
#include <numeric>
#include <functional>
#include <algorithm>
#include <utility>

#include <adios.h>

#include "ADIOSWriter.h"
#include "ADIOSUtilities.h"

#ifndef _NDEBUG
#define DebugMacro(x)  std::cerr << "DEBUG: Rank[" << Context::Rank << "] " << x << std::endl;
//#define DebugMacro(x)
#endif

// Define an output operator for std::pair for attributes
template<typename T1, typename T2>
std::ostream & operator<<(std::ostream & stream, const std::pair<T1, T2> &p)
{
  return (stream << p.first << ',' << p.second);
}

// Use an internal ADIOS function for now so we can use the transform info
extern "C" {
  int64_t adios_common_define_var (int64_t group_id, const char * name,
    const char * path, enum ADIOS_DATATYPES type, const char * dimensions,
    const char * global_dimensions, const char * local_offsets,
    char *transform_type_str);
}

static const int64_t INVALID_INT64 = std::numeric_limits<int64_t>::min();

//----------------------------------------------------------------------------
struct ADIOSWriter::Context
{
  static MPI_Comm Comm;
  static int Rank;
  static int CommSize;
  static int RefCount;

  Context()
  {
    if(this->RefCount == 0)
      {
      int init = 0;
      MPI_Initialized(&init);
      if(init == 1)
        {
        MPI_Comm_size(Context::Comm, &Context::CommSize);
        MPI_Comm_rank(Context::Comm, &Context::Rank);
        }

      int err;

      err = adios_init_noxml(Context::Comm);
      ADIOSUtilities::TestWriteErrorEq(0, err);

      err = adios_allocate_buffer(ADIOS_BUFFER_ALLOC_LATER, 100);
      ADIOSUtilities::TestWriteErrorEq(0, err);
      }
    ++this->RefCount;
  }

  ~Context()
  {
    --this->RefCount;
    if(this->RefCount == 0)
      {
      int init = 0;
      MPI_Initialized(&init);
      if(init == 1)
        {
        MPI_Barrier(Context::Comm);
        }
      adios_finalize(Context::Rank);
      }
  }
};

MPI_Comm ADIOSWriter::Context::Comm = static_cast<MPI_Comm>(NULL);
int ADIOSWriter::Context::Rank = 0;
int ADIOSWriter::Context::CommSize = 1;
int ADIOSWriter::Context::RefCount = 0;

//----------------------------------------------------------------------------
template<typename T>
std::string ToString(const T& x)
{
  std::stringstream ss;
  ss << x;
  return ss.str();
}

template<typename T, typename TIterator>
std::string ToString(TIterator begin, TIterator end)
{
  if(begin == end)
    {
    return "";
    }

  std::stringstream ss;
  std::copy(begin, end-1, std::ostream_iterator<size_t>(ss, ","));
  ss << *(end-1);
  return ss.str();
}

std::string ToString(const std::vector<size_t> &x)
{
  return ToString<size_t>(x.begin(), x.end());
}

//----------------------------------------------------------------------------
struct ADIOSWriter::ADIOSWriterImpl
{
  ADIOSWriterImpl(void)
  : IsWriting(false), File(INVALID_INT64), Group(INVALID_INT64),
    GroupSize(0), TotalSize(0)
  {
  }
  ~ADIOSWriterImpl(void)
  {
  }

  void TestDefine(void)
  {
    if(this->IsWriting)
      {
      throw std::runtime_error("Unable to declare variables after writing"
      " has started");
      }
  }

  bool IsWriting;
  int64_t File;
  int64_t Group;
  uint64_t GroupSize;
  uint64_t TotalSize;
};

//----------------------------------------------------------------------------
ADIOSWriter::ADIOSWriter(ADIOS::TransportMethod transport,
  const std::string &transportArgs)
: WriterContext(new ADIOSWriter::Context), Impl(new ADIOSWriterImpl)
{
  int err;

  err = adios_declare_group(&this->Impl->Group, "VTK", "", adios_flag_yes);
  ADIOSUtilities::TestWriteErrorEq(0, err);

  err = adios_select_method(this->Impl->Group,
    ADIOS::ToString(transport).c_str(), transportArgs.c_str(), "");
  ADIOSUtilities::TestWriteErrorEq(0, err);
}

//----------------------------------------------------------------------------
ADIOSWriter::~ADIOSWriter(void)
{
  this->Close();
  delete this->Impl;
  delete this->WriterContext;
}

//----------------------------------------------------------------------------
bool ADIOSWriter::SetCommunicator(MPI_Comm comm)
{
  if(Context::RefCount <= 0) // Not yet initialized, can set whatever we want
    {
    Context::Comm = comm;
    }
  else if(Context::Comm != comm) // Already initialized, can't change state
    {
    return false;
    }
  return true;
}

//----------------------------------------------------------------------------
template<typename TN>
void ADIOSWriter::DefineAttribute(const std::string& path, const TN& value)
{
  // ADIOS attributes are stored as thier "stringified" versions :-(
  std::stringstream valueStr;
  valueStr << value;

  DebugMacro( "Define Attribute: " << path << ": " << value);

  int err;
  err = adios_define_attribute(this->Impl->Group, path.c_str(), "",
    ADIOSUtilities::TypeNativeToADIOS<TN>::T,
    const_cast<char*>(valueStr.str().c_str()), "");
  ADIOSUtilities::TestWriteErrorEq(0, err);
}

#define INSTANTIATE(T) \
template void ADIOSWriter::DefineAttribute<T>(const std::string&, const T&);
INSTANTIATE(int8_t)
INSTANTIATE(int16_t)
INSTANTIATE(int32_t)
//INSTANTIATE(int64_t)
INSTANTIATE(uint8_t)
INSTANTIATE(uint16_t)
INSTANTIATE(uint32_t)
INSTANTIATE(uint64_t)
INSTANTIATE(vtkIdType)
INSTANTIATE(float)
INSTANTIATE(double)
INSTANTIATE(std::string)
#undef INSTANTIATE

//----------------------------------------------------------------------------
template<typename TN>
int ADIOSWriter::DefineScalar(const std::string& path)
{
  this->Impl->TestDefine();

  DebugMacro( "Define Scalar: " << path);

  int id;
  id = adios_define_var(this->Impl->Group, path.c_str(), "",
    ADIOSUtilities::TypeNativeToADIOS<TN>::T,
    "", "", "");
  ADIOSUtilities::TestWriteErrorNe(-1, id);
  this->Impl->GroupSize += sizeof(TN);

  return id;
}
#define INSTANTIATE(T) \
template int ADIOSWriter::DefineScalar<T>(const std::string& path);
INSTANTIATE(int8_t)
INSTANTIATE(int16_t)
INSTANTIATE(int32_t)
//INSTANTIATE(int64_t)
INSTANTIATE(uint8_t)
INSTANTIATE(uint16_t)
INSTANTIATE(uint32_t)
INSTANTIATE(uint64_t)
INSTANTIATE(vtkIdType)
INSTANTIATE(float)
INSTANTIATE(double)
#undef INSTANTIATE

//----------------------------------------------------------------------------
int ADIOSWriter::DefineScalar(const std::string& path, const std::string& v)
{
  this->Impl->TestDefine();

  DebugMacro( "Define Scalar: " << path);

  int id;
  id = adios_define_var(this->Impl->Group, path.c_str(), "",
    adios_string, "", "", "");
  ADIOSUtilities::TestWriteErrorNe(-1, id);
  this->Impl->GroupSize += v.size();

  return id;
}

//----------------------------------------------------------------------------
template<typename TN>
int ADIOSWriter::DefineArray(const std::string& path,
  const std::vector<size_t>& dims, ADIOS::Transform xfm)
{
  return this->DefineArray(path, dims, ADIOSUtilities::TypeNativeToVTK<TN>::T, xfm);
}
#define INSTANTIATE(T) \
template int ADIOSWriter::DefineArray<T>(const std::string& path, \
  const std::vector<size_t>& dims, ADIOS::Transform xfm);
INSTANTIATE(int8_t)
INSTANTIATE(int16_t)
INSTANTIATE(int32_t)
//INSTANTIATE(int64_t)
INSTANTIATE(uint8_t)
INSTANTIATE(uint16_t)
INSTANTIATE(uint32_t)
INSTANTIATE(uint64_t)
INSTANTIATE(vtkIdType)
INSTANTIATE(float)
INSTANTIATE(double)
#undef INSTANTIATE

//----------------------------------------------------------------------------
int ADIOSWriter::DefineArray(const std::string& path,
  const std::vector<size_t>& dims, int vtkType, ADIOS::Transform xfm)
{
  this->Impl->TestDefine();
  ADIOS_DATATYPES adiosType = ADIOSUtilities::TypeVTKToADIOS(vtkType);

  std::string dimsLocal = ToString(dims);
  size_t numElements = std::accumulate(dims.begin(), dims.end(), 1,
    std::multiplies<size_t>());
  size_t numBytes = ADIOSUtilities::TypeSize(adiosType) * numElements;

  DebugMacro("Define Array:  " << path << " [" << dimsLocal << "]");

  int id;
  id = adios_common_define_var(this->Impl->Group, path.c_str(), "",
    adiosType, dimsLocal.c_str(), "", "",
    const_cast<char*>(ADIOS::ToString(xfm).c_str()));
  ADIOSUtilities::TestWriteErrorNe(-1, id);
  this->Impl->GroupSize += numBytes;
  return id;
}

//----------------------------------------------------------------------------
int ADIOSWriter::DefineGlobalArray(const std::string& path,
  const size_t *dimsLocal, const size_t *dimsGlobal, const size_t *dimsOffset,
  size_t nDims, int vtkType, ADIOS::Transform xfm)
{
  this->Impl->TestDefine();
  ADIOS_DATATYPES adiosType = ADIOSUtilities::TypeVTKToADIOS(vtkType);

  size_t numElements = std::accumulate(dimsLocal, dimsLocal+nDims, 1,
    std::multiplies<size_t>());
  size_t numBytes = ADIOSUtilities::TypeSize(adiosType) * numElements;
  std::string strDimsLocal = ToString<size_t>(dimsLocal, dimsLocal+nDims);
  std::string strDimsGlobal = ToString<size_t>(dimsGlobal, dimsGlobal+nDims);
  std::string strDimsOffset = ToString<size_t>(dimsOffset, dimsOffset+nDims);

  int id;
  id = adios_common_define_var(this->Impl->Group, path.c_str(), "", adiosType,
    strDimsLocal.c_str(), strDimsGlobal.c_str(), strDimsOffset.c_str(),
    const_cast<char*>(ADIOS::ToString(xfm).c_str()));
  ADIOSUtilities::TestWriteErrorNe(-1, id);
  this->Impl->GroupSize += numBytes;
  return id;
}

//----------------------------------------------------------------------------
void ADIOSWriter::Open(const std::string &fileName, bool append)
{
  int err;

  err = adios_open(&this->Impl->File, "VTK", fileName.c_str(), append?"a":"w",
    Context::Comm);
  ADIOSUtilities::TestWriteErrorEq(0, err);

  err = adios_group_size(this->Impl->File, this->Impl->GroupSize,
    &this->Impl->TotalSize);
  ADIOSUtilities::TestWriteErrorEq(0, err);
}

//----------------------------------------------------------------------------
void ADIOSWriter::Close(void)
{
  if(this->Impl->File == INVALID_INT64)
    {
    return;
    }

  adios_close(this->Impl->File);
  this->Impl->File = INVALID_INT64;

  MPI_Barrier(Context::Comm);
}

//----------------------------------------------------------------------------
template<typename TN>
void ADIOSWriter::WriteScalar(const std::string& path, const TN& value)
{
  DebugMacro( "Write Scalar: " << path);

  this->Impl->IsWriting = true;

  int err;
  err = adios_write(this->Impl->File, path.c_str(), &const_cast<TN&>(value));
  ADIOSUtilities::TestWriteErrorEq(0, err);
}
#define INSTANTIATE(T) \
template void ADIOSWriter::WriteScalar<T>(const std::string& path, \
  const T& value);
INSTANTIATE(int8_t)
INSTANTIATE(int16_t)
INSTANTIATE(int32_t)
//INSTANTIATE(int64_t)
INSTANTIATE(uint8_t)
INSTANTIATE(uint16_t)
INSTANTIATE(uint32_t)
INSTANTIATE(uint64_t)
INSTANTIATE(vtkIdType)
INSTANTIATE(float)
INSTANTIATE(double)
#undef INSTANTIATE

//----------------------------------------------------------------------------
template<>
void ADIOSWriter::WriteScalar<std::string>(const std::string& path,
  const std::string& value)
{
  DebugMacro( "Write Scalar: " << path);

  this->Impl->IsWriting = true;

  int err;
  err = adios_write(this->Impl->File, path.c_str(),
    const_cast<char*>(value.c_str()));
  ADIOSUtilities::TestWriteErrorEq(0, err);
}

//----------------------------------------------------------------------------
template<typename TN>
void ADIOSWriter::WriteArray(const std::string& path, const TN* value)
{
  DebugMacro( "Write Array:  " << path);

  this->Impl->IsWriting = true;

  int err;
  err = adios_write(this->Impl->File, path.c_str(), const_cast<TN*>(value));
  ADIOSUtilities::TestWriteErrorEq(0, err);
}
#define INSTANTIATE(T) \
template void ADIOSWriter::WriteArray<T>(const std::string& path, \
  const T* value);
INSTANTIATE(int8_t)
INSTANTIATE(int16_t)
INSTANTIATE(int32_t)
//INSTANTIATE(int64_t)
INSTANTIATE(uint8_t)
INSTANTIATE(uint16_t)
INSTANTIATE(uint32_t)
INSTANTIATE(uint64_t)
INSTANTIATE(vtkIdType)
INSTANTIATE(float)
INSTANTIATE(double)
INSTANTIATE(void)
#undef INSTANTIATE
