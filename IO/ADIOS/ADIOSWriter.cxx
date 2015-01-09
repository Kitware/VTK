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

#include <cstring>

#include <algorithm>
#include <iterator>
#include <limits>
#include <map>
#include <ostream>
#include <sstream>
#include <vector>

#include <vtkType.h>

#include <adios.h>

#include "ADIOSUtilities.h"

#include "ADIOSWriter.h"

namespace ADIOS
{

//----------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, const ArrayDim d)
{
  if(d.ValueS.empty())
    {
    return os << d.ValueI;
    }
  return os << d.ValueS;
}

//----------------------------------------------------------------------------
template<typename T, typename TIterator>
std::string ToString(TIterator begin, TIterator end)
{
  if(begin == end)
    {
    return "";
    }

  std::stringstream ss;
  std::copy(begin, end-1, std::ostream_iterator<T>(ss, ","));
  ss << *(end-1);
  return ss.str();
}

template<typename T>
std::string ToString(const std::vector<T> &x)
{
  return ToString<T>(x.begin(), x.end());
}

//----------------------------------------------------------------------------

// Writer::InitContext is internally a singleton struct to ensure that the
//ADIOS runtime only gets initialized and finialized once
struct Writer::InitContext
{
  static int RefCount;
  static MPI_Comm GlobalComm;

  MPI_Comm Comm;
  int Rank;
  int CommSize;

  InitContext()
  : Comm(GlobalComm)
  {
    if(this->RefCount == 0)
      {
      int init = 0;
      MPI_Initialized(&init);
      WriteError::TestEq(1, init, "InitContext: MPI is not yet initialized");

      int err = adios_init_noxml(this->Comm);
      WriteError::TestEq(0, err);
      }
    ++this->RefCount;

    MPI_Comm_size(this->Comm, &this->CommSize);
    MPI_Comm_rank(this->Comm, &this->Rank);
  }

  ~InitContext()
  {
    --this->RefCount;
    if(this->RefCount == 0)
      {
      // If we've gotten this far then we know that MPI has been initialized
      // already
      //
      // Not really sure of a barrier is necessary here but it's explicitly used
      // in ADIOS examples and documnetation before finalizing.  Since this only
      // really occurs and pipeline tear-down time it shouldn't affect
      // performance.
      MPI_Barrier(this->Comm);
      adios_finalize(this->Rank);
      }
  }
};

// Dafault communicator is invalid
MPI_Comm Writer::InitContext::GlobalComm = MPI_COMM_NULL;
int Writer::InitContext::RefCount = 0;

//-------------------:---------------------------------------------------------
struct Writer::WriterImpl
{
  WriterImpl()
  : Group(-1)
  { }

  struct ScalarInfo
  {
    ScalarInfo(const std::string& path, ADIOS_DATATYPES type)
    : Path(path), Size(Type::SizeOf(type)), IsInt(Type::IsInt(type))
    { }
    const std::string Path;
    const size_t Size;
    const bool IsInt;
  };

  struct ScalarValue
  {
    ScalarValue(const std::string& path, const void* value)
    : Path(path), Value(value)
    { }

    virtual ~ScalarValue() { }
    virtual uint64_t GetInt() { return 0; }

    const std::string Path;
    const void* Value;
  };

  template<typename T>
  struct ScalarValueT : public ScalarValue
  {
    ScalarValueT(const std::string& path, const T& value)
    : ScalarValue(path, &ValueT), ValueT(value)
    { }

    virtual ~ScalarValueT() { }
    virtual uint64_t GetInt() { return static_cast<uint64_t>(this->ValueT); }

    const T ValueT;
  };

  struct ArrayInfo
  {
    ArrayInfo(const std::string& path, const std::vector<ArrayDim>& dims,
      ADIOS_DATATYPES type)
    : Path(path), Dims(dims), ElementSize(Type::SizeOf(type))
    { }
    const std::string Path;
    const std::vector<ArrayDim> Dims;
    const size_t ElementSize;
  };

