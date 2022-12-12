# Disable modules for which there are missing dependencies.
set(VTK_MODULE_ENABLE_VTK_CommonArchive NO CACHE STRING "") # libarchive
set(VTK_MODULE_ENABLE_VTK_DomainsMicroscopy NO CACHE STRING "") # openslide
set(VTK_MODULE_ENABLE_VTK_FiltersOpenTURNS NO CACHE STRING "") # openturns
set(VTK_MODULE_ENABLE_VTK_IOADIOS2 NO CACHE STRING "") # adios2
set(VTK_MODULE_ENABLE_VTK_IOFFMPEG NO CACHE STRING "") # ffmpeg
set(VTK_MODULE_ENABLE_VTK_IOLAS NO CACHE STRING "") # liblas
set(VTK_MODULE_ENABLE_VTK_IOMySQL NO CACHE STRING "") # mysql
set(VTK_MODULE_ENABLE_VTK_IOODBC NO CACHE STRING "") # odbc
set(VTK_MODULE_ENABLE_VTK_IOPDAL NO CACHE STRING "") # pdal
set(VTK_MODULE_ENABLE_VTK_IOPostgreSQL NO CACHE STRING "") # postgresql
set(VTK_MODULE_ENABLE_VTK_RenderingExternal NO CACHE STRING "") # glew
set(VTK_MODULE_ENABLE_VTK_RenderingFreeTypeFontConfig NO CACHE STRING "") # fontconfig
set(VTK_MODULE_ENABLE_VTK_RenderingOpenVR NO CACHE STRING "") # openvr
set(VTK_MODULE_ENABLE_VTK_fides NO CACHE STRING "") # adios2

include("${CMAKE_CURRENT_LIST_DIR}/configure_el8.cmake")
