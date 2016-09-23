/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiProcessStream.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMultiProcessStream.h"

#include "vtkObjectFactory.h"
#include "vtkSocketCommunicator.h" // for vtkSwap8 and vtkSwap4 macros.
#include <deque>
#include <cassert>

class vtkMultiProcessStream::vtkInternals
{
public:
  typedef std::deque<unsigned char> DataType;
  DataType Data;

  enum Types
  {
    int32_value,
    uint32_value,
    char_value,
    uchar_value,
    double_value,
    float_value,
    string_value,
    int64_value,
    uint64_value,
    stream_value
  };

  void Push(const unsigned char* data, size_t length)
  {
    for (size_t cc=0; cc < length; cc++)
    {
      this->Data.push_back(data[cc]);
    }
  }

  void Pop(unsigned char* data, size_t length)
  {
    for (size_t cc=0; cc < length; cc++)
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
      iter++;
      switch(type)
      {
      case int32_value:
      case uint32_value:
        wordSize = sizeof(int);
        break;

      case float_value:
        wordSize = sizeof(float);
        break;

      case double_value:
        wordSize = sizeof(double);
        break;

      case char_value:
      case uchar_value:
        wordSize = sizeof(char);
        break;

      case stream_value:
        wordSize = sizeof(int);
        break;

      case string_value:
        // We want to bitswap the string size which is an int
        wordSize = sizeof(int);
        break;
      }

      switch (wordSize)
      {
        case 1: break;
        case 4: vtkSwap4(&(*iter)); break;
        case 8: vtkSwap8(&(*iter)); break;
      }

      // In case of string we don't need to swap char values
      int nbSkip = 0;
      if (type == string_value || type == stream_value)
      {
        nbSkip = *reinterpret_cast<int*>(&*iter);
      }

      while (wordSize>0)
      {
        iter++;
        wordSize--;
      }

      // Skip String chars
      for (int cc=0; cc < nbSkip; cc++)
      {
        iter++;
      }
    }
  }
};

//----------------------------------------------------------------------------
vtkMultiProcessStream::vtkMultiProcessStream()
{
  this->Internals = new vtkMultiProcessStream::vtkInternals();
#ifdef VTK_WORDS_BIGENDIAN
  this->Endianness = vtkMultiProcessStream::BigEndian;
#else
  this->Endianness = vtkMultiProcessStream::LittleEndian;
#endif
}

//----------------------------------------------------------------------------
vtkMultiProcessStream::~vtkMultiProcessStream()
{
  delete this->Internals;
  this->Internals = 0;
}

//----------------------------------------------------------------------------
vtkMultiProcessStream::vtkMultiProcessStream(const vtkMultiProcessStream& other)
{
  this->Internals = new vtkMultiProcessStream::vtkInternals();
  this->Internals->Data = other.Internals->Data;
  this->Endianness = other.Endianness;
}

//----------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator=(const vtkMultiProcessStream& other)
{
  this->Internals->Data = other.Internals->Data;
  this->Endianness = other.Endianness;
  return (*this);
}

//----------------------------------------------------------------------------
void vtkMultiProcessStream::Reset()
{
  this->Internals->Data.clear();
}

//----------------------------------------------------------------------------
int vtkMultiProcessStream::Size()
{
  return( static_cast<int>(this->Internals->Data.size()));
}

//----------------------------------------------------------------------------
bool vtkMultiProcessStream::Empty()
{
 return( this->Internals->Data.empty() );
}

//----------------------------------------------------------------------------
void vtkMultiProcessStream::Push(double array[], unsigned int size)
{
  assert( "pre: array is NULL!" && (array != NULL) );
  this->Internals->Data.push_back( vtkInternals::double_value );
  this->Internals->Push(
      reinterpret_cast<unsigned char*>( &size ), sizeof(unsigned int ) );
  this->Internals->Push(
      reinterpret_cast<unsigned char*>(array), sizeof(double)*size);
}

