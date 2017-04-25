vtk_module_third_party(LibHaru
  INCLUDE_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}/vtklibharu/include"
    "${CMAKE_CURRENT_BINARY_DIR}/vtklibharu/include"
)

option(VTK_EXTERNAL_LIBHARU_IS_SHARED
  "Whether the system libharu in use is a shared library or not"
  ON
)
mark_as_advanced(VTK_EXTERNAL_LIBHARU_IS_SHARED)
