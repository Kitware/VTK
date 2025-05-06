#undef VISKORES_DIY_INCLUDE
VISKORES_THIRDPARTY_POST_INCLUDE
// clang-format on

#if defined(VISKORES_CLANG) || defined(VISKORES_GCC)
#pragma GCC visibility pop
#endif

// When using an external DIY
// We need to alias the diy namespace to
// viskoresdiy so that Viskores uses it properly
#if VISKORES_USE_EXTERNAL_DIY
namespace viskoresdiy = ::diy;

#else
// The aliasing approach fails for when we
// want to use an internal version since
// the diy namespace already points to the
// external version. Instead we use macro
// replacement to make sure all diy classes
// are placed in viskoresdiy placed
#undef diy // mangle namespace diy

#endif
