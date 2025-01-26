// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkTypeName_h
#define vtkTypeName_h

#include "vtkCxxABIConfigure.h"
#include "vtkStringToken.h" // For tokenized type-name.

#include <cstring>  // For std::strlen.
#include <string>   // For return value.
#include <typeinfo> // For typeid().

#if VTK_HAS_ABI_NAMESPACE
#define VTK_TYPENAME_STRINGIFY_INTERNAL(x) #x
#define VTK_TYPENAME_STRINGIFY(x) VTK_TYPENAME_STRINGIFY_INTERNAL(x)
#define VTK_ABI_NAMESPACE_STRING VTK_TYPENAME_STRINGIFY(VTK_ABI_NAMESPACE_NAME) "::"
#endif

namespace vtk
{
namespace detail
{
VTK_ABI_NAMESPACE_BEGIN

template <typename ObjectType>
struct Name
{
  static std::string value()
  {
    std::string result = typeid(ObjectType).name();
#ifdef VTK_HAS_CXXABI_DEMANGLE
    int status = 0;
    std::size_t size = 0;
    char* demangledSymbol = abi::__cxa_demangle(result.c_str(), nullptr, &size, &status);
    if (!status && size > 0)
    {
      result = demangledSymbol;
    }
    free(demangledSymbol);
#endif

    // Now that we have a (probably) demangled symbol, we need to remove
    // MSVC-specific cruft from the symbol name.
#ifdef VTK_COMPILER_MSVC
    // MSVC returns a name with "class " or "struct " prepended. Remove it
    // for consistency with other platforms. Note that template parameters
    // also include "class " or "struct ", so we must search and replace
    // repeatedly.
    for (std::string::size_type pos = result.find("class "); pos != std::string::npos;
         pos = result.find("class ", pos + 1))
    {
      result = result.substr(0, pos) + result.substr(pos + 6);
    }
    for (std::string::size_type pos = result.find("struct "); pos != std::string::npos;
         pos = result.find("struct ", pos + 1))
    {
      result = result.substr(0, pos) + result.substr(pos + 7);
    }
    // MSVC reports anonymous namespaces like so: `anonymous namespace'
    // while others report them like so: (anonymous namespace). Fix it
    // to be consistent.
    for (std::string::size_type pos = result.find("`anonymous namespace'");
         pos != std::string::npos; pos = result.find("`anonymous namespace'", pos + 1))
    {
      result = result.substr(0, pos) + "(anonymous namespace)" + result.substr(pos + 21);
    }
    // MSVC does not include spaces after commas separating template
    // parameters. Add it in:
    for (std::string::size_type pos = result.find(','); pos != std::string::npos;
         pos = result.find(',', pos + 1))
    {
      result = result.substr(0, pos) + ", " + result.substr(pos + 1);
    }
#endif
    // Finally, vtkABINamespace.h provides a namespace name, we will strip it
    // from the name. This is done to avoid burdening developers with string
    // processing when linking to multiple versions of VTK.
#if VTK_HAS_ABI_NAMESPACE
    const std::size_t nsLen = std::strlen(VTK_ABI_NAMESPACE_STRING);
    for (std::string::size_type pos = result.find(VTK_ABI_NAMESPACE_STRING);
         pos != std::string::npos; pos = result.find(VTK_ABI_NAMESPACE_STRING, pos + 1))
    {
      result = result.substr(0, pos) + result.substr(pos + nsLen);
    }
#endif
    return result;
  }

  /// Return an integer hash of the ObjectType's typename.
  ///
  /// This method should become constexpr once VTK allows c++14
  /// extensions, but because MSVC requires so much string
  /// manipulation, it is not possible until local variables
  /// are allowed in constexpr functions.
  static vtkStringToken::Hash token()
  {
    auto nameStr = Name<ObjectType>::value();
    auto result = vtkStringToken::StringHash(nameStr.c_str(), nameStr.size());
    return result;
  }
};

VTK_ABI_NAMESPACE_END
} // namespace detail

VTK_ABI_NAMESPACE_BEGIN
/**
 * Return the demangled type-name of the provided \a ObjectType.
 *
 * Note that if the `<cxxabi.h>` header is not present or does
 * not provide `abi::__cxa_demangle()`, a mangled name will be
 * returned.
 */
template <typename ObjectType>
inline std::string TypeName()
{
  return detail::Name<ObjectType>::value();
}

/**
 * Return a string token holding a hash of the demangled
 * type-name of the provided \a ObjectType.
 *
 * Note that this function should become constexpr, so the
 * hash will be computed at compile time (once VTK allows
 * c++14 extensions). Because of this, the string for the
 * hash cannot be added to the vtkStringManager and thus
 * may cause exceptions later.
 */
template <typename ObjectType>
inline vtkStringToken TypeToken()
{
  return vtkStringToken(detail::Name<ObjectType>::token());
}

VTK_ABI_NAMESPACE_END
} // namespace vtk

#endif // vtkTypeName_h

// VTK-HeaderTest-Exclude: vtkTypeName.h