  struct ArrayValue
  {
    ArrayValue(const std::string& path, const void* value)
    : Path(path), Value(value)
    { }
    const std::string Path;
    const void* Value;
  };

  int64_t Group;

  std::map<std::string, const ScalarInfo*> ScalarRegistry;
  std::map<std::string, const ArrayInfo*> ArrayRegistry;

  std::map<std::string, size_t> IntegralScalars;

  std::vector<const ScalarValue*> ScalarsToWrite;
  std::vector<const ArrayValue*> ArraysToWrite;
};
template<>
uint64_t Writer::WriterImpl::ScalarValueT<std::complex<float> >::GetInt()
{ return static_cast<uint64_t>(this->ValueT.real()); }
template<>
uint64_t Writer::WriterImpl::ScalarValueT<std::complex<double> >::GetInt()
{ return static_cast<uint64_t>(this->ValueT.real()); }

//-------------------:---------------------------------------------------------
bool Writer::SetCommunicator(MPI_Comm comm)
{
  // The communicator can only be set if ADIOS has not yet been initialized
  if(Writer::InitContext::RefCount == 0)
    {
    Writer::InitContext::GlobalComm = comm;
    return true;
    }
  return false;
}

//----------------------------------------------------------------------------
Writer::Writer(ADIOS::TransportMethod transport,
  const std::string& transportArgs)
: Ctx(new InitContext), Impl(new WriterImpl)
{
  int err;

  err = adios_declare_group(&this->Impl->Group, "VTK", "", adios_flag_yes);
  WriteError::TestEq(0, err);

  err = adios_select_method(this->Impl->Group,
    ToString(transport).c_str(), transportArgs.c_str(), "");
  WriteError::TestEq(0, err);
}

//----------------------------------------------------------------------------
Writer::~Writer()
{
  std::map<std::string, const WriterImpl::ScalarInfo*>::const_iterator s;
  for(s = this->Impl->ScalarRegistry.begin();
      s != this->Impl->ScalarRegistry.end();
      ++s)
    {
    delete s->second;
    }

  std::map<std::string, const WriterImpl::ArrayInfo*>::const_iterator a;
  for(a = this->Impl->ArrayRegistry.begin();
      a != this->Impl->ArrayRegistry.end();
      ++a)
    {
    delete a->second;
    }

  adios_free_group(this->Impl->Group);
  delete this->Impl;
  delete this->Ctx;
}

//----------------------------------------------------------------------------
void Writer::DefineAttribute(const std::string& path,
  ADIOS_DATATYPES adiosType, const std::string& value)
{
  int err = adios_define_attribute(this->Impl->Group, path.c_str(), "",
    adiosType, const_cast<char*>(value.c_str()), "");
  WriteError::TestEq(0, err);
}

//----------------------------------------------------------------------------
int Writer::DefineScalar(const std::string& path, ADIOS_DATATYPES adiosType)
{
  int id = adios_define_var(this->Impl->Group, path.c_str(), "", adiosType,
    "", "", "");
  WriteError::TestNe(-1, id);

  // Track localy
  this->Impl->ScalarRegistry[path] =
    new WriterImpl::ScalarInfo(path, adiosType);

  return id;
}

//----------------------------------------------------------------------------
int Writer::DefineLocalArray(const std::string& path,
  ADIOS_DATATYPES adiosType, const std::vector<ArrayDim>& dims, Transform xfm)
{
  // Verify the dimensions are usable
  for(std::vector<ArrayDim>::const_iterator di = dims.begin();
      di != dims.end(); ++di)
    {
    if(!di->ValueS.empty())
      {
      std::map<std::string, const WriterImpl::ScalarInfo*>::iterator si =
        this->Impl->ScalarRegistry.find(di->ValueS);
      WriteError::TestNe(this->Impl->ScalarRegistry.end(), si,
        "Dimension scalar variable " + di->ValueS + " is not defined");
      WriteError::TestEq(true, si->second->IsInt,
        "Dimension scalar variable " + di->ValueS + " is not an integer");
      }
    }

  // Define in the ADIOS group
  std::string dimsLocal = ToString(dims);
  int id = adios_define_var(this->Impl->Group, path.c_str(), "", adiosType,
     dimsLocal.c_str(), "", "");
  WriteError::TestNe(-1, id);

  int err = adios_set_transform(id, ToString(xfm).c_str());
  WriteError::TestEq(0, err);

  // Track locally
  this->Impl->ArrayRegistry[path] =
    new WriterImpl::ArrayInfo(path, dims, adiosType);

  return id;
}

//----------------------------------------------------------------------------
void Writer::WriteScalar(const std::string& path, ADIOS_DATATYPES adiosType,
    const void* val)
{
  std::map<std::string, const WriterImpl::ScalarInfo*>::iterator si =
    this->Impl->ScalarRegistry.find(path);
  WriteError::TestNe(this->Impl->ScalarRegistry.end(), si,
    "Scalar variable " + path + " is not defined");

  WriterImpl::ScalarValue *v;
  switch(adiosType)
    {
    case adios_byte:
      v = new WriterImpl::ScalarValueT<int8_t>(path,
        *reinterpret_cast<const int8_t*>(val));
      break;
    case adios_short:
      v = new WriterImpl::ScalarValueT<int16_t>(path,
        *reinterpret_cast<const int16_t*>(val));
      break;
    case adios_integer:
      v = new WriterImpl::ScalarValueT<int32_t>(path,
        *reinterpret_cast<const int32_t*>(val));
      break;
    case adios_long:
      v = new WriterImpl::ScalarValueT<int64_t>(path,
        *reinterpret_cast<const int32_t*>(val));
      break;
    case adios_unsigned_byte:
      v = new WriterImpl::ScalarValueT<uint8_t>(path,
        *reinterpret_cast<const uint8_t*>(val));
      break;
    case adios_unsigned_short:
      v = new WriterImpl::ScalarValueT<uint16_t>(path,
        *reinterpret_cast<const uint16_t*>(val));
      break;
    case adios_unsigned_integer:
      v = new WriterImpl::ScalarValueT<uint32_t>(path,
        *reinterpret_cast<const uint32_t*>(val));
      break;
    case adios_unsigned_long:
      v = new WriterImpl::ScalarValueT<uint64_t>(path,
        *reinterpret_cast<const uint64_t*>(val));
      break;
    case adios_real:
      v = new WriterImpl::ScalarValueT<float>(path,
        *reinterpret_cast<const float*>(val));
      break;
    case adios_double:
      v = new WriterImpl::ScalarValueT<double>(path,
        *reinterpret_cast<const double*>(val));
      break;
    case adios_complex:
      v = new WriterImpl::ScalarValueT<std::complex<float> >(path,
        *reinterpret_cast<const std::complex<float>*>(val));
      break;
    case adios_double_complex:
      v = new WriterImpl::ScalarValueT<std::complex<double> >(path,
        *reinterpret_cast<const std::complex<double>*>(val));
      break;
    default:
      v = NULL;
    }

  if(si->second->IsInt)
    {
    this->Impl->IntegralScalars[path] = v->GetInt();
    }
  this->Impl->ScalarsToWrite.push_back(v);
}

//----------------------------------------------------------------------------
void Writer::WriteArray(const std::string& path, const void* val)
{
  std::map<std::string, const WriterImpl::ArrayInfo*>::iterator ai =
    this->Impl->ArrayRegistry.find(path);
  WriteError::TestNe(this->Impl->ArrayRegistry.end(), ai,
    "Array variable " + path + " is not defined");

  for(std::vector<ArrayDim>::const_iterator di = ai->second->Dims.begin();
      di != ai->second->Dims.end();
      ++di)
    {
    if(!di->ValueS.empty())
      {
      std::map<std::string, size_t>::iterator ivi =
        this->Impl->IntegralScalars.find(di->ValueS);
      WriteError::TestNe(this->Impl->IntegralScalars.end(), ivi,
        "Scalar dimension variable " + di->ValueS +
        " has not yet been written");
      }
    }


  this->Impl->ArraysToWrite.push_back(
    new WriterImpl::ArrayValue(path, val));
}

//----------------------------------------------------------------------------
void Writer::Commit(const std::string& fName, bool app)
{
  uint64_t groupSize = 0;
  std::vector<const WriterImpl::ArrayValue*> nonEmptyArrays;

  // Step 1: Preprocessing

  // Determine scalar group size
  for(std::vector<const WriterImpl::ScalarValue*>::iterator svi =
        this->Impl->ScalarsToWrite.begin();
      svi != this->Impl->ScalarsToWrite.end();
      ++svi)
    {
    groupSize += this->Impl->ScalarRegistry[(*svi)->Path]->Size;
    }

  // Add the array sizes and filter out empties
  for(std::vector<const WriterImpl::ArrayValue*>::iterator avi =
        this->Impl->ArraysToWrite.begin();
      avi != this->Impl->ArraysToWrite.end();
      ++avi)
    {
    // This should be guaranteed to exist in the registry
    const WriterImpl::ArrayInfo *ai = this->Impl->ArrayRegistry[(*avi)->Path];

    size_t numElements = 0;
    for(std::vector<ArrayDim>::const_iterator di = ai->Dims.begin();
        di != ai->Dims.end();
        ++di)
      {
      if(numElements == 0)
        {
        numElements = di->ValueS.empty() ?
          di->ValueI : this->Impl->IntegralScalars[di->ValueS];
        }
      else
        {
        numElements *= di->ValueS.empty() ?
          di->ValueI : this->Impl->IntegralScalars[di->ValueS];
        }
      }
    //if(numElements == 0)
    //  {
    //  delete *avi;
    //  }
    //else
    //  {
      groupSize += numElements * ai->ElementSize;
      nonEmptyArrays.push_back(*avi);
    //  }
    }
  this->Impl->ArraysToWrite.clear();

  int err;

  // Step 2. Set the buffer size in MB with the full knowledge of the dynamic
  // group size
  err = adios_allocate_buffer(ADIOS_BUFFER_ALLOC_LATER, (groupSize >> 20) + 1);
  WriteError::TestEq(0, err);

  // Step 3. Open the file for writing
  int64_t file;
  err = adios_open(&file, "VTK", fName.c_str(), app?"a":"w", this->Ctx->Comm);
  WriteError::TestEq(0, err);

  uint64_t totalSize;
  err = adios_group_size(file, groupSize, &totalSize);
  WriteError::TestEq(0, err);

  // Step 4: Write scalars
  for(std::vector<const WriterImpl::ScalarValue*>::iterator svi =
        this->Impl->ScalarsToWrite.begin();
      svi != this->Impl->ScalarsToWrite.end();
      ++svi)
    {
    err = adios_write(file, (*svi)->Path.c_str(),
      const_cast<void*>((*svi)->Value));
    WriteError::TestEq(0, err);
    }

  // Step 5: Write Arrays
  for(std::vector<const WriterImpl::ArrayValue*>::iterator avi =
        nonEmptyArrays.begin();
      avi != nonEmptyArrays.end();
      ++avi)
    {
    err = adios_write(file, (*avi)->Path.c_str(),
      const_cast<void*>((*avi)->Value));
    WriteError::TestEq(0, err);
    }

  // Step 6. Close the file and commit the writes to ADIOS
  adios_close(file);
  MPI_Barrier(this->Ctx->Comm);

  // Step 7. Cleanup
  for(std::vector<const WriterImpl::ScalarValue*>::iterator svi =
        this->Impl->ScalarsToWrite.begin();
      svi != this->Impl->ScalarsToWrite.end();
      ++svi)
    {
    delete *svi;
    }
  for(std::vector<const WriterImpl::ArrayValue*>::iterator avi =
        nonEmptyArrays.begin();
      avi != nonEmptyArrays.end();
      ++avi)
    {
    delete *avi;
    }
  this->Impl->ScalarsToWrite.clear();
}

} // End namespace
