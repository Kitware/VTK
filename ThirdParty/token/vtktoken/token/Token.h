// © Kitware, Inc. See license.md for details.
#ifndef token_Token_h
#define token_Token_h

#include "token/Hash.h" // for token_NAMESPACE_* macros
#include "token/Exports.h" // for export macro
#include "token/Compiler.h" // for token_SPACE_BEFORE_SUFFIX

#include <cstdint> // for `std::uint*_t`
#include <iostream> // for `std::ostream`
#include <memory> // for `std::shared_ptr<Manager>`
#include <string> // for `std::string`

token_BEGIN_NAMESPACE

class Manager;

/**
 * @class   Token
 * @brief   Represent a string by its integer hash.
 *
 * This class is a lightweight object for representing a string as a
 * 32-bit integer token.  Tokens can be constructed at compile-time
 * (via the ""_token string-literal operator below) or run-time (via
 * the constructor).
 *
 * Equality comparisons are simple integer tests, while
 * inequality operators attempt to locate the original source
 * strings and compare them alphanumerically to preserve
 * lexicographic ordering.
 *
 * This class can be used inside ordered and unordered STL containers,
 * but unordered containers will be fast and ordered containers will
 * be slow.
 */

class TOKEN_EXPORT Token
{
public:
  /// Construct a token from a string literal.
  Token(const char* data = nullptr, std::size_t size = std::string::npos);
  /// Construct a token from a std::string.
  Token(const std::string& data);
  /// Construct a token given its hash value.
  /// NOTE: This will NOT insert a string into the manager as other constructors do.
  inline constexpr Token(Hash tokenId) noexcept
    : m_id(tokenId)
  {
  }

  /// Return the token's ID (usually its hash but possibly not in the case of collisions).
  Hash getId() const { return m_id; }
  /// Return the string corresponding to the token.
  const std::string& data() const;

  /// Return true if the hash has been initialized; false otherwise.
  bool valid() const { return m_id != Invalid(); }
  /// Return true if there is a non-empty string for the ID in this token.
  bool hasData() const;

  /// Fast equality comparison (compares hashes, not strings).
  bool operator==(const Token& other) const;
  /// Fast inequality comparison (compares hashes, not strings).
  bool operator!=(const Token& other) const;

  /// Slow, but unsurprising string comparison (preserves lexical string ordering).
  bool operator<(const Token& other) const;
  bool operator>(const Token& other) const;
  bool operator<=(const Token& other) const;
  bool operator>=(const Token& other) const;

  /// Return the hash of a string
  /// This is used internally but also by the ""_token() literal operator
  inline static constexpr Hash stringHash(const char* data, std::size_t size) noexcept
  {
    return Token::hash_32_fnv1a_const(data, size);
  }

  /// Return the database of strings and their tokens (hashes).
  static Manager* getManager();

protected:
  Hash m_id;
  static std::shared_ptr<Manager> m_manager;

  static Manager* getManagerInternal();

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

namespace literals
{

/// Construct the hash of a string literal, like so:
///
/// ```c++
/// using namespace token::literals;
/// Hash t = "test"_hash;
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
#if token_SPACE_BEFORE_SUFFIX
inline constexpr TOKEN_EXPORT Hash operator"" _hash(
  const char* data, std::size_t size)
#else
inline constexpr TOKEN_EXPORT Hash operator""_hash(
  const char* data, std::size_t size)
#endif
{
  return Token::stringHash(data, size);
}

/// Construct a token from a string literal, like so:
///
/// ```c++
/// using namespace token::literals;
/// Token t = "test"_token;
/// std::cout << t.id() << "\n"; // Prints a hash-code.
/// // std::cout << t.value() << "\n"; // Prints "test" if someone else constructed the token from a
/// ctor; else throws exception.
/// ```
#if token_SPACE_BEFORE_SUFFIX
inline constexpr TOKEN_EXPORT Token operator"" _token(
  const char* data, std::size_t size)
#else
inline constexpr TOKEN_EXPORT Token operator""_token(
  const char* data, std::size_t size)
#endif
{
  return Token(Token::stringHash(data, size));
}

} // namespace literals

bool TOKEN_EXPORT operator==(const std::string& a, const Token& b);
bool TOKEN_EXPORT operator!=(const std::string& a, const Token& b);
bool TOKEN_EXPORT operator>(const std::string& a, const Token& b);
bool TOKEN_EXPORT operator<(const std::string& a, const Token& b);
bool TOKEN_EXPORT operator>=(const std::string& a, const Token& b);
bool TOKEN_EXPORT operator<=(const std::string& a, const Token& b);

bool TOKEN_EXPORT operator==(const Token& a, const std::string& b);
bool TOKEN_EXPORT operator!=(const Token& a, const std::string& b);
bool TOKEN_EXPORT operator>(const Token& a, const std::string& b);
bool TOKEN_EXPORT operator<(const Token& a, const std::string& b);
bool TOKEN_EXPORT operator>=(const Token& a, const std::string& b);
bool TOKEN_EXPORT operator<=(const Token& a, const std::string& b);

bool TOKEN_EXPORT operator==(const char* a, const Token& b);
bool TOKEN_EXPORT operator!=(const char* a, const Token& b);
bool TOKEN_EXPORT operator>(const char* a, const Token& b);
bool TOKEN_EXPORT operator<(const char* a, const Token& b);
bool TOKEN_EXPORT operator>=(const char* a, const Token& b);
bool TOKEN_EXPORT operator<=(const char* a, const Token& b);

bool TOKEN_EXPORT operator==(const Token& a, const char* b);
bool TOKEN_EXPORT operator!=(const Token& a, const char* b);
bool TOKEN_EXPORT operator>(const Token& a, const char* b);
bool TOKEN_EXPORT operator<(const Token& a, const char* b);
bool TOKEN_EXPORT operator>=(const Token& a, const char* b);
bool TOKEN_EXPORT operator<=(const Token& a, const char* b);

token_CLOSE_NAMESPACE

namespace std
{
/// Tokens provide a specialization of std::hash so they can be used in unordered
/// containers.
template <>
struct TOKEN_EXPORT hash<token_NAMESPACE::Token>
{
  std::size_t operator()(const token_NAMESPACE::Token& t) const { return t.getId(); }
};
} // namespace std

#endif // Token_h
