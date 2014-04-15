/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOStreamWrapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSystemIncludes.h" // Cannot include vtkOStreamWrapper.h directly.

#include "vtkIOStream.h"
#include "vtkIndent.h"
#include "vtkLargeInteger.h"
#include "vtkStdString.h"
#include "vtkObjectBase.h"
#include "vtkSmartPointerBase.h"

#include <string>

#define VTKOSTREAM_OPERATOR(type) \
  vtkOStreamWrapper& vtkOStreamWrapper::operator << (type a) \
    { this->ostr << a; return *this; }

#define VTKOSTREAM_OPERATOR_FUNC(arg) \
  vtkOStreamWrapper& vtkOStreamWrapper::operator << (arg) \
    { this->ostr << a; return *this; }

//----------------------------------------------------------------------------
vtkOStreamWrapper::vtkOStreamWrapper(ostream& os): ostr(os)
{
}

//----------------------------------------------------------------------------
vtkOStreamWrapper::vtkOStreamWrapper(vtkOStreamWrapper& r): ostr(r.ostr)
{
}

//----------------------------------------------------------------------------
vtkOStreamWrapper::~vtkOStreamWrapper()
{
}

//----------------------------------------------------------------------------
vtkOStreamWrapper& vtkOStreamWrapper::operator << (const EndlType&)
{
  this->ostr << endl;
  return *this;
}

//----------------------------------------------------------------------------
VTKOSTREAM_OPERATOR(const vtkIndent&);
VTKOSTREAM_OPERATOR(vtkObjectBase&);
VTKOSTREAM_OPERATOR(const vtkLargeInteger&);
VTKOSTREAM_OPERATOR(const vtkSmartPointerBase&);
VTKOSTREAM_OPERATOR(const vtkStdString&);
VTKOSTREAM_OPERATOR(const char*);
VTKOSTREAM_OPERATOR(void*);
VTKOSTREAM_OPERATOR(char);
VTKOSTREAM_OPERATOR(short);
VTKOSTREAM_OPERATOR(int);
VTKOSTREAM_OPERATOR(long);
VTKOSTREAM_OPERATOR(unsigned char);
VTKOSTREAM_OPERATOR(unsigned short);
VTKOSTREAM_OPERATOR(unsigned int);
VTKOSTREAM_OPERATOR(unsigned long);
VTKOSTREAM_OPERATOR(float);
VTKOSTREAM_OPERATOR(double);
VTKOSTREAM_OPERATOR(bool);
#if defined(VTK_TYPE_USE_LONG_LONG)
VTKOSTREAM_OPERATOR(long long);
VTKOSTREAM_OPERATOR(unsigned long long);
#endif
#if defined(VTK_TYPE_USE___INT64)
VTKOSTREAM_OPERATOR(__int64);
VTKOSTREAM_OPERATOR(unsigned __int64);
#endif
VTKOSTREAM_OPERATOR_FUNC(void (*a)(void*));
VTKOSTREAM_OPERATOR_FUNC(void* (*a)(void*));
VTKOSTREAM_OPERATOR_FUNC(int (*a)(void*));
VTKOSTREAM_OPERATOR_FUNC(int* (*a)(void*));
VTKOSTREAM_OPERATOR_FUNC(float* (*a)(void*));
VTKOSTREAM_OPERATOR_FUNC(const char* (*a)(void*));
VTKOSTREAM_OPERATOR_FUNC(void (*a)(void*, int*));

//----------------------------------------------------------------------------
vtkOStreamWrapper& vtkOStreamWrapper::operator << (std_string const& s)
{
  this->ostr << reinterpret_cast<std::string const&>(s);
  return *this;
}

//----------------------------------------------------------------------------
#if defined(__IBMCPP__)
vtkOStreamWrapper& vtkOStreamWrapper::WriteInternal(const char* a)
{
  this->ostr << a;
  return *this;
}
vtkOStreamWrapper& vtkOStreamWrapper::WriteInternal(void* a)
{
  this->ostr << a;
  return *this;
}
#endif

//----------------------------------------------------------------------------
vtkOStreamWrapper& vtkOStreamWrapper::write(const char* str,
                                            unsigned long size)
{
  this->ostr.write(str, size);
  return *this;
}

//----------------------------------------------------------------------------
ostream& vtkOStreamWrapper::GetOStream()
{
  return this->ostr;
}

//----------------------------------------------------------------------------
vtkOStreamWrapper::operator ostream&()
{
  return this->ostr;
}

//----------------------------------------------------------------------------
vtkOStreamWrapper::operator int()
{
  return this->ostr? 1:0;
}

//----------------------------------------------------------------------------
void vtkOStreamWrapper::flush()
{
  this->ostr.flush();
}
