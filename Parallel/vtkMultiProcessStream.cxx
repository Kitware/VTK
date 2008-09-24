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
#include <vtkstd/deque>
#include <assert.h>

class vtkMultiProcessStream::vtkInternals
{
public:
  typedef vtkstd::deque<unsigned char> DataType;
  DataType Data;

  enum Types
    {
    int32_value,
    uint32_value,
    char_value,
    uchar_value,
    double_value,
    float_value
    };

  void Push(unsigned char* data, size_t length)
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
        }

      switch (wordSize)
        {
      case 1: break;
      case 4: vtkSwap4(&(*iter)); break;
      case 8: vtkSwap8(&(*iter)); break;
        }
      while (wordSize>0)
        {
        iter++;
        wordSize--;
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
}

//----------------------------------------------------------------------------
vtkMultiProcessStream& vtkMultiProcessStream::operator=(const vtkMultiProcessStream& other)
{
  this->Internals->Data = other.Internals->Data;
  return (*this);
}

//----------------------------------------------------------------------------
void vtkMultiProcessStream::Reset()
{
  this->Internals->Data.clear();
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
void vtkMultiProcessStream::GetRawData(vtkstd::vector<unsigned char>& data) const
{
  data.clear();
  data.push_back(this->Endianness);
  data.insert(data.end(),
    this->Internals->Data.begin(),
    this->Internals->Data.end());
}

//----------------------------------------------------------------------------
void vtkMultiProcessStream::SetRawData(const vtkstd::vector<unsigned char>& data)
{
  this->Internals->Data.clear();
  unsigned char endianness = data.front();
  this->Internals->Data.insert(
    this->Internals->Data.end(),
    (++data.begin()),
    data.end());
  if (this->Endianness != endianness)
    {
    this->Internals->SwapBytes();
    }
}

