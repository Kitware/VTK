# first include proj library
# always need
include(lib_proj.cmake)

if(NOT MSVC)
  if (NOT APPLE)
    # Use relative path so that package is relocatable
    set(CMAKE_INSTALL_RPATH "\$ORIGIN/../${LIBPROJ_LIBDIR}")
  else ()
    set (CMAKE_INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/${LIBPROJ_LIBDIR}")
    # TO DO: cmake 2.8.12 introduces a way to make the install tree
    # relocatable with OSX via
    # (1) set(CMAKE_MACOSX_RPATH ON) and
    # (2) setting the INSTALL_RPATH property on the executables to
    # "@loader_path/../${LIBPROJ_LIBDIR}"
  endif ()
endif ()

