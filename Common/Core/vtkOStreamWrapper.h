// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOStreamWrapper
 * @brief   Wrapper for C++ ostream.  Internal VTK use only.
 *
 * Provides a wrapper around the C++ ostream so that VTK source files
 * need not include the full C++ streams library.  This is intended to
 * prevent cluttering of the translation unit and speed up
 * compilation.  Experimentation has revealed between 10% and 60% less
 * time for compilation depending on the platform.  This wrapper is
 * used by the macros in vtkSetGet.h.
 */

#ifndef vtkOStreamWrapper_h
#define vtkOStreamWrapper_h

#include "vtkCommonCoreModule.h"

#ifndef VTK_SYSTEM_INCLUDES_INSIDE
Do_not_include_vtkOStreamWrapper_directly_vtkSystemIncludes_includes_it;
#endif

VTK_ABI_NAMESPACE_BEGIN
class vtkIndent;
class vtkObjectBase;
class vtkLargeInteger;
template <typename T>
class vtkSmartPointer;
class vtkSmartPointerBase;
class vtkStdString;

class VTKCOMMONCORE_EXPORT VTK_WRAPEXCLUDE vtkOStreamWrapper
{
  class std_string;

public:
  ///@{
  /**
   * Construct class to reference a real ostream.  All methods and
   * operators will be forwarded.
   */
  vtkOStreamWrapper(ostream& os);
  vtkOStreamWrapper(vtkOStreamWrapper& r);
  ///@}

  virtual ~vtkOStreamWrapper();

  /**
   * Type for a fake endl.
   */
  struct EndlType
  {
  };

  ///@{
  /**
   * Forward this output operator to the real ostream.
   */
  vtkOStreamWrapper& operator<<(const EndlType&);
  vtkOStreamWrapper& operator<<(const vtkIndent&);
  vtkOStreamWrapper& operator<<(vtkObjectBase&);
  vtkOStreamWrapper& operator<<(const vtkLargeInteger&);
  vtkOStreamWrapper& operator<<(const vtkSmartPointerBase&);
  vtkOStreamWrapper& operator<<(const vtkStdString&);
  vtkOStreamWrapper& operator<<(const char*);
  vtkOStreamWrapper& operator<<(void*);
  vtkOStreamWrapper& operator<<(char);
  vtkOStreamWrapper& operator<<(short);
  vtkOStreamWrapper& operator<<(int);
  vtkOStreamWrapper& operator<<(long);
  vtkOStreamWrapper& operator<<(long long);
  vtkOStreamWrapper& operator<<(unsigned char);
  vtkOStreamWrapper& operator<<(unsigned short);
  vtkOStreamWrapper& operator<<(unsigned int);
  vtkOStreamWrapper& operator<<(unsigned long);
  vtkOStreamWrapper& operator<<(unsigned long long);
  vtkOStreamWrapper& operator<<(float);
  vtkOStreamWrapper& operator<<(double);
  vtkOStreamWrapper& operator<<(bool);
  ///@}

  // Work-around for IBM Visual Age bug in overload resolution.
#if defined(__IBMCPP__)
  vtkOStreamWrapper& WriteInternal(const char*);
  vtkOStreamWrapper& WriteInternal(void*);
  template <typename T>
  vtkOStreamWrapper& operator<<(T* p)
  {
    return this->WriteInternal(p);
  }
#endif

  vtkOStreamWrapper& operator<<(void (*)(void*));
  vtkOStreamWrapper& operator<<(void* (*)(void*));
  vtkOStreamWrapper& operator<<(int (*)(void*));
  vtkOStreamWrapper& operator<<(int* (*)(void*));
  vtkOStreamWrapper& operator<<(float* (*)(void*));
  vtkOStreamWrapper& operator<<(const char* (*)(void*));
  vtkOStreamWrapper& operator<<(void (*)(void*, int*));

  // Accept std::string without a declaration.
  template <template <typename, typename, typename> class S>
  vtkOStreamWrapper& operator<<(const S<char, std::char_traits<char>, std::allocator<char>>& s)
  {
    return *this << reinterpret_cast<std_string const&>(s);
  }

  // Accept vtkSmartPointer for output.
  template <typename T>
  vtkOStreamWrapper& operator<<(const vtkSmartPointer<T>& ptr)
  {
    this->ostr << (static_cast<T*>(ptr));
    return *this;
  }

  /**
   * Forward the write method to the real stream.
   */
  vtkOStreamWrapper& write(const char*, unsigned long);

  /**
   * Get a reference to the real ostream.
   */
  ostream& GetOStream();

  /**
   * Allow conversion to the real ostream type.  This allows an
   * instance of vtkOStreamWrapper to look like ostream when passing to a
   * function argument.
   */
  operator ostream&();

  /**
   * Forward conversion to bool to the real ostream.
   */
  operator int();

  /**
   * Forward the flush method to the real ostream.
   */
  void flush();

  ///@{
  /**
   * Implementation detail to allow macros to provide an endl that may
   * or may not be used.
   */
  static void UseEndl(const EndlType&) {}
  ///@}
protected:
  // Reference to the real ostream.
  ostream& ostr;

private:
  vtkOStreamWrapper& operator=(const vtkOStreamWrapper& r) = delete;
  vtkOStreamWrapper& operator<<(std_string const&);
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkOStreamWrapper.h
