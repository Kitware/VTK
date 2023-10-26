# setup toolchain
set(CMAKE_TOOLCHAIN_FILE "$ENV{EMSDK}/cmake/Modules/Platform/emscripten.cmake")
set(CMAKE_CROSSCOMPILING_EMULATOR "$ENV{EMSDK_NODE}")

# Disable unsupported features
set(BUILD_SHARED_LIBS OFF CACHE BOOL "")
set(VTK_ENABLE_LOGGING ON CACHE BOOL "")
set(VTK_ENABLE_WRAPPING OFF CACHE BOOL "")
set(VTK_BUILD_TESTING OFF CACHE BOOL "")
set(VTK_BUILD_EXAMPLES OFF CACHE BOOL "")
set(VTK_ENABLE_CATALYST OFF CACHE BOOL "")
set(VTK_LEGACY_REMOVE ON CACHE BOOL "") # Hides many `-Wreserved-identifier` from legacy code.
# Modules which do not build successfully or do not have required software in the docker image.
set(VTK_GROUP_ENABLE_Qt NO CACHE STRING "") # no qt
# ├── Common
set(VTK_MODULE_ENABLE_VTK_CommonArchive NO CACHE STRING "") # libarchive
# ├── Domains
set(VTK_MODULE_ENABLE_VTK_DomainsMicroscopy NO CACHE STRING "") # no OpenSlide
# ├── Filters
set(VTK_MODULE_ENABLE_VTK_FiltersOpenTURNS NO CACHE STRING "") # no openturns
set(VTK_MODULE_ENABLE_VTK_FiltersReebGraph NO CACHE STRING "") # no boost
# ├── Infovis
set(VTK_MODULE_ENABLE_VTK_InfovisBoost NO CACHE STRING "" ) # no boost
set(VTK_MODULE_ENABLE_VTK_InfovisBoostGraphAlgorithms NO CACHE STRING "" ) # no boost
set(VTK_MODULE_ENABLE_VTK_InfovisCore NO CACHE STRING "" ) # no boost
set(VTK_MODULE_ENABLE_VTK_InfovisLayout NO CACHE STRING "" ) # no boost
# ├── IO
set(VTK_MODULE_ENABLE_VTK_IOADIOS2 NO CACHE STRING "") # no adios
set(VTK_MODULE_ENABLE_VTK_IOAlembic NO CACHE STRING "") # alembic
set(VTK_MODULE_ENABLE_VTK_IOAMR NO CACHE STRING "") # no hdf5
set(VTK_MODULE_ENABLE_VTK_IOCatalystConduit NO CACHE STRING "") # no catalyst
set(VTK_MODULE_ENABLE_VTK_IOFFMPEG NO CACHE STRING "") # no ffmpeg
set(VTK_MODULE_ENABLE_VTK_IOGDAL NO CACHE STRING "") # no gdal
set(VTK_MODULE_ENABLE_VTK_IOLAS NO CACHE STRING "") # no liblas
set(VTK_MODULE_ENABLE_VTK_IOMySQL NO CACHE STRING "") # no mysql
set(VTK_MODULE_ENABLE_VTK_IOOCCT NO CACHE STRING "") # no open cascade
set(VTK_MODULE_ENABLE_VTK_IOODBC NO CACHE STRING "") # no iodbc
set(VTK_MODULE_ENABLE_VTK_IOOpenVDB NO CACHE STRING "") # no openvdb
set(VTK_MODULE_ENABLE_VTK_IOPDAL NO CACHE STRING "") # no pdal
set(VTK_MODULE_ENABLE_VTK_IOPostgreSQL NO CACHE STRING "") # no postgresql
# ├── Rendering
set(VTK_MODULE_ENABLE_VTK_RenderingExternal NO CACHE STRING "") # gl code incompatible with gles 3.0
set(VTK_MODULE_ENABLE_VTK_RenderingFFMPEGOpenGL2 NO CACHE STRING "") # no ffmpeg
set(VTK_MODULE_ENABLE_VTK_RenderingFreeTypeFontConfig NO CACHE STRING "") # no fontconfig
set(VTK_MODULE_ENABLE_VTK_RenderingLICOpenGL2 NO CACHE STRING "") # gl code incompatible with gles 3.0
set(VTK_MODULE_ENABLE_VTK_RenderingMatplotlib NO CACHE STRING "") # no matplotlib
set(VTK_MODULE_ENABLE_VTK_RenderingOpenVR NO CACHE STRING "") # no openvr
set(VTK_MODULE_ENABLE_VTK_RenderingOpenXR NO CACHE STRING "") # no openxr
set(VTK_MODULE_ENABLE_VTK_RenderingQt NO CACHE STRING "") # no qt
set(VTK_MODULE_ENABLE_VTK_RenderingRayTracing NO CACHE STRING "") # no ospray or visrtx
set(VTK_MODULE_ENABLE_VTK_RenderingTk NO CACHE STRING "") # no tk
set(VTK_MODULE_ENABLE_VTK_RenderingVR NO CACHE STRING "") # gl code incompatible with gles 3.0
set(VTK_MODULE_ENABLE_VTK_RenderingZSpace NO CACHE STRING "") # no zspace
# ├── ThirdParty
set(VTK_MODULE_ENABLE_VTK_fides NO CACHE STRING "") # no adios2
set(VTK_MODULE_ENABLE_VTK_xdmf3 NO CACHE STRING "") # no boost
set(VTK_MODULE_ENABLE_VTK_libproj NO CACHE STRING "") # fails to generate proj.db
set(VTK_MODULE_ENABLE_VTK_vtkvtkm NO CACHE STRING "") # no execinfo.h in vtkm's loguru

include("${CMAKE_CURRENT_LIST_DIR}/configure_common.cmake")
