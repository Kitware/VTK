// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   SetGet
 *
 * The SetGet macros are used to interface to instance variables
 * in a standard fashion. This includes properly treating modified time
 * and printing out debug information.
 *
 * Macros are available for built-in types; for character strings;
 * vector arrays of built-in types size 2,3,4; for setting objects; and
 * debug, warning, and error printout information.
 */

#ifndef vtkSetGet_h
#define vtkSetGet_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkCompiler.h"
#include "vtkOptions.h"
#include "vtkSystemIncludes.h"
#include "vtksys/SystemTools.hxx"
#include <type_traits> // for std::underlying type.
#include <typeinfo>

//----------------------------------------------------------------------------
// Check for unsupported old compilers.
#if defined(_MSC_VER) && _MSC_VER < 1900
#error VTK requires MSVC++ 14.0 aka Visual Studio 2015 or newer
#endif

#if !defined(__clang__) && defined(__GNUC__) &&                                                    \
  (__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 8))
#error VTK requires GCC 4.8 or newer
#endif

#if VTK_USE_FUTURE_CONST
#define VTK_FUTURE_CONST const
#else
#define VTK_FUTURE_CONST
#endif

// Convert a macro representing a value to a string.
//
// Example: vtkQuoteMacro(__LINE__) will expand to "1234" whereas
// vtkInternalQuoteMacro(__LINE__) will expand to "__LINE__"
#define vtkInternalQuoteMacro(x) #x
#define vtkQuoteMacro(x) vtkInternalQuoteMacro(x)

// clang-format off
// A macro to get the name of a type
#define vtkImageScalarTypeNameMacro(type)                                                          \
  (((type) == VTK_VOID) ? "void" :                                                                 \
  (((type) == VTK_BIT) ? "bit" :                                                                   \
  (((type) == VTK_CHAR) ? "char" :                                                                 \
  (((type) == VTK_SIGNED_CHAR) ? "signed char" :                                                   \
  (((type) == VTK_UNSIGNED_CHAR) ? "unsigned char" :                                               \
  (((type) == VTK_SHORT) ? "short" :                                                               \
  (((type) == VTK_UNSIGNED_SHORT) ? "unsigned short" :                                             \
  (((type) == VTK_INT) ? "int" :                                                                   \
  (((type) == VTK_UNSIGNED_INT) ? "unsigned int" :                                                 \
  (((type) == VTK_LONG) ? "long" :                                                                 \
  (((type) == VTK_UNSIGNED_LONG) ? "unsigned long" :                                               \
  (((type) == VTK_LONG_LONG) ? "long long" :                                                       \
  (((type) == VTK_UNSIGNED_LONG_LONG) ? "unsigned long long" :                                     \
  (((type) == VTK_FLOAT) ? "float" :                                                               \
  (((type) == VTK_DOUBLE) ? "double" :                                                             \
  (((type) == VTK_ID_TYPE) ? "idtype" :                                                            \
  (((type) == VTK_STRING) ? "string" :                                                             \
  (((type) == VTK_VARIANT) ? "variant" :                                                           \
  (((type) == VTK_OBJECT) ? "object" :                                                             \
  "Undefined")))))))))))))))))))
// clang-format on

/* Various compiler-specific performance hints. */
#if defined(VTK_COMPILER_GCC) //------------------------------------------------

#define VTK_ALWAYS_INLINE __attribute__((always_inline)) inline
// CUDA does not recognize pragma options
#if defined(__CUDACC__)
#define VTK_ALWAYS_OPTIMIZE_START
#define VTK_ALWAYS_OPTIMIZE_END
#else
#define VTK_ALWAYS_OPTIMIZE_START _Pragma("GCC push_options") _Pragma("GCC optimize (\"O3\")")
#define VTK_ALWAYS_OPTIMIZE_END _Pragma("GCC pop_options")
#endif

#elif defined(VTK_COMPILER_CLANG) //--------------------------------------------

#define VTK_ALWAYS_INLINE __attribute__((always_inline)) inline
// Clang doesn't seem to support temporarily increasing optimization level,
// only decreasing it.
#define VTK_ALWAYS_OPTIMIZE_START
#define VTK_ALWAYS_OPTIMIZE_END

#elif defined(VTK_COMPILER_ICC) //----------------------------------------------

#define VTK_ALWAYS_INLINE __attribute((always_inline)) inline
// ICC doesn't seem to support temporarily increasing optimization level,
// only decreasing it.
#define VTK_ALWAYS_OPTIMIZE_START
#define VTK_ALWAYS_OPTIMIZE_END

#elif defined(VTK_COMPILER_MSVC) //---------------------------------------------

#define VTK_ALWAYS_INLINE __forceinline
#define VTK_ALWAYS_OPTIMIZE_START _Pragma("optimize(\"tgs\", on)")
// optimize("", on) resets to command line settings
#define VTK_ALWAYS_OPTIMIZE_END _Pragma("optimize(\"\", on)")

#else //------------------------------------------------------------------------

#define VTK_ALWAYS_INLINE inline
#define VTK_ALWAYS_OPTIMIZE_START
#define VTK_ALWAYS_OPTIMIZE_END

#endif