//----------------------------------------------------------------------------
void vtkMultiProcessStream::Push(float array[], unsigned int size)
{
  assert( "pre: array is NULL!" && (array != NULL) );
  this->Internals->Data.push_back( vtkInternals::float_value );
  this->Internals->Push(
      reinterpret_cast<unsigned char*>( &size ), sizeof(unsigned int) );
  this->Internals->Push(
      reinterpret_cast<unsigned char*>(array), sizeof(float)*size);
}

//----------------------------------------------------------------------------
void vtkMultiProcessStream::Push(int array[], unsigned int size)
{
  assert( "pre: array is NULL!" && (array != NULL) );
  this->Internals->Data.push_back( vtkInternals::int32_value );
  this->Internals->Push(
      reinterpret_cast<unsigned char*>( &size ), sizeof(unsigned int) );
  this->Internals->Push(
      reinterpret_cast<unsigned char*>(array), sizeof(int)*size);
}

//----------------------------------------------------------------------------
void vtkMultiProcessStream::Push(char array[], unsigned int size)
{
  assert( "pre: array is NULL!" && (array != NULL) );
  this->Internals->Data.push_back( vtkInternals::char_value );
  this->Internals->Push(
     reinterpret_cast<unsigned char*>( &size ), sizeof(unsigned int) );
  this->Internals->Push(
     reinterpret_cast<unsigned char*>(array), sizeof(char)*size);
}

//----------------------------------------------------------------------------
void vtkMultiProcessStream::Push(
    unsigned int array[], unsigned int size )
{
  assert( "pre: array is NULL!" && (array != NULL) );
  this->Internals->Data.push_back( vtkInternals::uint32_value );
  this->Internals->Push(
       reinterpret_cast<unsigned char*>( &size ), sizeof(unsigned int) );
  this->Internals->Push(
       reinterpret_cast<unsigned char*>(array), sizeof(unsigned int)*size);
}

//----------------------------------------------------------------------------
void vtkMultiProcessStream::Push(
    unsigned char array[], unsigned int size )
{
  assert( "pre: array is NULL!" && (array != NULL) );
  this->Internals->Data.push_back( vtkInternals::uchar_value );
  this->Internals->Push(
     reinterpret_cast<unsigned char*>( &size ), sizeof(unsigned int) );
  this->Internals->Push( array, size);
}

//----------------------------------------------------------------------------
void vtkMultiProcessStream::Push(
    vtkTypeInt64 array[], unsigned int size )
{
  assert( "pre: array is NULL!" && (array != NULL) );
  this->Internals->Data.push_back( vtkInternals::int64_value );
  this->Internals->Push(
      reinterpret_cast<unsigned char*>( &size ), sizeof(unsigned int) );
  this->Internals->Push(
       reinterpret_cast<unsigned char*>(array), sizeof(vtkTypeUInt64)*size );
}

//----------------------------------------------------------------------------
void vtkMultiProcessStream::Push(
    vtkTypeUInt64 array[], unsigned int size )
{
  assert( "pre: array is NULL!" && (array != NULL) );
  this->Internals->Data.push_back( vtkInternals::uint64_value );
  this->Internals->Push(
     reinterpret_cast<unsigned char*>( &size ), sizeof(unsigned int) );
  this->Internals->Push(
      reinterpret_cast<unsigned char*>(array), sizeof(vtkTypeUInt64)*size );
}

//----------------------------------------------------------------------------
void vtkMultiProcessStream::Pop(double*& array, unsigned int& size)
{
  assert( "pre: stream data must be double" &&
          this->Internals->Data.front()==vtkInternals::double_value);
  this->Internals->Data.pop_front();

  if( array == NULL )
  {
    // Get the size of the array
    this->Internals->Pop(
        reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int));

    // Allocate array
    array = new double[ size ];
    assert( "ERROR: cannot allocate array" && (array != NULL) );
  }
  else
  {
    unsigned int sz;

    // Get the size of the array
    this->Internals->Pop(
        reinterpret_cast<unsigned char*>(&sz), sizeof(unsigned int));
    assert("ERROR: input array size does not match size of data" &&
            (sz==size) );
  }

  // Pop the array data
  this->Internals->Pop(
      reinterpret_cast<unsigned char*>(array),sizeof(double)*size );
}

