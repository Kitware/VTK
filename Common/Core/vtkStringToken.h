// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkStringToken_h
#define vtkStringToken_h
/**
 * @class   vtkStringToken
 * @brief   Represent a string by its integer hash.
 *
 * This class does not inherit vtkObject; it is a lightweight
 * object for representing a string as a 32-bit integer token.
 * Tokens can be constructed at compile-time (via the ""_token
 * string-literal operator below) or run-time (via the constructor).
 *
 * Equality comparisons are simple integer tests, while
 * inequality operators attempt to locate the original source
 * strings and compare them alphanumerically to preserve
 * lexicographic ordering.
 *
 * This class can be used inside ordered and unordered
 * STL containers.
 */

#include "vtkSmartPointer.h"

#include <cstdint> // for `std::uint*_t`

VTK_ABI_NAMESPACE_BEGIN
class vtkStringManager;

class VTKCOMMONCORE_EXPORT vtkStringToken
{
public:
  using Hash = std::uint32_t;

  /// Construct a token from a string literal.
  vtkStringToken(const char* data = nullptr, std::size_t size = std::string::npos);
  /// Construct a token from a std::string.
  vtkStringToken(const std::string& data);
  /// Construct a token given its hash value.
  /// NOTE: This will NOT insert a string into the manager as other constructors do.
  inline constexpr vtkStringToken(Hash tokenId) noexcept
    : Id(tokenId)
  {
  }

  /// Return the token's ID (usually its hash but possibly not in the case of collisions).
  Hash GetId() const { return this->Id; }
  /// Return the string corresponding to the token.
  const std::string& Data() const;

  /// Fast equality comparison (compares hashes, not strings).
  bool operator==(const vtkStringToken& other) const;
  /// Fast inequality comparison (compares hashes, not strings).
  bool operator!=(const vtkStringToken& other) const;

  /// Slow, but unsurprising string comparison (preserves lexical string ordering).
  bool operator<(const vtkStringToken& other) const;
  bool operator>(const vtkStringToken& other) const;
  bool operator<=(const vtkStringToken& other) const;
  bool operator>=(const vtkStringToken& other) const;

  /// Return the hash of a string
  /// This is used internally but also by the ""_token() literal operator
  inline static constexpr Hash StringHash(const char* data, std::size_t size) noexcept
  {
    return vtkStringToken::hash_32_fnv1a_const(data, size);
  }

  /// Return the database of strings and their tokens (hashes).
  static const vtkStringManager* GetManager();

protected:
  Hash Id;
  static vtkSmartPointer<vtkStringManager> Manager;

  static vtkStringManager* GetManagerInternal();

  // ----
  // Adapted from https://notes.underscorediscovery.com/constexpr-fnv1a/index.html
  // which declared the source as public domain or equivalent. Retrieved on 2022-07-22.
  static constexpr uint32_t hash32a_const = 0x811c9dc5;
  static constexpr uint32_t hash32b_const = 0x1000193;
  static constexpr uint64_t hash64a_const = 0xcbf29ce484222325;
  static constexpr uint64_t hash64b_const = 0x100000001b3;

  // Compute a 32-bit hash of a string.
  // Unlike the original, this version handles embedded null characters so that
  // unicode multi-byte sequences can be hashed.
  inline static constexpr uint32_t hash_32_fnv1a_const(
    const char* const str, std::size_t size, const uint32_t value = hash32a_const) noexcept
  {
    return (!str || size <= 0)
      ? value
      : hash_32_fnv1a_const(&str[1], size - 1, (value ^ uint32_t(str[0])) * hash32b_const);
  }

#if 0
  // Compute a 64-bit hash of a string.
  inline static constexpr uint64_t hash_64_fnv1a_const(
    const char* const str, std::size_t size, const uint64_t value = hash64a_const) noexcept
  {
    return (!str || size <= 0) ? value :
      hash_64_fnv1a_const(&str[1], size - 1, (value ^ uint64_t(str[0])) * hash64b_const);
  }
#endif
};
VTK_ABI_NAMESPACE_END

