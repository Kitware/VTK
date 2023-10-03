// © Kitware, Inc. See license.md for details.
#ifndef token_Hash_h
#define token_Hash_h

#include "token/Options.h" // for token_NAMESPACE_* macros

#include <cstdint> // for `std::uint*_t`

token_BEGIN_NAMESPACE

/// The storage type for a string hash.
using Hash = std::uint32_t;

/// An invalid hash (that should never exist inside the manager's storage).
/// This value corresponds to the hash computed for an empty string.
inline constexpr Hash Invalid() { return 0x811c9dc5; }

token_CLOSE_NAMESPACE

#endif // token_Hash_h
