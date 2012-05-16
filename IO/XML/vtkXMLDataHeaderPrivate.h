/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLDataHeaderPrivate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkXMLDataHeaderPrivate_DoNotInclude
# error "do not include unless you know what you are doing"
#endif

#ifndef __vtkXMLDataHeaderPrivate_h
#define __vtkXMLDataHeaderPrivate_h

#include "vtkType.h"
#include <vector>

// Abstract interface using type vtkTypeUInt64 to access an array
// of either vtkTypeUInt32 or vtkTypeUInt64.  Shared by vtkXMLWriter
// and vtkXMLDataParser to write/read binary data headers.
class vtkXMLDataHeader
{
public:
  virtual void Resize(size_t count) = 0;
  virtual vtkTypeUInt64 Get(size_t index) const = 0;
  virtual bool Set(size_t index, vtkTypeUInt64 value) = 0;
  virtual size_t WordSize() const = 0;
  virtual size_t WordCount() const = 0;
  virtual unsigned char* Data() = 0;
  size_t DataSize() const { return this->WordCount()*this->WordSize(); }
  virtual ~vtkXMLDataHeader() {}
  static inline vtkXMLDataHeader* New(int width, size_t count);
};

template <typename T>
class vtkXMLDataHeaderImpl: public vtkXMLDataHeader
{
  std::vector<T> Header;
public:
  vtkXMLDataHeaderImpl(size_t n): Header(n, 0) {}
  virtual void Resize(size_t count)
    { this->Header.resize(count, 0); }
  virtual vtkTypeUInt64 Get(size_t index) const
    { return this->Header[index]; }
  virtual bool Set(size_t index, vtkTypeUInt64 value)
    {
    this->Header[index] = T(value);
    return vtkTypeUInt64(this->Header[index]) == value;
    }
  virtual size_t WordSize() const { return sizeof(T); }
  virtual size_t WordCount() const { return this->Header.size(); }
  virtual unsigned char* Data()
    { return reinterpret_cast<unsigned char*>(&this->Header[0]); }
};

vtkXMLDataHeader* vtkXMLDataHeader::New(int width, size_t count)
{
  switch(width)
    {
    case 32: return new vtkXMLDataHeaderImpl<vtkTypeUInt32>(count);
    case 64: return new vtkXMLDataHeaderImpl<vtkTypeUInt64>(count);
    }
  return 0;
}

#endif
// VTK-HeaderTest-Exclude: vtkXMLDataHeaderPrivate.h
