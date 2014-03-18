/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOStreamWrapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOStreamWrapper - Wrapper for C++ ostream.  Internal VTK use only.
// .SECTION Description
// Provides a wrapper around the C++ ostream so that VTK source files
// need not include the full C++ streams library.  This is intended to
// prevent cluttering of the translation unit and speed up
// compilation.  Experimentation has revealed between 10% and 60% less
// time for compilation depending on the platform.  This wrapper is
// used by the macros in vtkSetGet.h.

#ifndef __vtkOStreamWrapper_h
#define __vtkOStreamWrapper_h

#include "vtkCommonCoreModule.h"

#ifndef __VTK_SYSTEM_INCLUDES__INSIDE
Do_not_include_vtkOStreamWrapper_directly__vtkSystemIncludes_includes_it;
#endif

class vtkIndent;
class vtkObjectBase;
class vtkLargeInteger;
class vtkSmartPointerBase;
// workaround clang bug, needs export on forward declaration
#ifdef __clang__
class VTKCOMMONCORE_EXPORT vtkStdString;
#else
class vtkStdString;
#endif

class VTKCOMMONCORE_EXPORT vtkOStreamWrapper
{
public:
  // Description:
  // Construct class to reference a real ostream.  All methods and
  // operators will be forwarded.
  vtkOStreamWrapper(ostream& os);
  vtkOStreamWrapper(vtkOStreamWrapper& r);

  // Description:
  virtual ~vtkOStreamWrapper();

  // Description:
  // Type for a fake endl.
  struct EndlType {};

  // Description:
  // Forward this output operator to the real ostream.
  vtkOStreamWrapper& operator << (const EndlType&);
  vtkOStreamWrapper& operator << (const vtkIndent&);
  vtkOStreamWrapper& operator << (vtkObjectBase&);
  vtkOStreamWrapper& operator << (const vtkLargeInteger&);
  vtkOStreamWrapper& operator << (const vtkSmartPointerBase&);
  vtkOStreamWrapper& operator << (const vtkStdString&);
  vtkOStreamWrapper& operator << (const char*);
  vtkOStreamWrapper& operator << (void*);
  vtkOStreamWrapper& operator << (char);
  vtkOStreamWrapper& operator << (short);
  vtkOStreamWrapper& operator << (int);
  vtkOStreamWrapper& operator << (long);
  vtkOStreamWrapper& operator << (unsigned char);
  vtkOStreamWrapper& operator << (unsigned short);
  vtkOStreamWrapper& operator << (unsigned int);
  vtkOStreamWrapper& operator << (unsigned long);
  vtkOStreamWrapper& operator << (float);
  vtkOStreamWrapper& operator << (double);
#if defined(VTK_TYPE_USE_LONG_LONG)
  vtkOStreamWrapper& operator << (long long);
  vtkOStreamWrapper& operator << (unsigned long long);
#endif
#if defined(VTK_TYPE_USE___INT64)
  vtkOStreamWrapper& operator << (__int64);
  vtkOStreamWrapper& operator << (unsigned __int64);
#endif
  vtkOStreamWrapper& operator << (bool);

  // Work-around for IBM Visual Age bug in overload resolution.
#if defined(__IBMCPP__)
  vtkOStreamWrapper& WriteInternal(const char*);
  vtkOStreamWrapper& WriteInternal(void*);
  template <typename T>
  vtkOStreamWrapper& operator << (T* p)
    {
    return this->WriteInternal(p);
    }
#endif

  vtkOStreamWrapper& operator << (void (*)(void*));
  vtkOStreamWrapper& operator << (void* (*)(void*));
  vtkOStreamWrapper& operator << (int (*)(void*));
  vtkOStreamWrapper& operator << (int* (*)(void*));
  vtkOStreamWrapper& operator << (float* (*)(void*));
  vtkOStreamWrapper& operator << (const char* (*)(void*));
  vtkOStreamWrapper& operator << (void (*)(void*, int*));

  // Description:
  // Forward the write method to the real stream.
  vtkOStreamWrapper& write(const char*, unsigned long);

  // Description:
  // Get a reference to the real ostream.
  ostream& GetOStream();

  // Description:
  // Allow conversion to the real ostream type.  This allows an
  // instance of vtkOStreamWrapper to look like ostream when passing to a
  // function argument.
  operator ostream&();

  // Description:
  // Forward conversion to bool to the real ostream.
  operator int();

  // Description:
  // Forward the flush method to the real ostream.
  void flush();

  // Description:
  // Implementation detail to allow macros to provide an endl that may
  // or may not be used.
  static void UseEndl(const EndlType&) {}
protected:
  // Reference to the real ostream.
  ostream& ostr;
private:
  vtkOStreamWrapper& operator=(const vtkOStreamWrapper& r); // Not Implemented.
};

#endif
// VTK-HeaderTest-Exclude: vtkOStreamWrapper.h
