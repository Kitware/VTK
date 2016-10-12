if(CMAKE_C_COMPILER_ID STREQUAL "XL")
  # This module does not compile with XL.
  return()
endif()
if(${CMAKE_CROSS_COMPILING})
  return()
endif()
vtk_module(vtkoggtheora EXCLUDE_FROM_WRAPPING)