//----------------------------------------------------------------------------
void vtkMultiProcessStream::Pop(float*& array, unsigned int& size)
{
  assert( "pre: stream data must be float" &&
          this->Internals->Data.front()==vtkInternals::float_value);
  this->Internals->Data.pop_front();

  if( array == NULL )
  {
    // Get the size of the array
    this->Internals->Pop(
        reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int) );

    // Allocate array
    array = new float[ size ];
    assert( "ERROR: cannot allocate array" && (array != NULL) );
  }
  else
  {
    unsigned int sz;

    // Get the size of the array
    this->Internals->Pop(
        reinterpret_cast<unsigned char*>(&sz), sizeof(unsigned int));
    assert("ERROR: input array size does not match size of data" &&
                (sz==size) );
  }

  // Pop the array data
  this->Internals->Pop(
      reinterpret_cast<unsigned char*>(array),sizeof(float)*size);
}

//----------------------------------------------------------------------------
void vtkMultiProcessStream::Pop(int*& array, unsigned int& size)
{
  assert( "pre: stream data must be int" &&
           this->Internals->Data.front()==vtkInternals::int32_value );
  this->Internals->Data.pop_front();

  if( array == NULL )
  {
    // Get the size of the array
    this->Internals->Pop(
       reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int) );

    // Allocate the array
    array = new int[ size ];
    assert( "ERROR: cannot allocate array" && (array != NULL) );
  }
  else
  {
    unsigned int sz;

    // Get the size of the array
    this->Internals->Pop(
        reinterpret_cast<unsigned char*>(&sz), sizeof(unsigned int));
    assert("ERROR: input array size does not match size of data" &&
                (sz==size) );
  }

  // Pop the array data
  this->Internals->Pop(
      reinterpret_cast<unsigned char*>(array), sizeof(int)*size );
}

//----------------------------------------------------------------------------
void vtkMultiProcessStream::Pop(char*& array, unsigned int& size)
{
  assert( "pre: stream data must be of type char" &&
          this->Internals->Data.front()==vtkInternals::char_value );
  this->Internals->Data.pop_front();

  if( array == NULL )
  {
    // Get the size of the array
    this->Internals->Pop(
        reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int) );

    // Allocate the array
    array = new char[ size ];
    assert( "ERROR: cannot allocate array" && (array != NULL) );
  }
  else
  {
    unsigned int sz;

    // Get the size of the array
    this->Internals->Pop(
        reinterpret_cast<unsigned char*>(&sz), sizeof(unsigned int));
    assert("ERROR: input array size does not match size of data" &&
                (sz==size) );
  }

  // Pop the array data
  this->Internals->Pop(
      reinterpret_cast<unsigned char*>(array), sizeof(char)*size );
}

//----------------------------------------------------------------------------
void vtkMultiProcessStream::Pop(unsigned int*& array, unsigned int& size )
{
  assert( "pre: stream data must be of type unsigned int" &&
          this->Internals->Data.front()==vtkInternals::uint32_value );
  this->Internals->Data.pop_front();

  if( array == NULL )
  {
    // Get the size of the array
    this->Internals->Pop(
        reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int) );

    // Allocate the array
    array = new unsigned int[ size ];
    assert( "ERROR: cannot allocate array" && (array != NULL) );
  }
  else
  {
    unsigned int sz;

    // Get the size of the array
    this->Internals->Pop(
        reinterpret_cast<unsigned char*>(&sz), sizeof(unsigned int));
    assert("ERROR: input array size does not match size of data" &&
                (sz==size) );
  }

  // Pop the array data
  this->Internals->Pop(
      reinterpret_cast<unsigned char*>(array), sizeof(unsigned int)*size );
}

