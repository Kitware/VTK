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

#include <token/Token.h>

#include "vtkCompiler.h"     // for VTK_COMPILER_GCC
#include "vtkSmartPointer.h" // for ivar

//clang-format off
#include <vtk_nlohmannjson.h>
#include VTK_NLOHMANN_JSON(json.hpp)
//clang-format on

#include <cstdint>       // for `std::uint*_t`
#include <unordered_set> // for membership API

VTK_ABI_NAMESPACE_BEGIN

class VTKCOMMONCORE_EXPORT vtkStringToken
{
public:
  using Hash = std::uint32_t;

  /// Construct a token from a string literal.
  VTK_WRAPEXCLUDE vtkStringToken(const char* data = nullptr, std::size_t size = std::string::npos);
  /// Construct a token from a std::string.
  vtkStringToken(const std::string& data);
  /// Construct a token given its hash value.
  /// NOTE: This will NOT insert a string into the manager as other constructors do.
  constexpr vtkStringToken(Hash tokenId) noexcept
    : Id(tokenId)
  {
  }

  /// Return the token's ID (usually its hash but possibly not in the case of collisions).
  Hash GetId() const { return this->Id; }
  /// A Python-wrappable (but less strongly typed) alternative to GetId()
  unsigned int GetHash() const { return static_cast<unsigned int>(this->Id); }
  /// Return the string corresponding to the token.
  const std::string& Data() const;

  /// Return whether the token is valid or not.
  ///
  /// Valid tokens are those whose hash is not equal to the hash of an empty string.
  bool IsValid() const;
  /// Return whether a string is available for the token's hash ID.
  bool HasData() const;

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
  static constexpr Hash StringHash(const char* data, std::size_t size) noexcept
  {
    return token_NAMESPACE::Token::stringHash(data, size);
  }

  /// Return the hash code used to indicate an invalid (empty) token.
  static Hash InvalidHash();

  /// Methods to manage groups of tokens underneath a parent.
  ///
  /// Grouping tokens provides applications a way to create and discover
  /// dynamic enumerations or sets of strings.
  /// For example, you might tokenize `car` and add tokens for car parts
  /// such `body`, `motor`, and `wheels` as children of `car`.
  /// Another library might then add more children such as `windshield`
  /// and `hood`; the application can present all of these children by
  /// asking for all the children of `car`.
  ///@{
  /// Add \a member as a child of this token.
  ///
  /// If true is returned, the \a member was added.
  /// If false is returned, the \a member was already part of the group
  /// or one of the tokens (this token or \a member) was invalid.
  bool AddChild(vtkStringToken member);
  /// Remove a \a member from this token's children.
  ///
  /// If this returns false, \a member was not a child of this token.
  bool RemoveChild(vtkStringToken member);
  /// Return all the children of this token.
  std::unordered_set<vtkStringToken> Children(bool recursive = true);
  /// Return all the tokens that have children.
  static std::unordered_set<vtkStringToken> AllGroups();
  ///@}

protected:
  Hash Id;
};

/// Convert a JSON value into a string token.
///
/// The value \a jj must be an integer or string.
/// If it is a string, it will be hashed into a token.
/// If it is an integer, it will be treated as a hash.
inline void VTKCOMMONCORE_EXPORT from_json(const nlohmann::json& jj, vtkStringToken& tt)
{
  if (jj.is_number_integer())
  {
    tt = vtkStringToken(jj.get<vtkStringToken::Hash>());
  }
  else if (jj.is_string())
  {
    tt = jj.get<std::string>();
  }
  else
  {
    throw std::runtime_error("String tokens must be JSON integers or strings.");
  }
}

/// Convert a string token into a JSON value.
///
/// If \a tt has a valid string, \a jj will be set to the string.
/// Otherwise, \a jj will be set to the integer hash held by \a tt.
inline void VTKCOMMONCORE_EXPORT to_json(nlohmann::json& jj, const vtkStringToken& tt)
{
  if (tt.HasData())
  {
    jj = tt.Data();
  }
  else
  {
    jj = tt.GetId();
  }
}

VTK_ABI_NAMESPACE_END

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
constexpr VTKCOMMONCORE_EXPORT vtkStringToken::Hash operator""_hash(
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
constexpr VTKCOMMONCORE_EXPORT vtkStringToken operator""_token(const char* data, std::size_t size)
{
  return vtkStringToken(vtkStringToken::StringHash(data, size));
}

VTK_ABI_NAMESPACE_END
} // namespace literals
} // namespace vtk

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
