# Support XML files >2GB.
add_definitions(-DXML_LARGE_SIZE)

if (NOT BUILD_SHARED_LIBS)
  set(XML_STATIC 1)
endif ()

vtk_module_third_party(EXPAT)