//----------------------------------------------------------------------------
void vtkMultiProcessStream::Pop(unsigned char*& array, unsigned int& size )
{
  assert( "pre: stream data must be of type unsigned char" &&
          this->Internals->Data.front()==vtkInternals::uchar_value );
  this->Internals->Data.pop_front();

  if( array == NULL )
  {
    // Get the size of the array
    this->Internals->Pop(
        reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int) );

    // Allocate the array
    array = new unsigned char[ size ];
    assert( "ERROR: cannot allocate array" && (array != NULL) );
  }
  else
  {
    unsigned int sz;

    // Get the size of the array
    this->Internals->Pop(
        reinterpret_cast<unsigned char*>(&sz), sizeof(unsigned int));
    assert("ERROR: input array size does not match size of data" &&
                (sz==size) );
  }

  // Pop the array data
  this->Internals->Pop( array, size );
}

//----------------------------------------------------------------------------
void vtkMultiProcessStream::Pop(vtkTypeInt64*& array, unsigned int& size )
{
  assert( "pre: stream data must be of type vtkTypeInt64" &&
          this->Internals->Data.front()==vtkInternals::int64_value );
  this->Internals->Data.pop_front();

  if( array == NULL )
  {
    // Get the size of the array
    this->Internals->Pop(
        reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int) );

    // Allocate the array
    array = new vtkTypeInt64[ size ];
    assert( "ERROR: cannot allocate array" && (array != NULL) );
  }
  else
  {
    unsigned int sz;

    // Get the size of the array
    this->Internals->Pop(
        reinterpret_cast<unsigned char*>(&sz), sizeof(unsigned int));
    assert("ERROR: input array size does not match size of data" &&
                (sz==size) );
  }

  // Pop the array data
  this->Internals->Pop(
      reinterpret_cast<unsigned char*>(array), sizeof(vtkTypeInt64)*size );
}

//----------------------------------------------------------------------------
void vtkMultiProcessStream::Pop(vtkTypeUInt64*& array, unsigned int& size )
{
  assert( "pre: stream data must be of type vtkTypeUInt64" &&
          this->Internals->Data.front()==vtkInternals::uint64_value );
  this->Internals->Data.pop_front();

  if( array == NULL )
  {
    // Get the size of the array
    this->Internals->Pop(
        reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int) );

    // Allocate the array
    array = new vtkTypeUInt64[ size ];
    assert( "ERROR: cannot allocate array" && (array != NULL) );
  }
  else
  {
    unsigned int sz;

    // Get the size of the array
    this->Internals->Pop(
        reinterpret_cast<unsigned char*>(&sz), sizeof(unsigned int));
    assert("ERROR: input array size does not match size of data" &&
                (sz==size) );
  }

  // Pop the array data
  this->Internals->Pop(
      reinterpret_cast<unsigned char*>(array), sizeof(vtkTypeUInt64)*size );
}

//----------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator << (double value)
{
  this->Internals->Data.push_back(vtkInternals::double_value);
  this->Internals->Push(reinterpret_cast<unsigned char*>(&value), sizeof(double));
  return (*this);
}

//----------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator << (float value)
{
  this->Internals->Data.push_back(vtkInternals::float_value);
  this->Internals->Push(reinterpret_cast<unsigned char*>(&value), sizeof(float));
  return (*this);
}

//----------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator << (int value)
{
  this->Internals->Data.push_back(vtkInternals::int32_value);
  this->Internals->Push(reinterpret_cast<unsigned char*>(&value), sizeof(int));
  return (*this);
}

//----------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator << (char value)
{
  this->Internals->Data.push_back(vtkInternals::char_value);
  this->Internals->Push(reinterpret_cast<unsigned char*>(&value), sizeof(char));
  return (*this);
}

//----------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator << (bool v)
{
  char value = v;
  this->Internals->Data.push_back(vtkInternals::char_value);
  this->Internals->Push(reinterpret_cast<unsigned char*>(&value), sizeof(char));
  return (*this);
}


//----------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator << (unsigned int value)
{
  this->Internals->Data.push_back(vtkInternals::uint32_value);
  this->Internals->Push(reinterpret_cast<unsigned char*>(&value), sizeof(unsigned int));
  return (*this);
}

//----------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator << (unsigned char value)
{
  this->Internals->Data.push_back(vtkInternals::uchar_value);
  this->Internals->Push(&value, sizeof(unsigned char));
  return (*this);
}

