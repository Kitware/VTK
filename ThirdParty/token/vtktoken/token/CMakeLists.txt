set(classes
  Compiler
  Hash
  Singletons
  Token
  Type
  TypeContainer
  Manager

  json/jsonToken
  json/jsonManager

  # Generated hedaers
  "${CMAKE_CURRENT_BINARY_DIR}/Options"
  "${CMAKE_CURRENT_BINARY_DIR}/CxxABIConfigure"
  # XXX(kitware): Get this generated header installed:
  "${CMAKE_CURRENT_BINARY_DIR}/Exports"
)

# Determine whether the cxxabi header exists and if it contains a demangle function.
# XXX(c++17): prefer `__has_include`
check_include_file_cxx("cxxabi.h" HAVE_CXXABI_H)
if (HAVE_CXXABI_H)
  check_cxx_symbol_exists(abi::__cxa_demangle "cxxabi.h" token_HAS_CXXABI_DEMANGLE)
endif()
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/CxxABIConfigure.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/CxxABIConfigure.h")

foreach (class ${classes})
  list(APPEND headers "${class}.h")
  # XXX(kitware): vtk_module_add_module below also installs
  if (FALSE)
  install(
    FILES "${class}.h"
    DESTINATION "include/token/${token_VERSION}/token/${headerDir}"
  )
  endif()
  if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${class}.cxx")
    list(APPEND sources "${class}.cxx")
  endif()
endforeach()

if(FALSE) # XXX(kitware): Use VTK module system
# Note that we REQUIRE the token library to be a shared library.
# This is because on some platforms (macos), if multiple executable
# units (i.e., shared libraries) link to the same static token library,
# then token::Token::getManager() will return pointers to different
# instances, causing odd behavior and crashes when strings interned
# by one library are not available to others.
add_library(token SHARED ${sources} ${headers})
target_link_libraries(token
  PUBLIC
    nlohmann_json
)
else()

  # When building inside VTK and wrapping python, we need to force the token lib to be shared
  # to ensure there is a single instance of the global symbols.
  set(token_module_args)
  if (VTK_WRAP_PYTHON)
    list(APPEND token_module_args FORCE_SHARED)
  endif ()

  vtk_module_add_module(VTK::token
    HEADER_DIRECTORIES
    ${token_module_args}
    SOURCES ${sources}
    HEADERS ${headers}
    HEADERS_SUBDIR "token"
  )
vtk_module_link(VTK::token PUBLIC VTK::nlohmannjson)
endif()

target_include_directories(token
  PUBLIC
    $<BUILD_INTERFACE:${token_SOURCE_DIR}>
    $<BUILD_INTERFACE:${token_BINARY_DIR}>
    # XXX(kitware): Use path that works with VTK module
    # $<INSTALL_INTERFACE:include/token/${token_VERSION}>
)
if (FALSE) # XXX(kitware): VTK handles installation
token_install_library(token)
endif ()
generate_export_header(token EXPORT_FILE_NAME Exports.h)

add_executable(tokenize
  tokenize.cxx
)
if (FALSE) # XXX(kitware): use module target name.
target_link_libraries(tokenize
  PUBLIC
    token
)
token_install_target(tokenize)
else()
target_link_libraries(tokenize
  PUBLIC
    VTK::token
)
endif()

add_subdirectory(testing)