//
// Set built-in type.  Creates member Set"name"() (e.g., SetVisibility());
//
#define vtkSetMacro(name, type)                                                                    \
  virtual void Set##name(type _arg)                                                                \
  {                                                                                                \
    vtkDebugMacro(<< " setting " #name " to " << _arg);                                            \
    if (this->name != _arg)                                                                        \
    {                                                                                              \
      this->name = _arg;                                                                           \
      this->Modified();                                                                            \
    }                                                                                              \
  }
#define vtkSetMacroOverride(name, type)                                                            \
  void Set##name(type _arg) override                                                               \
  {                                                                                                \
    vtkDebugMacro(<< " setting " #name " to " << _arg);                                            \
    if (this->name != _arg)                                                                        \
    {                                                                                              \
      this->name = _arg;                                                                           \
      this->Modified();                                                                            \
    }                                                                                              \
  }

//
// Get built-in type.  Creates member Get"name"() (e.g., GetVisibility());
//
#define vtkGetMacro(name, type)                                                                    \
  virtual type Get##name() VTK_FUTURE_CONST                                                        \
  {                                                                                                \
    vtkDebugMacro(<< " returning " #name " of " << this->name);                                    \
    return this->name;                                                                             \
  }

//
// Set 'enum class' type.  Creates member Set"name"() (e.g., SetKind());
// vtkSetMacro can't be used because 'enum class' won't trivially convert to integer for logging.
//
#define vtkSetEnumMacro(name, enumType)                                                            \
  virtual void Set##name(enumType _arg)                                                            \
  {                                                                                                \
    vtkDebugMacro(<< " setting " #name " to "                                                      \
                  << static_cast<std::underlying_type<enumType>::type>(_arg));                     \
    if (this->name != _arg)                                                                        \
    {                                                                                              \
      this->name = _arg;                                                                           \
      this->Modified();                                                                            \
    }                                                                                              \
  }
#define vtkSetEnumMacroOverride(name, enumType)                                                    \
  void Set##name(enumType _arg) override                                                           \
  {                                                                                                \
    vtkDebugMacro(<< " setting " #name " to "                                                      \
                  << static_cast<std::underlying_type<enumType>::type>(_arg));                     \
    if (this->name != _arg)                                                                        \
    {                                                                                              \
      this->name = _arg;                                                                           \
      this->Modified();                                                                            \
    }                                                                                              \
  }

//
// Get 'enum class' type.  Creates member Get"name"() (e.g., GetKind());
// vtkSetMacro can't be used because 'enum class' won't trivially convert to integer for logging.
//
#define vtkGetEnumMacro(name, enumType)                                                            \
  virtual enumType Get##name() const                                                               \
  {                                                                                                \
    vtkDebugMacro(<< " returning " #name " of "                                                    \
                  << static_cast<std::underlying_type<enumType>::type>(this->name));               \
    return this->name;                                                                             \
  }

//
// Set character string.  Creates member Set"name"()
// (e.g., SetFilename(char *));
//
#define vtkSetStringMacro(name)                                                                    \
  virtual void Set##name(const char* _arg) vtkSetStringBodyMacro(name, _arg)
#define vtkSetStringMacroOverride(name)                                                            \
  void Set##name(const char* _arg) vtkSetStringBodyMacro(name, _arg) override

//
// Set a string token. Creates member Set"name"()
// (e.g., SetArrayName(vtkStringToken));
//
#define vtkSetStringTokenMacro(name)                                                               \
  virtual void Set##name(vtkStringToken _arg) vtkSetStringTokenBodyMacro(name, _arg)

// Set a file path, like vtkSetStringMacro but with VTK_FILEPATH hint.
#define vtkSetFilePathMacro(name)                                                                  \
  virtual void Set##name(VTK_FILEPATH const char* _arg) vtkSetStringBodyMacro(name, _arg)
#define vtkSetFilePathMacroOverride(name)                                                          \
  void Set##name(VTK_FILEPATH const char* _arg) vtkSetStringBodyMacro(name, _arg) override

// This macro defines a body of set string macro. It can be used either in
// the header file using vtkSetStringMacro or in the implementation.
#define vtkSetStringBodyMacro(name, _arg)                                                          \
  {                                                                                                \
    vtkDebugMacro(<< " setting " #name " to " << (_arg ? _arg : "(null)"));                        \
    if (this->name == nullptr && _arg == nullptr)                                                  \
    {                                                                                              \
      return;                                                                                      \
    }                                                                                              \
    if (this->name && _arg && (!strcmp(this->name, _arg)))                                         \
    {                                                                                              \
      return;                                                                                      \
    }                                                                                              \
    delete[] this->name;                                                                           \
    if (_arg)                                                                                      \
    {                                                                                              \
      size_t n = strlen(_arg) + 1;                                                                 \
      char* cp1 = new char[n];                                                                     \
      const char* cp2 = (_arg);                                                                    \
      this->name = cp1;                                                                            \
      do                                                                                           \
      {                                                                                            \
        *cp1++ = *cp2++;                                                                           \
      } while (--n);                                                                               \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
      this->name = nullptr;                                                                        \
    }                                                                                              \
    this->Modified();                                                                              \
  }

//
// Get character string.  Creates member Get"name"()
// (e.g., char *GetFilename());
//
#define vtkGetStringMacro(name) virtual char* Get##name() vtkGetStringBodyMacro(name)

//
// Get string token.  Creates member Get"name"()
// (e.g., vtkStringToken GetArrayName());
//
#define vtkGetStringTokenMacro(name)                                                               \
  virtual vtkStringToken Get##name() vtkGetStringTokenBodyMacro(name)

// This macro defines a body of set string macro. It can be used either in
// the header file using vtkSetStringMacro or in the implementation.
#define vtkSetStringTokenBodyMacro(name, _arg)                                                     \
  {                                                                                                \
    vtkDebugMacro(<< " setting " #name " to " << _arg.Data());                                     \
    if (this->name == _arg)                                                                        \
    {                                                                                              \
      return;                                                                                      \
    }                                                                                              \
    this->name = _arg;                                                                             \
    this->Modified();                                                                              \
  }

// Get a file path, like vtkGetStringMacro but with VTK_FILEPATH hint.
#define vtkGetFilePathMacro(name)                                                                  \
  virtual VTK_FILEPATH VTK_FUTURE_CONST char* Get##name() VTK_FUTURE_CONST vtkGetStringBodyMacro(  \
    name)

// This macro defines a body of get string macro. It can be used either in
// the header file using vtkGetStringMacro or in the implementation.
#define vtkGetStringBodyMacro(name)                                                                \
  {                                                                                                \
    vtkDebugMacro(<< " returning " #name " of " << (this->name ? this->name : "(null)"));          \
    return this->name;                                                                             \
  }

// This macro defines a body of get string-token macro. It can be used either in
// the header file using vtkGetStringTokenMacro or in the implementation.
#define vtkGetStringTokenBodyMacro(name)                                                           \
  {                                                                                                \
    vtkDebugMacro(<< " returning " #name " of " << this->name.Data());                             \
    return this->name;                                                                             \
  }

//
// Set std::string. Creates a member Set"name"()
//
#define vtkSetStdStringFromCharMacro(name)                                                         \
  virtual void Set##name(const char* arg)                                                          \
  {                                                                                                \
    vtkDebugMacro(<< " setting " #name " to " << (arg ? arg : "(null)"));                          \
    if (arg)                                                                                       \
    {                                                                                              \
      if (this->name == arg)                                                                       \
      {                                                                                            \
        return;                                                                                    \
      }                                                                                            \
      this->name = arg;                                                                            \
    }                                                                                              \
    else if (this->name.empty())                                                                   \
    {                                                                                              \
      return;                                                                                      \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
      this->name.clear();                                                                          \
    }                                                                                              \
    this->Modified();                                                                              \
  }
#define vtkSetStdStringFromCharMacroOverride(name)                                                 \
  void Set##name(const char* arg) override                                                         \
  {                                                                                                \
    vtkDebugMacro(<< " setting " #name " to " << (arg ? arg : "(null)"));                          \
    if (arg)                                                                                       \
    {                                                                                              \
      if (this->name == arg)                                                                       \
      {                                                                                            \
        return;                                                                                    \
      }                                                                                            \
      this->name = arg;                                                                            \
    }                                                                                              \
    else if (this->name.empty())                                                                   \
    {                                                                                              \
      return;                                                                                      \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
      this->name.clear();                                                                          \
    }                                                                                              \
    this->Modified();                                                                              \
  }

//
// Get character string.  Creates member Get"name"()
// (e.g., char *GetFilename());
//
#define vtkGetCharFromStdStringMacro(name)                                                         \
  virtual const char* Get##name()                                                                  \
  {                                                                                                \
    vtkDebugMacro(<< " returning " #name " of " << this->name);                                    \
    return this->name.c_str();                                                                     \
  }

//
// Set built-in type where value is constrained between min/max limits.
// Create member Set"name"() (eg., SetRadius()). #defines are
// convenience for clamping open-ended values.
// The Get"name"MinValue() and Get"name"MaxValue() members return the
// min and max limits.
//
#define vtkSetClampMacro(name, type, min, max)                                                     \
  virtual void Set##name(type _arg)                                                                \
  {                                                                                                \
    vtkDebugMacro(<< " setting " #name " to " << _arg);                                            \
    if (this->name != (_arg < min ? min : (_arg > max ? max : _arg)))                              \
    {                                                                                              \
      this->name = (_arg < min ? min : (_arg > max ? max : _arg));                                 \
      this->Modified();                                                                            \
    }                                                                                              \
  }                                                                                                \
  virtual type Get##name##MinValue()                                                               \
  {                                                                                                \
    return min;                                                                                    \
  }                                                                                                \
  virtual type Get##name##MaxValue()                                                               \
  {                                                                                                \
    return max;                                                                                    \
  }
#define vtkSetClampMacroOverride(name, type, min, max)                                             \
  void Set##name(type _arg) override                                                               \
  {                                                                                                \
    vtkDebugMacro(<< " setting " #name " to " << _arg);                                            \
    if (this->name != (_arg < min ? min : (_arg > max ? max : _arg)))                              \
    {                                                                                              \
      this->name = (_arg < min ? min : (_arg > max ? max : _arg));                                 \
      this->Modified();                                                                            \
    }                                                                                              \
  }                                                                                                \
  type Get##name##MinValue() override                                                              \
  {                                                                                                \
    return min;                                                                                    \
  }                                                                                                \
  type Get##name##MaxValue() override                                                              \
  {                                                                                                \
    return max;                                                                                    \
  }

//
// This macro defines a body of set object macro. It can be used either in
// the header file vtkSetObjectMacro or in the implementation one
// vtkSetObjectMacro. It sets the pointer to object; uses vtkObject
// reference counting methodology. Creates method
// Set"name"() (e.g., SetPoints()).
//
#define vtkSetObjectBodyMacro(name, type, args)                                                    \
  do                                                                                               \
  {                                                                                                \
    vtkDebugMacro(<< " setting " #name " to " << args);                                            \
    if (this->name != args)                                                                        \
    {                                                                                              \
      type* tempSGMacroVar = this->name;                                                           \
      this->name = args;                                                                           \
      if (this->name != nullptr)                                                                   \
      {                                                                                            \
        this->name->Register(this);                                                                \
      }                                                                                            \
      if (tempSGMacroVar != nullptr)                                                               \
      {                                                                                            \
        tempSGMacroVar->UnRegister(this);                                                          \
      }                                                                                            \
      this->Modified();                                                                            \
    }                                                                                              \
  } while (0)

//
// This macro defines a body of set object macro with
// a smart pointer class member.
//
#define vtkSetSmartPointerBodyMacro(name, type, args)                                              \
  do                                                                                               \
  {                                                                                                \
    vtkDebugMacro(<< " setting " #name " to " << args);                                            \
    if (this->name != args)                                                                        \
    {                                                                                              \
      this->name = args;                                                                           \
      this->Modified();                                                                            \
    }                                                                                              \
  } while (0)

//
// Set pointer to object; uses vtkObject reference counting methodology.
// Creates method Set"name"() (e.g., SetPoints()). This macro should
// be used in the header file.
//
#define vtkSetObjectMacro(name, type)                                                              \
  virtual void Set##name(type* _arg)                                                               \
  {                                                                                                \
    vtkSetObjectBodyMacro(name, type, _arg);                                                       \
  }
#define vtkSetObjectMacroOverride(name, type)                                                      \
  void Set##name(type* _arg) override                                                              \
  {                                                                                                \
    vtkSetObjectBodyMacro(name, type, _arg);                                                       \
  }

//
// Set pointer to a smart pointer class member.
// Creates method Set"name"() (e.g., SetPoints()). This macro should
// be used in the header file.
//
#define vtkSetSmartPointerMacro(name, type)                                                        \
  virtual void Set##name(type* _arg)                                                               \
  {                                                                                                \
    vtkSetSmartPointerBodyMacro(name, type, _arg);                                                 \
  }
#define vtkSetSmartPointerMacroOverride(name, type)                                                \
  void Set##name(type* _arg) override                                                              \
  {                                                                                                \
    vtkSetSmartPointerBodyMacro(name, type, _arg);                                                 \
  }

//
// Set pointer to object; uses vtkObject reference counting methodology.
// Creates method Set"name"() (e.g., SetPoints()). This macro should
// be used in the implementation file. You will also have to write
// prototype in the header file. The prototype should look like this:
// virtual void Set"name"("type" *);
//
// Please use vtkCxxSetObjectMacro not vtkSetObjectImplementationMacro.
// The first one is just for people who already used it.
#define vtkSetObjectImplementationMacro(class, name, type) vtkCxxSetObjectMacro(class, name, type)

#define vtkCxxSetObjectMacro(cls, name, type)                                                      \
  void cls::Set##name(type* _arg)                                                                  \
  {                                                                                                \
    vtkSetObjectBodyMacro(name, type, _arg);                                                       \
  }

//
// Set pointer to smart pointer.
// This macro is used to define the implementation.
//
#define vtkCxxSetSmartPointerMacro(cls, name, type)                                                \
  void cls::Set##name(type* _arg)                                                                  \
  {                                                                                                \
    vtkSetSmartPointerBodyMacro(name, type, _arg);                                                 \
  }

//
// Get pointer to object wrapped in vtkNew.  Creates member Get"name"
// (e.g., GetPoints()).  This macro should be used in the header file.
//
#define vtkGetNewMacro(name, type)                                                                 \
  virtual type* Get##name()                                                                        \
  {                                                                                                \
    vtkDebugMacro(<< " returning " #name " address " << this->name);                               \
    return this->name;                                                                             \
  }

//
// Get pointer to object.  Creates member Get"name" (e.g., GetPoints()).
// This macro should be used in the header file.
//
#define vtkGetObjectMacro(name, type)                                                              \
  virtual type* Get##name()                                                                        \
  {                                                                                                \
    vtkDebugMacro(<< " returning " #name " address " << static_cast<type*>(this->name));           \
    return this->name;                                                                             \
  }

//
// Get pointer to object in a smart pointer class member.
// This is only an alias and is similar to vtkGetObjectMacro.
//
#define vtkGetSmartPointerMacro(name, type) vtkGetObjectMacro(name, type)

//
// Create members "name"On() and "name"Off() (e.g., DebugOn() DebugOff()).
// Set method must be defined to use this macro.
//
#define vtkBooleanMacro(name, type)                                                                \
  virtual void name##On()                                                                          \
  {                                                                                                \
    this->Set##name(static_cast<type>(1));                                                         \
  }                                                                                                \
  virtual void name##Off()                                                                         \
  {                                                                                                \
    this->Set##name(static_cast<type>(0));                                                         \
  }

//
// Following set macros for vectors define two members for each macro.  The
// first
// allows setting of individual components (e.g, SetColor(float,float,float)),
// the second allows setting from an array (e.g., SetColor(float* rgb[3])).
// The macros vary in the size of the vector they deal with.
//
#define vtkSetVector2Macro(name, type)                                                             \
  virtual void Set##name(type _arg1, type _arg2)                                                   \
  {                                                                                                \
    vtkDebugMacro(<< " setting " #name " to (" << _arg1 << "," << _arg2 << ")");                   \
    if ((this->name[0] != _arg1) || (this->name[1] != _arg2))                                      \
    {                                                                                              \
      this->name[0] = _arg1;                                                                       \
      this->name[1] = _arg2;                                                                       \
      this->Modified();                                                                            \
    }                                                                                              \
  }                                                                                                \
  void Set##name(const type _arg[2])                                                               \
  {                                                                                                \
    this->Set##name(_arg[0], _arg[1]);                                                             \
  }
#define vtkSetVector2MacroOverride(name, type)                                                     \
  void Set##name(type _arg1, type _arg2) override                                                  \
  {                                                                                                \
    vtkDebugMacro(<< " setting " #name " to (" << _arg1 << "," << _arg2 << ")");                   \
    if ((this->name[0] != _arg1) || (this->name[1] != _arg2))                                      \
    {                                                                                              \
      this->name[0] = _arg1;                                                                       \
      this->name[1] = _arg2;                                                                       \
      this->Modified();                                                                            \
    }                                                                                              \
  }

#define vtkGetVector2Macro(name, type)                                                             \
  virtual type* Get##name() VTK_SIZEHINT(2)                                                        \
  {                                                                                                \
    vtkDebugMacro(<< " returning " #name " pointer " << this->name);                               \
    return this->name;                                                                             \
  }                                                                                                \
  VTK_WRAPEXCLUDE                                                                                  \
  virtual void Get##name(type& _arg1, type& _arg2)                                                 \
  {                                                                                                \
    _arg1 = this->name[0];                                                                         \
    _arg2 = this->name[1];                                                                         \
    vtkDebugMacro(<< " returning " #name " = (" << _arg1 << "," << _arg2 << ")");                  \
  }                                                                                                \
  VTK_WRAPEXCLUDE                                                                                  \
  virtual void Get##name(type _arg[2])                                                             \
  {                                                                                                \
    this->Get##name(_arg[0], _arg[1]);                                                             \
  }

#define vtkSetVector3Macro(name, type)                                                             \
  virtual void Set##name(type _arg1, type _arg2, type _arg3)                                       \
  {                                                                                                \
    vtkDebugMacro(<< " setting " #name " to (" << _arg1 << "," << _arg2 << "," << _arg3 << ")");   \
    if ((this->name[0] != _arg1) || (this->name[1] != _arg2) || (this->name[2] != _arg3))          \
    {                                                                                              \
      this->name[0] = _arg1;                                                                       \
      this->name[1] = _arg2;                                                                       \
      this->name[2] = _arg3;                                                                       \
      this->Modified();                                                                            \
    }                                                                                              \
  }                                                                                                \
  virtual void Set##name(const type _arg[3])                                                       \
  {                                                                                                \
    this->Set##name(_arg[0], _arg[1], _arg[2]);                                                    \
  }
#define vtkSetVector3MacroOverride(name, type)                                                     \
  void Set##name(type _arg1, type _arg2, type _arg3) override                                      \
  {                                                                                                \
    vtkDebugMacro(<< " setting " #name " to (" << _arg1 << "," << _arg2 << "," << _arg3 << ")");   \
    if ((this->name[0] != _arg1) || (this->name[1] != _arg2) || (this->name[2] != _arg3))          \
    {                                                                                              \
      this->name[0] = _arg1;                                                                       \
      this->name[1] = _arg2;                                                                       \
      this->name[2] = _arg3;                                                                       \
      this->Modified();                                                                            \
    }                                                                                              \
  }                                                                                                \
  void Set##name(const type _arg[3]) override                                                      \
  {                                                                                                \
    this->Set##name(_arg[0], _arg[1], _arg[2]);                                                    \
  }

#define vtkGetVector3Macro(name, type)                                                             \
  virtual type* Get##name() VTK_SIZEHINT(3)                                                        \
  {                                                                                                \
    vtkDebugMacro(<< " returning " #name " pointer " << this->name);                               \
    return this->name;                                                                             \
  }                                                                                                \
  VTK_WRAPEXCLUDE                                                                                  \
  virtual void Get##name(type& _arg1, type& _arg2, type& _arg3)                                    \
  {                                                                                                \
    _arg1 = this->name[0];                                                                         \
    _arg2 = this->name[1];                                                                         \
    _arg3 = this->name[2];                                                                         \
    vtkDebugMacro(<< " returning " #name " = (" << _arg1 << "," << _arg2 << "," << _arg3 << ")");  \
  }                                                                                                \
  VTK_WRAPEXCLUDE                                                                                  \
  virtual void Get##name(type _arg[3])                                                             \
  {                                                                                                \
    this->Get##name(_arg[0], _arg[1], _arg[2]);                                                    \
  }

#define vtkSetVector4Macro(name, type)                                                             \
  virtual void Set##name(type _arg1, type _arg2, type _arg3, type _arg4)                           \
  {                                                                                                \
    vtkDebugMacro(<< " setting " #name " to (" << _arg1 << "," << _arg2 << "," << _arg3 << ","     \
                  << _arg4 << ")");                                                                \
    if ((this->name[0] != _arg1) || (this->name[1] != _arg2) || (this->name[2] != _arg3) ||        \
      (this->name[3] != _arg4))                                                                    \
    {                                                                                              \
      this->name[0] = _arg1;                                                                       \
      this->name[1] = _arg2;                                                                       \
      this->name[2] = _arg3;                                                                       \
      this->name[3] = _arg4;                                                                       \
      this->Modified();                                                                            \
    }                                                                                              \
  }                                                                                                \
  virtual void Set##name(const type _arg[4])                                                       \
  {                                                                                                \
    this->Set##name(_arg[0], _arg[1], _arg[2], _arg[3]);                                           \
  }
#define vtkSetVector4MacroOverride(name, type)                                                     \
  void Set##name(type _arg1, type _arg2, type _arg3, type _arg4) override                          \
  {                                                                                                \
    vtkDebugMacro(<< " setting " #name " to (" << _arg1 << "," << _arg2 << "," << _arg3 << ","     \
                  << _arg4 << ")");                                                                \
    if ((this->name[0] != _arg1) || (this->name[1] != _arg2) || (this->name[2] != _arg3) ||        \
      (this->name[3] != _arg4))                                                                    \
    {                                                                                              \
      this->name[0] = _arg1;                                                                       \
      this->name[1] = _arg2;                                                                       \
      this->name[2] = _arg3;                                                                       \
      this->name[3] = _arg4;                                                                       \
      this->Modified();                                                                            \
    }                                                                                              \
  }                                                                                                \
  void Set##name(const type _arg[4]) override                                                      \
  {                                                                                                \
    this->Set##name(_arg[0], _arg[1], _arg[2], _arg[3]);                                           \
  }

#define vtkGetVector4Macro(name, type)                                                             \
  virtual type* Get##name() VTK_SIZEHINT(4)                                                        \
  {                                                                                                \
    vtkDebugMacro(<< " returning " #name " pointer " << this->name);                               \
    return this->name;                                                                             \
  }                                                                                                \
  VTK_WRAPEXCLUDE                                                                                  \
  virtual void Get##name(type& _arg1, type& _arg2, type& _arg3, type& _arg4)                       \
  {                                                                                                \
    _arg1 = this->name[0];                                                                         \
    _arg2 = this->name[1];                                                                         \
    _arg3 = this->name[2];                                                                         \
    _arg4 = this->name[3];                                                                         \
    vtkDebugMacro(<< " returning " #name " = (" << _arg1 << "," << _arg2 << "," << _arg3 << ","    \
                  << _arg4 << ")");                                                                \
  }                                                                                                \
  VTK_WRAPEXCLUDE                                                                                  \
  virtual void Get##name(type _arg[4])                                                             \
  {                                                                                                \
    this->Get##name(_arg[0], _arg[1], _arg[2], _arg[3]);                                           \
  }

#define vtkSetVector6Macro(name, type)                                                             \
  virtual void Set##name(type _arg1, type _arg2, type _arg3, type _arg4, type _arg5, type _arg6)   \
  {                                                                                                \
    vtkDebugMacro(<< " setting " #name " to (" << _arg1 << "," << _arg2 << "," << _arg3 << ","     \
                  << _arg4 << "," << _arg5 << "," << _arg6 << ")");                                \
    if ((this->name[0] != _arg1) || (this->name[1] != _arg2) || (this->name[2] != _arg3) ||        \
      (this->name[3] != _arg4) || (this->name[4] != _arg5) || (this->name[5] != _arg6))            \
    {                                                                                              \
      this->name[0] = _arg1;                                                                       \
      this->name[1] = _arg2;                                                                       \
      this->name[2] = _arg3;                                                                       \
      this->name[3] = _arg4;                                                                       \
      this->name[4] = _arg5;                                                                       \
      this->name[5] = _arg6;                                                                       \
      this->Modified();                                                                            \
    }                                                                                              \
  }                                                                                                \
  virtual void Set##name(const type _arg[6])                                                       \
  {                                                                                                \
    this->Set##name(_arg[0], _arg[1], _arg[2], _arg[3], _arg[4], _arg[5]);                         \
  }
#define vtkSetVector6MacroOverride(name, type)                                                     \
  void Set##name(type _arg1, type _arg2, type _arg3, type _arg4, type _arg5, type _arg6) override  \
  {                                                                                                \
    vtkDebugMacro(<< " setting " #name " to (" << _arg1 << "," << _arg2 << "," << _arg3 << ","     \
                  << _arg4 << "," << _arg5 << "," << _arg6 << ")");                                \
    if ((this->name[0] != _arg1) || (this->name[1] != _arg2) || (this->name[2] != _arg3) ||        \
      (this->name[3] != _arg4) || (this->name[4] != _arg5) || (this->name[5] != _arg6))            \
    {                                                                                              \
      this->name[0] = _arg1;                                                                       \
      this->name[1] = _arg2;                                                                       \
      this->name[2] = _arg3;                                                                       \
      this->name[3] = _arg4;                                                                       \
      this->name[4] = _arg5;                                                                       \
      this->name[5] = _arg6;                                                                       \
      this->Modified();                                                                            \
    }                                                                                              \
  }                                                                                                \
  void Set##name(const type _arg[6]) override                                                      \
  {                                                                                                \
    this->Set##name(_arg[0], _arg[1], _arg[2], _arg[3], _arg[4], _arg[5]);                         \
  }

#define vtkGetVector6Macro(name, type)                                                             \
  virtual type* Get##name() VTK_SIZEHINT(6)                                                        \
  {                                                                                                \
    vtkDebugMacro(<< " returning " #name " pointer " << this->name);                               \
    return this->name;                                                                             \
  }                                                                                                \
  VTK_WRAPEXCLUDE                                                                                  \
  virtual void Get##name(                                                                          \
    type& _arg1, type& _arg2, type& _arg3, type& _arg4, type& _arg5, type& _arg6)                  \
  {                                                                                                \
    _arg1 = this->name[0];                                                                         \
    _arg2 = this->name[1];                                                                         \
    _arg3 = this->name[2];                                                                         \
    _arg4 = this->name[3];                                                                         \
    _arg5 = this->name[4];                                                                         \
    _arg6 = this->name[5];                                                                         \
    vtkDebugMacro(<< " returning " #name " = (" << _arg1 << "," << _arg2 << "," << _arg3 << ","    \
                  << _arg4 << "," << _arg5 << "," << _arg6 << ")");                                \
  }                                                                                                \
  VTK_WRAPEXCLUDE                                                                                  \
  virtual void Get##name(type _arg[6])                                                             \
  {                                                                                                \
    this->Get##name(_arg[0], _arg[1], _arg[2], _arg[3], _arg[4], _arg[5]);                         \
  }

//
// General set vector macro creates a single method that copies specified
// number of values into object.
// Examples: void SetColor(c,3)
//
#define vtkSetVectorMacro(name, type, count)                                                       \
  virtual void Set##name(const type data[])                                                        \
  {                                                                                                \
    int i;                                                                                         \
    for (i = 0; i < count; i++)                                                                    \
    {                                                                                              \
      if (data[i] != this->name[i])                                                                \
      {                                                                                            \
        break;                                                                                     \
      }                                                                                            \
    }                                                                                              \
    if (i < count)                                                                                 \
    {                                                                                              \
      for (i = 0; i < count; i++)                                                                  \
      {                                                                                            \
        this->name[i] = data[i];                                                                   \
      }                                                                                            \
      this->Modified();                                                                            \
    }                                                                                              \
  }
#define vtkSetVectorMacroOverride(name, type, count)                                               \
  void Set##name(const type data[]) override                                                       \
  {                                                                                                \
    int i;                                                                                         \
    for (i = 0; i < count; i++)                                                                    \
    {                                                                                              \
      if (data[i] != this->name[i])                                                                \
      {                                                                                            \
        break;                                                                                     \
      }                                                                                            \
    }                                                                                              \
    if (i < count)                                                                                 \
    {                                                                                              \
      for (i = 0; i < count; i++)                                                                  \
      {                                                                                            \
        this->name[i] = data[i];                                                                   \
      }                                                                                            \
      this->Modified();                                                                            \
    }                                                                                              \
  }

//
// Get vector macro defines two methods. One returns pointer to type
// (i.e., array of type). This is for efficiency. The second copies data
// into user provided array. This is more object-oriented.
// Examples: float *GetColor() and void GetColor(float c[count]).
//
#define vtkGetVectorMacro(name, type, count)                                                       \
  virtual type* Get##name() VTK_SIZEHINT(count)                                                    \
  {                                                                                                \
    vtkDebugMacro(<< " returning " #name " pointer " << this->name);                               \
    return this->name;                                                                             \
  }                                                                                                \
  VTK_WRAPEXCLUDE                                                                                  \
  virtual void Get##name(type data[count])                                                         \
  {                                                                                                \
    for (int i = 0; i < count; i++)                                                                \
    {                                                                                              \
      data[i] = this->name[i];                                                                     \
    }                                                                                              \
  }

// Use a global function which actually calls:
//  vtkOutputWindow::GetInstance()->DisplayText();
// This is to avoid vtkObject #include of vtkOutputWindow
// while vtkOutputWindow #includes vtkObject

VTK_ABI_NAMESPACE_BEGIN
extern VTKCOMMONCORE_EXPORT void vtkOutputWindowDisplayText(const char*);
extern VTKCOMMONCORE_EXPORT void vtkOutputWindowDisplayErrorText(const char*);
extern VTKCOMMONCORE_EXPORT void vtkOutputWindowDisplayWarningText(const char*);
extern VTKCOMMONCORE_EXPORT void vtkOutputWindowDisplayGenericWarningText(const char*);
extern VTKCOMMONCORE_EXPORT void vtkOutputWindowDisplayDebugText(const char*);

// overloads that allow providing information about the filename and lineno
// generating the message.
class vtkObject;
extern VTKCOMMONCORE_EXPORT void vtkOutputWindowDisplayErrorText(
  const char*, int, const char*, vtkObject* sourceObj);
extern VTKCOMMONCORE_EXPORT void vtkOutputWindowDisplayWarningText(
  const char*, int, const char*, vtkObject* sourceObj);
extern VTKCOMMONCORE_EXPORT void vtkOutputWindowDisplayGenericWarningText(
  const char*, int, const char*);
extern VTKCOMMONCORE_EXPORT void vtkOutputWindowDisplayDebugText(
  const char*, int, const char*, vtkObject* sourceObj);
VTK_ABI_NAMESPACE_END

//
// This macro is used for any output that may not be in an instance method
// vtkGenericWarningMacro(<< "this is debug info" << this->SomeVariable);
//
#define vtkGenericWarningMacro(x)                                                                  \
  do                                                                                               \
  {                                                                                                \
    if (vtkObject::GetGlobalWarningDisplay())                                                      \
    {                                                                                              \
      vtkOStreamWrapper::EndlType const endl;                                                      \
      vtkOStreamWrapper::UseEndl(endl);                                                            \
      vtkOStrStreamWrapper vtkmsg;                                                                 \
      vtkmsg << "" x;                                                                              \
      std::string const _filename = vtksys::SystemTools::GetFilenameName(__FILE__);                \
      vtkOutputWindowDisplayGenericWarningText(_filename.c_str(), __LINE__, vtkmsg.str());         \
      vtkmsg.rdbuf()->freeze(0);                                                                   \
    }                                                                                              \
  } while (false)

//
// This macro is used for debug statements in instance methods
// vtkDebugMacro(<< "this is debug info" << this->SomeVariable);
//
#define vtkDebugMacro(x) vtkDebugWithObjectMacro(this, x)

//
// This macro is used to print out warning messages.
// vtkWarningMacro(<< "Warning message" << variable);
//
#define vtkWarningMacro(x) vtkWarningWithObjectMacro(this, x)

//
// This macro is used to print out errors
// vtkErrorMacro(<< "Error message" << variable);
//
#define vtkErrorMacro(x) vtkErrorWithObjectMacro(this, x)

//
// This macro is used to print out errors
// vtkErrorWithObjectMacro(self, << "Error message" << variable);
// self can be null
// Using two casts here so that nvcc compiler can handle const this
// pointer properly
//
#define vtkErrorWithObjectMacro(self, x)                                                           \
  do                                                                                               \
  {                                                                                                \
    if (vtkObject::GetGlobalWarningDisplay())                                                      \
    {                                                                                              \
      vtkOStreamWrapper::EndlType const endl;                                                      \
      vtkOStreamWrapper::UseEndl(endl);                                                            \
      vtkOStrStreamWrapper vtkmsg;                                                                 \
      vtkObject* _object = const_cast<vtkObject*>(static_cast<const vtkObject*>(self));            \
      if (_object)                                                                                 \
      {                                                                                            \
        vtkmsg << _object->GetObjectDescription() << ": ";                                         \
      }                                                                                            \
      else                                                                                         \
      {                                                                                            \
        vtkmsg << "(nullptr): ";                                                                   \
      }                                                                                            \
      vtkmsg << "" x;                                                                              \
      std::string const _filename = vtksys::SystemTools::GetFilenameName(__FILE__);                \
      vtkOutputWindowDisplayErrorText(_filename.c_str(), __LINE__, vtkmsg.str(), _object);         \
      vtkmsg.rdbuf()->freeze(0);                                                                   \
      vtkObject::BreakOnError();                                                                   \
    }                                                                                              \
  } while (false)

//
// This macro is used to print out warnings
// vtkWarningWithObjectMacro(self, "Warning message" << variable);
// self can be null
// Using two casts here so that nvcc compiler can handle const this
// pointer properly
//
#define vtkWarningWithObjectMacro(self, x)                                                         \
  do                                                                                               \
  {                                                                                                \
    if (vtkObject::GetGlobalWarningDisplay())                                                      \
    {                                                                                              \
      vtkOStreamWrapper::EndlType const endl;                                                      \
      vtkOStreamWrapper::UseEndl(endl);                                                            \
      vtkOStrStreamWrapper vtkmsg;                                                                 \
      vtkObject* _object = const_cast<vtkObject*>(static_cast<const vtkObject*>(self));            \
      if (_object)                                                                                 \
      {                                                                                            \
        vtkmsg << _object->GetObjectDescription() << ": ";                                         \
      }                                                                                            \
      else                                                                                         \
      {                                                                                            \
        vtkmsg << "(nullptr): ";                                                                   \
      }                                                                                            \
      vtkmsg << "" x;                                                                              \
      std::string const _filename = vtksys::SystemTools::GetFilenameName(__FILE__);                \
      vtkOutputWindowDisplayWarningText(_filename.c_str(), __LINE__, vtkmsg.str(), _object);       \
      vtkmsg.rdbuf()->freeze(0);                                                                   \
    }                                                                                              \
  } while (false)

/**
 * This macro is used to print out debug message
 * vtkDebugWithObjectMacro(self, "Warning message" << variable);
 * self can be null
 * Using two casts here so that nvcc compiler can handle const this
 * pointer properly
 */
#ifdef NDEBUG
#define vtkDebugWithObjectMacro(self, x)                                                           \
  do                                                                                               \
  {                                                                                                \
  } while (false)
#else
#define vtkDebugWithObjectMacro(self, x)                                                           \
  do                                                                                               \
  {                                                                                                \
    vtkObject* _object = const_cast<vtkObject*>(static_cast<const vtkObject*>(self));              \
    if ((!_object || _object->GetDebug()) && vtkObject::GetGlobalWarningDisplay())                 \
    {                                                                                              \
      vtkOStreamWrapper::EndlType const endl;                                                      \
      vtkOStreamWrapper::UseEndl(endl);                                                            \
      vtkOStrStreamWrapper vtkmsg;                                                                 \
      if (_object)                                                                                 \
      {                                                                                            \
        vtkmsg << _object->GetObjectDescription() << ": ";                                         \
      }                                                                                            \
      else                                                                                         \
      {                                                                                            \
        vtkmsg << "(nullptr): ";                                                                   \
      }                                                                                            \
      vtkmsg << "" x;                                                                              \
      std::string const _filename = vtksys::SystemTools::GetFilenameName(__FILE__);                \
      vtkOutputWindowDisplayDebugText(_filename.c_str(), __LINE__, vtkmsg.str(), _object);         \
      vtkmsg.rdbuf()->freeze(0);                                                                   \
    }                                                                                              \
  } while (false)
#endif

//
// This macro is used to quiet compiler warnings about unused parameters
// to methods. Only use it when the parameter really shouldn't be used.
// Don't use it as a way to shut up the compiler while you take your
// sweet time getting around to implementing the method.
//
#ifdef __VTK_WRAP__
#define vtkNotUsed(x) x
#else
#define vtkNotUsed(x)
#endif

//
// This macro is used for functions which may not be used in a translation unit
// due to different paths taken based on template types. Please give a reason
// why the function may be considered unused (within a translation unit). For
// example, a template specialization might not be used in compiles of sources
// which use different template types.
//
#ifdef __GNUC__
#define vtkMaybeUnused(reason) __attribute__((unused))
#else
#define vtkMaybeUnused(reason)
#endif

#define vtkWorldCoordinateMacro(name)                                                              \
  virtual vtkCoordinate* Get##name##Coordinate()                                                   \
  {                                                                                                \
    vtkDebugMacro(<< this->GetClassName() << " (" << this                                          \
                  << "): returning " #name "Coordinate address " << this->name##Coordinate);       \
    return this->name##Coordinate;                                                                 \
  }                                                                                                \
  virtual void Set##name(double x[3])                                                              \
  {                                                                                                \
    this->Set##name(x[0], x[1], x[2]);                                                             \
  }                                                                                                \
  virtual void Set##name(double x, double y, double z)                                             \
  {                                                                                                \
    this->name##Coordinate->SetValue(x, y, z);                                                     \
  }                                                                                                \
  virtual double* Get##name() VTK_SIZEHINT(3)                                                      \
  {                                                                                                \
    return this->name##Coordinate->GetValue();                                                     \
  }

#define vtkViewportCoordinateMacro(name)                                                           \
  virtual vtkCoordinate* Get##name##Coordinate()                                                   \
  {                                                                                                \
    vtkDebugMacro(<< this->GetClassName() << " (" << this                                          \
                  << "): returning " #name "Coordinate address " << this->name##Coordinate);       \
    return this->name##Coordinate;                                                                 \
  }                                                                                                \
  virtual void Set##name(double x[2])                                                              \
  {                                                                                                \
    this->Set##name(x[0], x[1]);                                                                   \
  }                                                                                                \
  virtual void Set##name(double x, double y)                                                       \
  {                                                                                                \
    this->name##Coordinate->SetValue(x, y);                                                        \
  }                                                                                                \
  virtual double* Get##name() VTK_SIZEHINT(2)                                                      \
  {                                                                                                \
    return this->name##Coordinate->GetValue();                                                     \
  }

// Allows definition of vtkObject API such that NewInstance may return a
// superclass of thisClass.
#define vtkAbstractTypeMacroWithNewInstanceType(                                                   \
  thisClass, superclass, instanceType, thisClassName)                                              \
protected:                                                                                         \
  const char* GetClassNameInternal() const override                                                \
  {                                                                                                \
    return thisClassName;                                                                          \
  }                                                                                                \
                                                                                                   \
public:                                                                                            \
  typedef superclass Superclass;                                                                   \
  static vtkTypeBool IsTypeOf(const char* type)                                                    \
  {                                                                                                \
    if (!strcmp(thisClassName, type))                                                              \
    {                                                                                              \
      return 1;                                                                                    \
    }                                                                                              \
    return superclass::IsTypeOf(type);                                                             \
  }                                                                                                \
  vtkTypeBool IsA(const char* type) override                                                       \
  {                                                                                                \
    return this->thisClass::IsTypeOf(type);                                                        \
  }                                                                                                \
  static thisClass* SafeDownCast(vtkObjectBase* o)                                                 \
  {                                                                                                \
    if (o && o->IsA(thisClassName))                                                                \
    {                                                                                              \
      return static_cast<thisClass*>(o);                                                           \
    }                                                                                              \
    return nullptr;                                                                                \
  }                                                                                                \
  VTK_NEWINSTANCE instanceType* NewInstance() const                                                \
  {                                                                                                \
    return instanceType::SafeDownCast(this->NewInstanceInternal());                                \
  }                                                                                                \
  static vtkIdType GetNumberOfGenerationsFromBaseType(const char* type)                            \
  {                                                                                                \
    if (!strcmp(thisClassName, type))                                                              \
    {                                                                                              \
      return 0;                                                                                    \
    }                                                                                              \
    return 1 + superclass::GetNumberOfGenerationsFromBaseType(type);                               \
  }                                                                                                \
  vtkIdType GetNumberOfGenerationsFromBase(const char* type) override                              \
  {                                                                                                \
    return this->thisClass::GetNumberOfGenerationsFromBaseType(type);                              \
  }

// Same as vtkTypeMacro, but adapted for cases where thisClass is abstract.
#define vtkAbstractTypeMacro(thisClass, superclass)                                                \
  vtkAbstractTypeMacroWithNewInstanceType(thisClass, superclass, thisClass, #thisClass)            \
                                                                                                   \
public:

// Macro used to determine whether a class is the same class or
// a subclass of the named class.
#define vtkTypeMacro(thisClass, superclass)                                                        \
  vtkAbstractTypeMacro(thisClass, superclass)                                                      \
                                                                                                   \
protected:                                                                                         \
  vtkObjectBase* NewInstanceInternal() const override                                              \
  {                                                                                                \
    return thisClass::New();                                                                       \
  }                                                                                                \
                                                                                                   \
public:

// Macro to use when you are a direct child class of vtkObjectBase, instead
// of vtkTypeMacro. This is required to properly specify NewInstanceInternal
// as a virtual method.
// It is used to determine whether a class is the same class or a subclass
// of the named class.

#define vtkBaseTypeMacro(thisClass, superclass)                                                    \
  vtkAbstractTypeMacro(thisClass, superclass)                                                      \
                                                                                                   \
protected:                                                                                         \
  virtual vtkObjectBase* NewInstanceInternal() const                                               \
  {                                                                                                \
    return thisClass::New();                                                                       \
  }                                                                                                \
                                                                                                   \
public:

// Version of vtkAbstractTypeMacro for when thisClass is templated.
// For templates, we use the compiler generated typeid(...).name() identifier
// to distinguish classes. Otherwise, the template parameter names would appear
// in the class name, rather than the actual parameters. The resulting name may
// not be human readable on some platforms, but it will at least be unique. On
// GCC 4.9.2 release builds, this ends up being the same performance-wise as
// returning a string literal as the name() string is resolved at compile time.
//
// If either class has multiple template parameters, the commas will interfere
// with the macro call. In this case, create a typedef to the multi-parameter
// template class and pass that into the macro instead.
#define vtkAbstractTemplateTypeMacro(thisClass, superclass)                                        \
  vtkAbstractTypeMacroWithNewInstanceType(                                                         \
    thisClass, superclass, thisClass, typeid(thisClass).name())                                    \
                                                                                                   \
public:

// Version of vtkTypeMacro for when thisClass is templated.
// See vtkAbstractTemplateTypeMacro for more info.
#define vtkTemplateTypeMacro(thisClass, superclass)                                                \
  vtkAbstractTemplateTypeMacro(thisClass, superclass);                                             \
                                                                                                   \
protected:                                                                                         \
  vtkObjectBase* NewInstanceInternal() const override                                              \
  {                                                                                                \
    return thisClass::New();                                                                       \
  }                                                                                                \
                                                                                                   \
public:

// NOTE: This is no longer the prefer method for dispatching an array to a
// worker template. See vtkArrayDispatch for the new approach.
//
// The vtkTemplateMacro is used to centralize the set of types
// supported by Execute methods.  It also avoids duplication of long
// switch statement case lists.
//
// This version of the macro allows the template to take any number of
// arguments.  Example usage:
// switch(array->GetDataType())
//   {
//   vtkTemplateMacro(myFunc(static_cast<VTK_TT*>(data), arg2));
//   }
#define vtkTemplateMacroCase(typeN, type, call)                                                    \
  case typeN:                                                                                      \
  {                                                                                                \
    typedef type VTK_TT;                                                                           \
    call;                                                                                          \
  }                                                                                                \
  break
#define vtkTemplateMacro(call)                                                                     \
  vtkTemplateMacroCase(VTK_DOUBLE, double, call);                                                  \
  vtkTemplateMacroCase(VTK_FLOAT, float, call);                                                    \
  vtkTemplateMacroCase(VTK_LONG_LONG, long long, call);                                            \
  vtkTemplateMacroCase(VTK_UNSIGNED_LONG_LONG, unsigned long long, call);                          \
  vtkTemplateMacroCase(VTK_ID_TYPE, vtkIdType, call);                                              \
  vtkTemplateMacroCase(VTK_LONG, long, call);                                                      \
  vtkTemplateMacroCase(VTK_UNSIGNED_LONG, unsigned long, call);                                    \
  vtkTemplateMacroCase(VTK_INT, int, call);                                                        \
  vtkTemplateMacroCase(VTK_UNSIGNED_INT, unsigned int, call);                                      \
  vtkTemplateMacroCase(VTK_SHORT, short, call);                                                    \
  vtkTemplateMacroCase(VTK_UNSIGNED_SHORT, unsigned short, call);                                  \
  vtkTemplateMacroCase(VTK_CHAR, char, call);                                                      \
  vtkTemplateMacroCase(VTK_SIGNED_CHAR, signed char, call);                                        \
  vtkTemplateMacroCase(VTK_UNSIGNED_CHAR, unsigned char, call)

// This is same as Template macro with additional case for VTK_STRING.
#define vtkExtendedTemplateMacro(call)                                                             \
  vtkTemplateMacro(call);                                                                          \
  vtkTemplateMacroCase(VTK_STRING, vtkStdString, call)

// The vtkTemplate2Macro is used to dispatch like vtkTemplateMacro but
// over two template arguments instead of one.
//
// Example usage:
// switch(vtkTemplate2PackMacro(array1->GetDataType(),
//                              array2->GetDataType()))
//   {
//   vtkTemplateMacro(myFunc(static_cast<VTK_T1*>(data1),
//                           static_cast<VTK_T2*>(data2),
//                           otherArg));
//   }
#define vtkTemplate2Macro(call)                                                                    \
  vtkTemplate2MacroCase1(VTK_DOUBLE, double, call);                                                \
  vtkTemplate2MacroCase1(VTK_FLOAT, float, call);                                                  \
  vtkTemplate2MacroCase1(VTK_LONG_LONG, long long, call);                                          \
  vtkTemplate2MacroCase1(VTK_UNSIGNED_LONG_LONG, unsigned long long, call);                        \
  vtkTemplate2MacroCase1(VTK_ID_TYPE, vtkIdType, call);                                            \
  vtkTemplate2MacroCase1(VTK_LONG, long, call);                                                    \
  vtkTemplate2MacroCase1(VTK_UNSIGNED_LONG, unsigned long, call);                                  \
  vtkTemplate2MacroCase1(VTK_INT, int, call);                                                      \
  vtkTemplate2MacroCase1(VTK_UNSIGNED_INT, unsigned int, call);                                    \
  vtkTemplate2MacroCase1(VTK_SHORT, short, call);                                                  \
  vtkTemplate2MacroCase1(VTK_UNSIGNED_SHORT, unsigned short, call);                                \
  vtkTemplate2MacroCase1(VTK_CHAR, char, call);                                                    \
  vtkTemplate2MacroCase1(VTK_SIGNED_CHAR, signed char, call);                                      \
  vtkTemplate2MacroCase1(VTK_UNSIGNED_CHAR, unsigned char, call)
#define vtkTemplate2MacroCase1(type1N, type1, call)                                                \
  vtkTemplate2MacroCase2(type1N, type1, VTK_DOUBLE, double, call);                                 \
  vtkTemplate2MacroCase2(type1N, type1, VTK_FLOAT, float, call);                                   \
  vtkTemplate2MacroCase2(type1N, type1, VTK_LONG_LONG, long long, call);                           \
  vtkTemplate2MacroCase2(type1N, type1, VTK_UNSIGNED_LONG_LONG, unsigned long long, call);         \
  vtkTemplate2MacroCase2(type1N, type1, VTK_ID_TYPE, vtkIdType, call);                             \
  vtkTemplate2MacroCase2(type1N, type1, VTK_LONG, long, call);                                     \
  vtkTemplate2MacroCase2(type1N, type1, VTK_UNSIGNED_LONG, unsigned long, call);                   \
  vtkTemplate2MacroCase2(type1N, type1, VTK_INT, int, call);                                       \
  vtkTemplate2MacroCase2(type1N, type1, VTK_UNSIGNED_INT, unsigned int, call);                     \
  vtkTemplate2MacroCase2(type1N, type1, VTK_SHORT, short, call);                                   \
  vtkTemplate2MacroCase2(type1N, type1, VTK_UNSIGNED_SHORT, unsigned short, call);                 \
  vtkTemplate2MacroCase2(type1N, type1, VTK_CHAR, char, call);                                     \
  vtkTemplate2MacroCase2(type1N, type1, VTK_SIGNED_CHAR, signed char, call);                       \
  vtkTemplate2MacroCase2(type1N, type1, VTK_UNSIGNED_CHAR, unsigned char, call)
#define vtkTemplate2MacroCase2(type1N, type1, type2N, type2, call)                                 \
  case vtkTemplate2PackMacro(type1N, type2N):                                                      \
  {                                                                                                \
    typedef type1 VTK_T1;                                                                          \
    typedef type2 VTK_T2;                                                                          \
    call;                                                                                          \
  };                                                                                               \
  break
#define vtkTemplate2PackMacro(type1N, type2N) ((((type1N)&0xFF) << 8) | ((type2N)&0xFF))

// The vtkArrayIteratorTemplateMacro is used to centralize the set of types
// supported by Execute methods.  It also avoids duplication of long
// switch statement case lists.
//
// This version of the macro allows the template to take any number of
// arguments.
//
// Note that in this macro VTK_TT is defined to be the type of the iterator
// for the given type of array. One must include the
// vtkArrayIteratorIncludes.h header file to provide for extending of this macro
// by addition of new iterators.
//
// Example usage:
// vtkArrayIter* iter = array->NewIterator();
// switch(array->GetDataType())
//   {
//   vtkArrayIteratorTemplateMacro(myFunc(static_cast<VTK_TT*>(iter), arg2));
//   }
// iter->Delete();
//
#define vtkArrayIteratorTemplateMacroCase(typeN, type, call)                                       \
  vtkTemplateMacroCase(typeN, vtkArrayIteratorTemplate<type>, call)
#define vtkArrayIteratorTemplateMacro(call)                                                        \
  vtkArrayIteratorTemplateMacroCase(VTK_DOUBLE, double, call);                                     \
  vtkArrayIteratorTemplateMacroCase(VTK_FLOAT, float, call);                                       \
  vtkArrayIteratorTemplateMacroCase(VTK_LONG_LONG, long long, call);                               \
  vtkArrayIteratorTemplateMacroCase(VTK_UNSIGNED_LONG_LONG, unsigned long long, call);             \
  vtkArrayIteratorTemplateMacroCase(VTK_ID_TYPE, vtkIdType, call);                                 \
  vtkArrayIteratorTemplateMacroCase(VTK_LONG, long, call);                                         \
  vtkArrayIteratorTemplateMacroCase(VTK_UNSIGNED_LONG, unsigned long, call);                       \
  vtkArrayIteratorTemplateMacroCase(VTK_INT, int, call);                                           \
  vtkArrayIteratorTemplateMacroCase(VTK_UNSIGNED_INT, unsigned int, call);                         \
  vtkArrayIteratorTemplateMacroCase(VTK_SHORT, short, call);                                       \
  vtkArrayIteratorTemplateMacroCase(VTK_UNSIGNED_SHORT, unsigned short, call);                     \
  vtkArrayIteratorTemplateMacroCase(VTK_CHAR, char, call);                                         \
  vtkArrayIteratorTemplateMacroCase(VTK_SIGNED_CHAR, signed char, call);                           \
  vtkArrayIteratorTemplateMacroCase(VTK_UNSIGNED_CHAR, unsigned char, call);                       \
  vtkArrayIteratorTemplateMacroCase(VTK_STRING, vtkStdString, call);                               \
  vtkTemplateMacroCase(VTK_BIT, vtkBitArrayIterator, call)

//----------------------------------------------------------------------------
// Deprecation attribute.

#if !defined(VTK_DEPRECATED) && !defined(VTK_WRAPPING_CXX)
#if __cplusplus >= 201402L && defined(__has_cpp_attribute)
#if __has_cpp_attribute(deprecated)
#define VTK_DEPRECATED [[deprecated]]
#endif
#elif defined(_MSC_VER)
#define VTK_DEPRECATED __declspec(deprecated)
#elif defined(__GNUC__) && !defined(__INTEL_COMPILER)
#define VTK_DEPRECATED __attribute__((deprecated))
#endif
#endif

#ifndef VTK_DEPRECATED
#define VTK_DEPRECATED
#endif

//----------------------------------------------------------------------------
// format string checking.

#if !defined(VTK_FORMAT_PRINTF)
#if defined(__GNUC__)
#define VTK_FORMAT_PRINTF(a, b) __attribute__((format(printf, a, b)))
#else
#define VTK_FORMAT_PRINTF(a, b)
#endif
#endif

// Qualifiers used for function arguments and return types indicating that the
// class is wrapped externally.
#define VTK_WRAP_EXTERN

//----------------------------------------------------------------------------
// Switch case fall-through policy.

// Use "VTK_FALLTHROUGH;" to annotate deliberate fall-through in switches,
// use it analogously to "break;".  The trailing semi-colon is required.
#if !defined(VTK_FALLTHROUGH) && defined(__has_cpp_attribute)
#if __cplusplus >= 201703L && __has_cpp_attribute(fallthrough)
#define VTK_FALLTHROUGH [[fallthrough]]
#elif __cplusplus >= 201103L && __has_cpp_attribute(gnu::fallthrough)
#define VTK_FALLTHROUGH [[gnu::fallthrough]]
#elif __cplusplus >= 201103L && __has_cpp_attribute(clang::fallthrough)
#define VTK_FALLTHROUGH [[clang::fallthrough]]
#endif
#endif

#ifndef VTK_FALLTHROUGH
#define VTK_FALLTHROUGH ((void)0)
#endif

//----------------------------------------------------------------------------
// To suppress code with undefined behaviour. Ideally, such code should be fixed
// but sometimes suppression can be useful.

#if defined(__has_attribute)
#if __has_attribute(no_sanitize)
#define VTK_NO_UBSAN __attribute__((no_sanitize("undefined")))
#else
#define VTK_NO_UBSAN
#endif
#else
#define VTK_NO_UBSAN
#endif

//----------------------------------------------------------------------------

// XXX(xcode-8)
// AppleClang first supported thread_local only by Xcode 8, for older Xcodes
// fall back to the non-standard __thread which is equivalent for many, but
// not all, cases.  Notably, it has limitations on variable scope.
#if defined(__apple_build_version__) && (__clang_major__ < 8)
#define VTK_THREAD_LOCAL _Thread_local
#else
#define VTK_THREAD_LOCAL thread_local
#endif

//----------------------------------------------------------------------------
// Macro to generate bitflag operators for C++11 scoped enums.

#define VTK_GENERATE_BITFLAG_OPS(EnumType)                                                         \
  inline EnumType operator|(EnumType f1, EnumType f2)                                              \
  {                                                                                                \
    using T = typename std::underlying_type<EnumType>::type;                                       \
    return static_cast<EnumType>(static_cast<T>(f1) | static_cast<T>(f2));                         \
  }                                                                                                \
  inline EnumType operator&(EnumType f1, EnumType f2)                                              \
  {                                                                                                \
    using T = typename std::underlying_type<EnumType>::type;                                       \
    return static_cast<EnumType>(static_cast<T>(f1) & static_cast<T>(f2));                         \
  }                                                                                                \
  inline EnumType operator^(EnumType f1, EnumType f2)                                              \
  {                                                                                                \
    using T = typename std::underlying_type<EnumType>::type;                                       \
    return static_cast<EnumType>(static_cast<T>(f1) ^ static_cast<T>(f2));                         \
  }                                                                                                \
  inline EnumType operator~(EnumType f1)                                                           \
  {                                                                                                \
    using T = typename std::underlying_type<EnumType>::type;                                       \
    return static_cast<EnumType>(~static_cast<T>(f1));                                             \
  }                                                                                                \
  inline EnumType& operator|=(EnumType& f1, EnumType f2)                                           \
  {                                                                                                \
    using T = typename std::underlying_type<EnumType>::type;                                       \
    return f1 = static_cast<EnumType>(static_cast<T>(f1) | static_cast<T>(f2));                    \
  }                                                                                                \
  inline EnumType& operator&=(EnumType& f1, EnumType f2)                                           \
  {                                                                                                \
    using T = typename std::underlying_type<EnumType>::type;                                       \
    return f1 = static_cast<EnumType>(static_cast<T>(f1) & static_cast<T>(f2));                    \
  }                                                                                                \
  inline EnumType& operator^=(EnumType& f1, EnumType f2)                                           \
  {                                                                                                \
    using T = typename std::underlying_type<EnumType>::type;                                       \
    return f1 = static_cast<EnumType>(static_cast<T>(f1) ^ static_cast<T>(f2));                    \
  }

#endif
// VTK-HeaderTest-Exclude: vtkSetGet.h
