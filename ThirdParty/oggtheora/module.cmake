if("${CMAKE_C_COMPILER_ID}" MATCHES "^XL$")
  # This module does not compile with XL.
  return()
endif()
vtk_module(vtkoggtheora EXCLUDE_FROM_WRAPPING)
