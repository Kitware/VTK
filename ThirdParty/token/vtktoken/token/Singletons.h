// © Kitware, Inc. See license.md for details.
#ifndef token_Singletons_h
#define token_Singletons_h

#include "token/TypeContainer.h" // For API and exports.

token_BEGIN_NAMESPACE

/// Return a container of singleton objects indexed by their type.
///
/// Because the index is based on the type of the object being
/// contained (it is a checksum computed on the typename-string),
/// there can be only zero or one objects of a given type in the
/// container.
TOKEN_EXPORT TypeContainer& singletons();

/// Destroy the container holding all registered singleton objects.
///
/// The destructors of any contained objects will be called.
/// This function is invoked at exit, but if your application
/// needs to ensure objects are released before the other
/// destructors are called (since no ordering is guaranteed for
/// statically-allocated objects), you may call this at any time.
///
/// Libraries should not invoke this function; if your library
/// uses this singleton container, your code run at exit should
/// simply remove any stored objects rather than forcing all of
/// the application's singletons to be destroyed.
TOKEN_EXPORT void finalizeSingletons();

namespace detail {

// Implementation detail for Schwarz counter idiom.
class TOKEN_EXPORT singletonsCleanup
{
public:
  singletonsCleanup();
  ~singletonsCleanup();

private:
  singletonsCleanup(const singletonsCleanup& other) = delete;
  singletonsCleanup& operator=(const singletonsCleanup& rhs) = delete;
};

static singletonsCleanup singletonsCleanupInstance;

} // namespace detail

token_CLOSE_NAMESPACE

#endif // token_Singletons
