##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

#Currently all we are going to build is a set of options that are possible
#based on the compiler. For now we are going on the presumption
#that x86 architecture is the only target for vectorization and therefore
#we don't need any system detect.
#
#Here is the breakdown of what each flag type means:
#
#  1. none:
#  Do not explicitly enable vectorization, but at the same don't explicitly disable
#  vectorization.
#
#  2. native:
#  Allow the compiler to use auto-detection based on the systems CPU to determine
#  the highest level of vectorization support that is allowed. This means that
#  libraries and executables built with this setting are non-portable.
#
#  3. avx
#  Compile with just AVX enabled, no AVX2 or AVX512 vectorization will be used.
#  This means that Sandy Bridge, Ivy Bridge, Haswell, and Skylake are supported,
#  but Haswell and newer will not use any AVX2 instructions
#
#  4. avx2
#  Compile with  AVX2/AVX enabled, no AVX512 vectorization will be used.
#  This means that Sandy Bridge, and Ivy Bridge can not run the code.
#
#  5. avx512
#  Compile with AVX512/AVX2/AVX options enabled.
#  This means that Sandy Bridge, Ivy Bridge, Haswell and can not run the code.
#  Only XeonPhi Knights Landing and Skylake processors can run the code.
#
#  AVX512 is designed to mix with avx/avx2 without any performance penalties,
#  so we enable AVX2 so that we get AVX2 support for < 32bit value types which
#  AVX512 has less support for
#
#
# I wonder if we should go towards a per platform cmake include that stores
# all this knowledge
#   include(gcc.cmake)
#   include(icc.cmake)
#   include(clang.cmake)
#
# This way we could also do compile warning flag detection at the same time
#
#
# Note: By default we use 'native' as the default option
#
#

# guard against building vectorization_flags more than once
if(TARGET viskores_vectorization_flags)
  return()
endif()

add_library(viskores_vectorization_flags INTERFACE)
set_target_properties(
  viskores_vectorization_flags
  PROPERTIES
  EXPORT_NAME vectorization_flags
  )

if(NOT Viskores_INSTALL_ONLY_LIBRARIES)
  install(TARGETS viskores_vectorization_flags EXPORT ${Viskores_EXPORT_NAME})
endif()

# We currently don't know the vectorization flags for MSVC. But we want
# the viskores_vectorization_flags target to exist so we have a consistent
# interface.
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR
   CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
  return()
endif()

set(vec_levels none native)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  #for now we presume gcc >= 5.4
  list(APPEND vec_levels avx avx2)

  #common flags for the avx and avx2 instructions for the gcc compiler
  set(native_flags -march=native)
  if(CMAKE_SYSTEM_PROCESSOR STREQUAL "ppc64le")
    #GCC PowerPC font end doesn't support the march flag
    set(native_flags -mcpu=native -mtune=native)
  endif()

  set(avx_flags -mavx)
  set(avx2_flags ${avx_flags} -mf16c -mavx2 -mfma -mlzcnt -mbmi -mbmi2)
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.1)
    #if GNU is less than 5.1 you get avx, avx2, and some avx512
    list(APPEND vec_levels avx512-knl)
    set(knl_flags ${avx2_flags} -mavx512f -mavx512pf -mavx512er -mavx512cd)
  elseif(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8.0)
    #if GNU is 5.1+ to 8.0 we explicit set the flags as they
    #only have the concept of knl architecture and not skylake
    list(APPEND vec_levels avx512-skx avx512-knl)
    set(knl_flags ${avx2_flags} -mavx512f -mavx512pf -mavx512er -mavx512cd)
    set(skylake_flags ${avx2_flags} -mavx512f -mavx512dq -mavx512cd -mavx512bw -mavx512vl)
  else()
    #if GNU is 8+ has architecture names
    list(APPEND vec_levels avx512-skx avx512-knl)
    set(knl_flags -march=knl)
    set(skylake_flags -march=skylake-avx512)
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  list(APPEND vec_levels avx avx2 avx512-skx avx512-knl)
  set(native_flags -march=native)
  set(avx_flags -mavx)
  set(avx2_flags ${avx_flags} -mf16c -mavx2 -mfma -mlzcnt -mbmi -mbmi2)
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.0)
    set(knl_flags ${avx2_flags} -avx512f -avx512cd -avx512dq -avx512bw -avx512vl)
    set(skylake_flags ${avx2_flags} -avx512f -avx512cd -avx512er -avx512pf)
  else()
    # Clang 4+ introduced skylake-avx512  and knl platforms
    set(knl_flags -march=knl)
    set(skylake_flags -march=skylake-avx512)
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  list(APPEND vec_levels avx avx2)
  set(native_flags -march=native)
  set(avx_flags -mavx)
  set(avx2_flags ${avx_flags} -mf16c -mavx2 -mfma -mlzcnt -mbmi -mbmi2)
  if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 9.0)
    # Manually verified that skylake-avx512 exists in XCode 9.1. Could exist
    # in older versions that we dont have access to.
    list(APPEND vec_levels avx512-skx)
    set(skylake_flags -march=skylake-avx512)
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "PGI")
  #I can't find documentation to explicitly state the level of vectorization
  #support I want from the PGI compiler
  #so for now we are going to do nothing
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
  #Intel 15.X is the first version with avx512
  #Intel 16.X has way better vector generation compared to 15.X though

  set(native_flags -xHost)
  set(avx_flags  -xAVX)
  set(avx2_flags -xCORE-AVX2)

  if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 15.0)
    message(STATUS "While Intel ICC 14.0 and lower support #pragma simd")
    message(STATUS "The code that is generated in testing has caused SIGBUS")
    message(STATUS "runtime signals to be thrown. We recommend you upgrade")
    message(STATUS "or disable vectorization.")
  elseif (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 16.0)
    list(APPEND vec_levels avx avx2)
  else()
    list(APPEND vec_levels avx avx2 avx512-skx avx512-knl)
    set(knl_flags -xMIC-AVX512)
    set(skylake_flags -xCORE-AVX512)
  endif()
