vtk_module_third_party(LZMA)

include(CMakeDependentOption)
cmake_dependent_option(VTK_MODULE_vtklzma_IS_SHARED "Whether the LZMA in use is shared or not" ON
  VTK_USE_SYSTEM_LZMA "${BUILD_SHARED_LIBS}")
mark_as_advanced(VTK_MODULE_vtklzma_IS_SHARED)
if (VTK_MODULE_vtklzma_IS_SHARED)
  set(LZMA_BUILT_AS_DYNAMIC_LIB 1)
endif ()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_lzma.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_lzma.h")

if (NOT VTK_INSTALL_NO_DEVELOPMENT)
  install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/vtk_lzma.h"
    DESTINATION "${VTK_INSTALL_INCLUDE_DIR}"
    COMPONENT Development)
endif()
