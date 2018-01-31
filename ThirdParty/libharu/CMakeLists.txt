include(CMakeDependentOption)

cmake_dependent_option(VTK_MODULE_vtklibharu_IS_SHARED "Whether the system libharu in use is a shared library or not" ON
  VTK_USE_SYSTEM_LIBHARU "${BUILD_SHARED_LIBS}")

if (WIN32 AND VTK_MODULE_vtklibharu_IS_SHARED)
  set(HPDF_DLL 1)
endif()

vtk_module_third_party(LibHaru
  INCLUDE_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}/vtklibharu/include"
    "${CMAKE_CURRENT_BINARY_DIR}/vtklibharu/include"
)