//-----------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator << (vtkTypeInt64 value)
{
  this->Internals->Data.push_back(vtkInternals::int64_value);
  this->Internals->Push(reinterpret_cast<unsigned char*>(&value),
                        sizeof(vtkTypeInt64));
  return (*this);
}

//-----------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator << (vtkTypeUInt64 value)
{
  this->Internals->Data.push_back(vtkInternals::uint64_value);
  this->Internals->Push(reinterpret_cast<unsigned char*>(&value),
                        sizeof(vtkTypeUInt64));
  return (*this);
}

//----------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator << (const char* value)
{
  this->operator<< (std::string(value));
  return *this;
}

//----------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator << (const std::string& value)
{
  // Find the real string size
  int size = static_cast<int>(value.size());

  // Set the type
  this->Internals->Data.push_back(vtkInternals::string_value);

  // Set the string size
  this->Internals->Push(reinterpret_cast<unsigned char*>(&size),
                        sizeof(int));

  // Set the string content
  for(int idx=0; idx < size; idx++)
  {
    this->Internals->Push( reinterpret_cast<const unsigned char*>(&value[idx]),
                           sizeof(char));
  }
  return (*this);
}

//----------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator << (
  const vtkMultiProcessStream& value)
{
  unsigned int size = static_cast<unsigned int>(value.Internals->Data.size());
  size += 1;
  this->Internals->Data.push_back(vtkInternals::stream_value);
  this->Internals->Push(reinterpret_cast<unsigned char*>(&size),
    sizeof(unsigned int));
  this->Internals->Data.push_back(value.Endianness);
  this->Internals->Data.insert(this->Internals->Data.end(),
    value.Internals->Data.begin(), value.Internals->Data.end());
  return (*this);
}

//----------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator >> (
  vtkMultiProcessStream& value)
{
  assert(this->Internals->Data.front() == vtkInternals::stream_value);
  this->Internals->Data.pop_front();

  unsigned int size;
  this->Internals->Pop(reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int));

  assert(size>=1);
  this->Internals->Pop(&value.Endianness, 1);
  size--;

  value.Internals->Data.resize(size);
  this->Internals->Pop(&value.Internals->Data[0], size);
  return (*this);
}

//----------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator >> (double &value)
{
  assert(this->Internals->Data.front() == vtkInternals::double_value);
  this->Internals->Data.pop_front();
  this->Internals->Pop(reinterpret_cast<unsigned char*>(&value), sizeof(double));
  return (*this);
}

//----------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator >> (float &value)
{
  assert(this->Internals->Data.front() == vtkInternals::float_value);
  this->Internals->Data.pop_front();
  this->Internals->Pop(reinterpret_cast<unsigned char*>(&value), sizeof(float));
  return (*this);
}

//----------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator >> (int &value)
{
  // Automatically convert 64 bit values in case we are trying to transfer
  // vtkIdType with processes compiled with 32/64 values.
  if (this->Internals->Data.front() == vtkInternals::int64_value)
  {
    vtkTypeInt64 value64;
    (*this) >> value64;
    value = static_cast<int>(value64);
    return (*this);
  }
  assert(this->Internals->Data.front() == vtkInternals::int32_value);
  this->Internals->Data.pop_front();
  this->Internals->Pop(reinterpret_cast<unsigned char*>(&value), sizeof(int));
  return (*this);
}

//----------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator >> (char &value)
{
  assert(this->Internals->Data.front() == vtkInternals::char_value);
  this->Internals->Data.pop_front();
  this->Internals->Pop(reinterpret_cast<unsigned char*>(&value), sizeof(char));
  return (*this);
}

//----------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator >> (bool &v)
{
  char value;
  assert(this->Internals->Data.front() == vtkInternals::char_value);
  this->Internals->Data.pop_front();
  this->Internals->Pop(reinterpret_cast<unsigned char*>(&value), sizeof(char));
  v = (value != 0);
  return (*this);
}


