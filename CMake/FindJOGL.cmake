if (COMMAND "find_jar")
  set(_jogl_versions
    ${JOGL_EXTRA_VERSIONS} 2.3.2)

  set(_JOGL_Java_JAR_PATHS)
  if (DEFINED Java_JAR_PATHS)
    set(_JOGL_Java_JAR_PATHS "${Java_JAR_PATHS}")
  endif ()

  foreach (_JOGL_prefix IN ITEMS "${CMAKE_INSTALL_PREFIX}" /usr/local /usr)
    foreach (_JOGL_libdir IN ITEMS "${CMAKE_INSTALL_LIBDIR}" lib64 lib)
      list(APPEND Java_JAR_PATHS
        "${_JOGL_prefix}/${_JOGL_libdir}/java"
        "${_JOGL_prefix}/${_JOGL_libdir}")
    endforeach ()
  endforeach ()
  list(REMOVE_DUPLICATES Java_JAR_PATHS)
  unset(_JOGL_libdir)
  unset(_JOGL_prefix)

  find_jar(JOGL_LIB
    NAMES jogl-all jogl2
    PATHS # Abuse the argument parser in `find_jar`
    PATH_SUFFIXES jogl2
    VERSIONS ${_jogl_versions}
    DOC "Path to the JOGL jar")
  mark_as_advanced(JOGL_LIB)

  find_jar(JOGL_GLUE
    NAMES gluegen-rt gluegen2-rt
    VERSIONS ${_jogl_versions}
    PATHS # Abuse the argument parser in `find_jar`
    PATH_SUFFIXES gluegen2
    DOC "Path to the JOGL gluegen jar")
  mark_as_advanced(JOGL_GLUE)

  set(Java_JAR_PATHS)
  if (DEFINED _JOGL_Java_JAR_PATHS)
    set(Java_JAR_PATHS "${_JOGL_Java_JAR_PATHS}")
  endif ()
  unset(_JOGL_Java_JAR_PATHS)

  unset(_jogl_versions)
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(JOGL
  REQUIRED_VARS JOGL_LIB JOGL_GLUE)

if (JOGL_FOUND)
  if (NOT TARGET JOGL::glue)
    add_library(JOGL::glue STATIC IMPORTED)
    set_target_properties(JOGL::glue PROPERTIES
      IMPORTED_LINK_INTERFACE_LANGUAGES Java
      IMPORTED_LOCATION "${JOGL_GLUE}")
  endif ()
  if (NOT TARGET JOGL::JOGL)
    add_library(JOGL::JOGL STATIC IMPORTED)
    set_target_properties(JOGL::JOGL PROPERTIES
      IMPORTED_LINK_INTERFACE_LANGUAGES Java
      IMPORTED_LOCATION "${JOGL_LIB}"
      INTERFACE_LINK_LIBRARIES "JOGL::glue")
  endif ()
endif ()
