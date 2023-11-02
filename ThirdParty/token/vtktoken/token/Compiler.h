// © Kitware, Inc. See license.md for details.
#ifndef token_Compiler_h
#define token_Compiler_h

#if defined(_MSC_VER)
#  define token_COMPILER MSC
#  define token_COMPILER_MSC 1
#  define token_SPACE_BEFORE_SUFFIX 0
#elif defined(__INTEL_COMPILER)
#  define token_COMPILER INTEL
#  define token_COMPILER_INTEL 1
#  define token_SPACE_BEFORE_SUFFIX 0
#elif defined (__PGI)
#  define token_COMPILER PGI
#  define token_COMPILER_PGI 1
#  define token_SPACE_BEFORE_SUFFIX 0
#elif defined (__clang__)
#  define token_COMPILER CLANG
#  define token_COMPILER_CLANG 1
#  define token_SPACE_BEFORE_SUFFIX 0
#elif defined(__GNUC__)
#  define token_COMPILER GCC
#  define token_COMPILER_GCC 1
/* Several compilers pretend to be GCC but have minor differences. To
 * compensate for that, we checked for those compilers first above. */
#  define token_GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

// gcc 4.8.5 and earlier require a space after a string-literal-operator's double-quotes
// and before the name of the operator. C++23 deprecates a space between them. Test which
// we need to make things work.
#  if token_GCC_VERSION <= 40805
#    define token_SPACE_BEFORE_SUFFIX 1
#  else
#    define token_SPACE_BEFORE_SUFFIX 0
#  endif
#else
#  define token_COMPILER UNKNOWN
#  define token_COMPILER_UNKNOWN 1
#  define token_SPACE_BEFORE_SUFFIX 0
#endif

#if __cplusplus >= 201402L || (defined(token_COMPILER_MSC) && _MSC_VER >= 1910)
#define token_HAVE_CXX_14
#endif

// Issue:
// Dynamic cast is not just based on the name of the class, but also the
// combined visibility of the class on OSX. When building the hash_code of
// an object the symbol visibility controls of the type are taken into
// consideration (including symbol vis of template parameters). Therefore, if a
// class has a component with private/hidden vis then it cannot be passed across
// library boundaries.
//
// Solution:
// The solution is fairly simple, but annoying. You need to mark template
// classes intended for use in dynamic_cast with appropropriate visibility
// settings.
//
// TL;DR:
// This markup is used when we want to make sure:
//  - The class can be compiled into multiple libraries and at runtime will
//    resolve to a single type instance
//  - Be a type ( or component of a types signature ) that can be passed between
//    dynamic libraries and requires RTTI support ( dynamic_cast ).
#if defined(token_COMPILER_MSC)
#define TOKEN_ALWAYS_EXPORT
#else
#define TOKEN_ALWAYS_EXPORT __attribute__((visibility("default")))
#endif

#endif // token_Compiler_h