endif()

set_property(GLOBAL PROPERTY Viskores_NATIVE_FLAGS ${native_flags})
set_property(GLOBAL PROPERTY Viskores_AVX_FLAGS ${avx_flags})
set_property(GLOBAL PROPERTY Viskores_AVX2_FLAGS ${avx2_flags})

set_property(GLOBAL PROPERTY Viskores_KNL_FLAGS ${knl_flags})
set_property(GLOBAL PROPERTY Viskores_SKYLAKE_FLAGS ${skylake_flags})

# Now that we have set up what levels the compiler lets setup the CMake option
# We use a combo box style property, so that ccmake and cmake-gui have a
# nice interface
#
set(Viskores_Vectorization "none" CACHE INTERNAL "Level of compiler vectorization support")
set_property(CACHE Viskores_Vectorization PROPERTY STRINGS ${vec_levels})
if (NOT ${Viskores_Vectorization} STREQUAL "none")
  set(VISKORES_VECTORIZATION_ENABLED "ON")
endif()

#
# Now that we have set up the options, lets setup the compile flags that
# we are going to require.
#
set(flags)
if(Viskores_Vectorization STREQUAL "native")
  get_property(flags GLOBAL PROPERTY Viskores_NATIVE_FLAGS)
elseif(Viskores_Vectorization STREQUAL "avx")
  get_property(flags GLOBAL PROPERTY Viskores_AVX_FLAGS)
elseif(Viskores_Vectorization STREQUAL "avx2")
  get_property(flags GLOBAL PROPERTY Viskores_AVX2_FLAGS)
elseif(Viskores_Vectorization STREQUAL "avx512-skx")
  get_property(flags GLOBAL PROPERTY Viskores_SKYLAKE_FLAGS)
elseif(Viskores_Vectorization STREQUAL "avx512-knl")
  get_property(flags GLOBAL PROPERTY Viskores_KNL_FLAGS)
endif()

target_compile_options(viskores_vectorization_flags
  INTERFACE $<$<COMPILE_LANGUAGE:CXX>:${flags}>
  )

if(TARGET viskores_cuda AND flags AND NOT CMAKE_CUDA_HOST_COMPILER)
  # Also propagate down these optimizations when building host side code
  # with cuda. To be safe we only do this when we know the C++ and CUDA
  # host compiler are from the same vendor
  string(REGEX REPLACE ";" "," cuda_flags "${flags}")
  target_compile_options(viskores_vectorization_flags
    INTERFACE $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=${cuda_flags}>
    )
endif()