//----------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator >> (unsigned int &value)
{
  assert(this->Internals->Data.front() == vtkInternals::uint32_value);
  this->Internals->Data.pop_front();
  this->Internals->Pop(reinterpret_cast<unsigned char*>(&value), sizeof(unsigned int));
  return (*this);
}

//----------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator >> (unsigned char &value)
{
  assert(this->Internals->Data.front() == vtkInternals::uchar_value);
  this->Internals->Data.pop_front();
  this->Internals->Pop(reinterpret_cast<unsigned char*>(&value), sizeof(unsigned char));
  return (*this);
}

//----------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator >> (vtkTypeInt64 &value)
{
  // Automatically convert 64 bit values in case we are trying to transfer
  // vtkIdType with processes compiled with 32/64 values.
  if (this->Internals->Data.front() == vtkInternals::int32_value)
  {
    int value32;
    (*this) >> value32;
    value = value32;
    return (*this);
  }
  assert(this->Internals->Data.front() == vtkInternals::int64_value);
  this->Internals->Data.pop_front();
  this->Internals->Pop(reinterpret_cast<unsigned char*>(&value), sizeof(vtkTypeInt64));
  return (*this);
}

//----------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator >> (vtkTypeUInt64 &value)
{
  assert(this->Internals->Data.front() == vtkInternals::uint64_value);
  this->Internals->Data.pop_front();
  this->Internals->Pop(reinterpret_cast<unsigned char*>(&value), sizeof(unsigned int));
  return (*this);
}

//----------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator >> (std::string& value)
{
  value = "";
  assert(this->Internals->Data.front() == vtkInternals::string_value);
  this->Internals->Data.pop_front();
  int stringSize;
  this->Internals->Pop( reinterpret_cast<unsigned char*>(&stringSize),
                        sizeof(int));
  char c_value;
  for(int idx=0; idx < stringSize; idx++)
  {
    this->Internals->Pop( reinterpret_cast<unsigned char*>(&c_value),
                          sizeof(char));
    value += c_value;
  }
  return (*this);
}

//----------------------------------------------------------------------------
void vtkMultiProcessStream::GetRawData(std::vector<unsigned char>& data) const
{
  data.clear();
  data.push_back(this->Endianness);
  data.resize(1 + this->Internals->Data.size());
  vtkInternals::DataType::iterator iter;
  int cc=1;
  for (iter = this->Internals->Data.begin();
    iter != this->Internals->Data.end(); ++iter, ++cc)
  {
    data[cc] = (*iter);
  }
}

//----------------------------------------------------------------------------
void vtkMultiProcessStream::SetRawData(const std::vector<unsigned char>& data)
{
  this->Internals->Data.clear();
  unsigned char endianness = data.front();
  std::vector<unsigned char>::const_iterator iter = data.begin();
  iter++;
  this->Internals->Data.resize(data.size()-1);
  int cc=0;
  for (;iter != data.end(); iter++, cc++)
  {
    this->Internals->Data[cc] = *iter;
  }
  if (this->Endianness != endianness)
  {
    this->Internals->SwapBytes();
  }
}

//----------------------------------------------------------------------------
void vtkMultiProcessStream::GetRawData(
    unsigned char*& data, unsigned int &size )
{
  delete [] data;

  size = this->Size()+1;
  data = new unsigned char[ size+1 ];
  assert( "pre: cannot allocate raw data buffer" && (data != NULL) );


  data[0] = this->Endianness;
  vtkInternals::DataType::iterator iter = this->Internals->Data.begin();
  for(int idx=1 ; iter != this->Internals->Data.end(); ++iter, ++idx )
  {
    data[ idx ] = *iter;
  }
}

//----------------------------------------------------------------------------
void vtkMultiProcessStream::SetRawData(const unsigned char* data,
  unsigned int size)
{
  this->Internals->Data.clear();
  if (size > 0)
  {
    unsigned char endianness = data[0];
    this->Internals->Data.resize(size-1);
    int cc=0;
    for (;cc < static_cast<int>(size-1); cc++)
    {
      this->Internals->Data[cc] = data[cc+1];
    }
    if (this->Endianness != endianness)
    {
      this->Internals->SwapBytes();
    }
  }
}

