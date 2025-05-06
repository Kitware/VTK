if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR
   CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
  set(DIY_COMPILER_IS_MSVC 1)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "PGI")
  set(DIY_COMPILER_IS_PGI 1)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
  set(DIY_COMPILER_IS_ICC 1)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  set(DIY_COMPILER_IS_CLANG 1)
  set(DIY_COMPILER_IS_APPLECLANG 1)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(DIY_COMPILER_IS_CLANG 1)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(DIY_COMPILER_IS_GNU 1)
endif()

#-----------------------------------------------------------------------------
add_library(diy_developer_flags INTERFACE)

if(DIY_COMPILER_IS_MSVC)
  target_compile_definitions(diy_developer_flags INTERFACE
    "_SCL_SECURE_NO_WARNINGS" "_CRT_SECURE_NO_WARNINGS")

  if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.15)
    set(cxx_flags "-W3")
  endif()
  #list(APPEND cxx_flags -wd4702 -wd4505)

  if(MSVC_VERSION LESS 1900)
    # In VS2013 the C4127 warning has a bug in the implementation and
    # generates false positive warnings for lots of template code
    #list(APPEND cxx_flags -wd4127)
  endif()

  target_compile_options(diy_developer_flags INTERFACE $<$<COMPILE_LANGUAGE:CXX>:${cxx_flags}>)

elseif(DIY_COMPILER_IS_ICC)
  # dissable some false positive warnings
  set(cxx_flags -wd186 -wd3280)
  list(APPEND cxx_flags -diag-disable=11074 -diag-disable=11076)
  #list(APPEND cxx_flags -wd1478 -wd 13379)
  target_compile_options(diy_developer_flags INTERFACE $<$<COMPILE_LANGUAGE:CXX>:${cxx_flags}>)

elseif(DIY_COMPILER_IS_GNU OR DIY_COMPILER_IS_CLANG)
  set(cxx_flags -Wall -Wcast-align -Wchar-subscripts -Wextra -Wpointer-arith -Wformat -Wformat-security -Wshadow -Wunused -fno-common)

  #Only add float-conversion warnings for gcc as the integer warnigns in GCC
  #include the implicit casting of all types smaller than int to ints.
  if (DIY_COMPILER_IS_GNU AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.99)
    list(APPEND cxx_flags -Wfloat-conversion)
  elseif (DIY_COMPILER_IS_CLANG)
    list(APPEND cxx_flags -Wconversion)
  endif()

  # TODO: remove after resolving these warnings
  # temporarily disable the following warnings as we will need a well thought out plan for fixing these
  list(APPEND cxx_flags -Wno-sign-conversion -Wno-sign-compare -Wno-cast-align)

  #Add in the -Wodr warning for GCC versions 5.2+
  if (DIY_COMPILER_IS_CLANG OR (DIY_COMPILER_IS_GNU AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 5.1))
    list(APPEND cxx_flags -Wodr)
  endif()

  #GCC 5, 6 don't properly handle strict-overflow suppression through pragma's.
  #Instead of suppressing around the location of the strict-overflow you
  #have to suppress around the entry point, or in viskores case the worklet
  #invocation site. This is incredibly tedious and has been fixed in gcc 7
  #
  if(DIY_COMPILER_IS_GNU AND
    (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.99) AND
    (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6.99) )
    list(APPEND cxx_flags -Wno-strict-overflow)
  endif()

  target_compile_options(diy_developer_flags INTERFACE $<$<COMPILE_LANGUAGE:CXX>:${cxx_flags}>)
endif()
