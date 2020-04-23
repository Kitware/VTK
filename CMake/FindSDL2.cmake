#[==[
Provides the following variables:

  * `SDL2_FOUND`: Whether SDL2 was found or not.
  * `SDL2::SDL2`: A target to use with `target_link_libraries`.
#]==]

if (EMSCRIPTEN)
  # Create imported target SDL2::SDL2
  if (NOT TARGET SDL2::SDL2)
    add_library(SDL2::SDL2 INTERFACE IMPORTED)
    set_target_properties(SDL2::SDL2 PROPERTIES
      INTERFACE_COMPILE_OPTIONS "SHELL:-s USE_SDL=2"
      INTERFACE_LINK_OPTIONS "SHELL:-s USE_SDL=2"
      )
  endif()
  set(SDL2_FOUND TRUE)
  return()
endif()

set(_FindSDL2_args)
if (SDL2_FIND_PACKAGE_QUIETLY)
  list(APPEND _FindSDL2_args QUIET)
endif ()
if (SDL2_FIND_PACKAGE_REQUIRED)
  list(APPEND _FindSDL2_args REQUIRED)
endif ()

# More argument forwarding if `find_package(SDL2)` supports components (mixer, ttf, etc.).
find_package(SDL2 CONFIG ${_FindSDL2_args})
unset(_FindSDL2_args)
