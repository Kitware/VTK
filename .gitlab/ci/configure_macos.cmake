# Modules which require software not on CI machines.
set(VTK_MODULE_ENABLE_VTK_CommonArchive NO CACHE STRING "") # libarchive
set(VTK_MODULE_ENABLE_VTK_DomainsMicroscopy NO CACHE STRING "") # openslide
set(VTK_MODULE_ENABLE_VTK_FiltersOpenTURNS NO CACHE STRING "") # openturns
set(VTK_MODULE_ENABLE_VTK_FiltersReebGraph NO CACHE STRING "") # boost
set(VTK_MODULE_ENABLE_VTK_GeovisGDAL NO CACHE STRING "") # gdal
set(VTK_MODULE_ENABLE_VTK_IOADIOS2 NO CACHE STRING "") # adios
set(VTK_MODULE_ENABLE_VTK_IOFFMPEG NO CACHE STRING "") # ffmpeg
set(VTK_MODULE_ENABLE_VTK_IOGDAL NO CACHE STRING "") # ffmpeg
set(VTK_MODULE_ENABLE_VTK_IOLAS NO CACHE STRING "") # liblas, boost
set(VTK_MODULE_ENABLE_VTK_IOMySQL NO CACHE STRING "") # mysql
set(VTK_MODULE_ENABLE_VTK_IOODBC NO CACHE STRING "") # odbc
set(VTK_MODULE_ENABLE_VTK_IOOpenVDB NO CACHE STRING "") # OpenVDB
set(VTK_MODULE_ENABLE_VTK_IOPDAL NO CACHE STRING "") # pdal
set(VTK_MODULE_ENABLE_VTK_IOPostgreSQL NO CACHE STRING "") # postgresql
set(VTK_MODULE_ENABLE_VTK_InfovisBoost NO CACHE STRING "") # boost
set(VTK_MODULE_ENABLE_VTK_InfovisBoostGraphAlgorithms NO CACHE STRING "") # boost
set(VTK_MODULE_ENABLE_VTK_RenderingFreeTypeFontConfig NO CACHE STRING "") # fontconfig
set(VTK_MODULE_ENABLE_VTK_RenderingMatplotlib NO CACHE STRING "") # matplotlib
set(VTK_MODULE_ENABLE_VTK_RenderingOpenVR NO CACHE STRING "") # openvr
set(VTK_MODULE_ENABLE_VTK_RenderingRayTracing NO CACHE STRING "") # ospray
set(VTK_MODULE_ENABLE_VTK_fides NO CACHE STRING "") # adios
set(VTK_MODULE_ENABLE_VTK_xdmf3 NO CACHE STRING "") # boost

# Add rpath entries for Xcode frameworks.
set(CMAKE_BUILD_RPATH "$ENV{DEVELOPER_DIR}/Library/Frameworks" CACHE STRING "")
set(CMAKE_INSTALL_RPATH "$ENV{DEVELOPER_DIR}/Library/Frameworks" CACHE STRING "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_common.cmake")