#ifndef __VTK_WRAP__
namespace vtk
{
namespace literals
{
VTK_ABI_NAMESPACE_BEGIN

/// Construct the hash of a string literal, like so:
///
/// ```c++
/// using namespace vtk::literals;
/// vtkStringToken::Hash t = "test"_hash;
/// bool ok = false;
/// switch (t)
/// {
///   case "foo"_hash: ok = false;
///   case "test"_hash: ok = true;
/// }
/// std::cout << t << "\n"; // Prints a hash-code.
/// std::cout << ok << "\n"; // Prints a true value.
/// ```
///
/// As the example above shows, it is possible to use
/// hashed strings in switch statements since the hashing
/// occurs at build time. This is more efficient than
/// a sequence of if-conditionals performing string
/// comparisons.
inline constexpr VTKCOMMONCORE_EXPORT vtkStringToken::Hash operator"" _hash(
  const char* data, std::size_t size)
{
  return vtkStringToken::StringHash(data, size);
}

/// Construct a token from a string literal, like so:
///
/// ```c++
/// using namespace vtk::literals;
/// vtkStringToken t = "test"_token;
/// std::cout << t.id() << "\n"; // Prints a hash-code.
/// // std::cout << t.value() << "\n"; // Prints "test" if someone else constructed the token from a
/// ctor; else throws exception.
/// ```
inline constexpr VTKCOMMONCORE_EXPORT vtkStringToken operator"" _token(
  const char* data, std::size_t size)
{
  return vtkStringToken(vtkStringToken::StringHash(data, size));
}

VTK_ABI_NAMESPACE_END
} // namespace literals
} // namespace vtk
#endif // __VTK_WRAP__

VTK_ABI_NAMESPACE_BEGIN
bool VTKCOMMONCORE_EXPORT operator==(const std::string& a, const vtkStringToken& b);
bool VTKCOMMONCORE_EXPORT operator!=(const std::string& a, const vtkStringToken& b);
bool VTKCOMMONCORE_EXPORT operator>(const std::string& a, const vtkStringToken& b);
bool VTKCOMMONCORE_EXPORT operator<(const std::string& a, const vtkStringToken& b);
bool VTKCOMMONCORE_EXPORT operator>=(const std::string& a, const vtkStringToken& b);
bool VTKCOMMONCORE_EXPORT operator<=(const std::string& a, const vtkStringToken& b);

bool VTKCOMMONCORE_EXPORT operator==(const vtkStringToken& a, const std::string& b);
bool VTKCOMMONCORE_EXPORT operator!=(const vtkStringToken& a, const std::string& b);
bool VTKCOMMONCORE_EXPORT operator>(const vtkStringToken& a, const std::string& b);
bool VTKCOMMONCORE_EXPORT operator<(const vtkStringToken& a, const std::string& b);
bool VTKCOMMONCORE_EXPORT operator>=(const vtkStringToken& a, const std::string& b);
bool VTKCOMMONCORE_EXPORT operator<=(const vtkStringToken& a, const std::string& b);

bool VTKCOMMONCORE_EXPORT operator==(const char* a, const vtkStringToken& b);
bool VTKCOMMONCORE_EXPORT operator!=(const char* a, const vtkStringToken& b);
bool VTKCOMMONCORE_EXPORT operator>(const char* a, const vtkStringToken& b);
bool VTKCOMMONCORE_EXPORT operator<(const char* a, const vtkStringToken& b);
bool VTKCOMMONCORE_EXPORT operator>=(const char* a, const vtkStringToken& b);
bool VTKCOMMONCORE_EXPORT operator<=(const char* a, const vtkStringToken& b);

bool VTKCOMMONCORE_EXPORT operator==(const vtkStringToken& a, const char* b);
bool VTKCOMMONCORE_EXPORT operator!=(const vtkStringToken& a, const char* b);
bool VTKCOMMONCORE_EXPORT operator>(const vtkStringToken& a, const char* b);
bool VTKCOMMONCORE_EXPORT operator<(const vtkStringToken& a, const char* b);
bool VTKCOMMONCORE_EXPORT operator>=(const vtkStringToken& a, const char* b);
bool VTKCOMMONCORE_EXPORT operator<=(const vtkStringToken& a, const char* b);
VTK_ABI_NAMESPACE_END

namespace std
{
/// vtkStringTokens provide a specialization of std::hash so they can be used in unordered
/// containers.
template <>
struct VTKCOMMONCORE_EXPORT hash<vtkStringToken>
{
  std::size_t operator()(const vtkStringToken& t) const { return t.GetId(); }
};
} // namespace std
#endif // vtkStringToken_h
