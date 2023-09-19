# Disable modules for which there are missing dependencies.
set(VTK_MODULE_ENABLE_VTK_FiltersOpenTURNS NO CACHE STRING "") # openturns
set(VTK_MODULE_ENABLE_VTK_IOADIOS2 NO CACHE STRING "") # adios2
set(VTK_MODULE_ENABLE_VTK_IOAlembic NO CACHE STRING "") # alembic
set(VTK_MODULE_ENABLE_VTK_IOFFMPEG NO CACHE STRING "") # ffmpeg
set(VTK_MODULE_ENABLE_VTK_IOMySQL NO CACHE STRING "") # mysql
set(VTK_MODULE_ENABLE_VTK_IOPDAL NO CACHE STRING "") # pdal
set(VTK_MODULE_ENABLE_VTK_IOPostgreSQL NO CACHE STRING "") # postgresql
set(VTK_MODULE_ENABLE_VTK_RenderingOpenVR NO CACHE STRING "") # openvr
set(VTK_MODULE_ENABLE_VTK_RenderingOpenXR NO CACHE STRING "") # openxr
set(VTK_MODULE_ENABLE_VTK_fides NO CACHE STRING "") # adios2
set(VTK_MODULE_ENABLE_VTK_vtkvtkm NO CACHE STRING "") # requires C++14

include("${CMAKE_CURRENT_LIST_DIR}/configure_el7.cmake")
