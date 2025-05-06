#include <viskores/thirdparty/diy/Configure.h>

#if VISKORES_USE_EXTERNAL_DIY
#define VISKORES_DIY_INCLUDE(header) <diy/header>
#else
#define VISKORES_DIY_INCLUDE(header) <viskoresdiy/header>
#define diy viskoresdiy // mangle namespace diy
#endif

#if defined(VISKORES_CLANG) || defined(VISKORES_GCC)
#pragma GCC visibility push(default)
#endif

// clang-format off
VISKORES_THIRDPARTY_PRE_INCLUDE
