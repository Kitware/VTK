# OpenSlide required
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
find_package(OpenSlide REQUIRED)

mark_as_advanced(OPENSLIDE_INCLUDE_DIR OPENSLIDE_LIBRARY)

set(OPENSLIDE_SRCS vtkOpenSlideReader.cxx)

include_directories(${OPENSLIDE_INCLUDE_DIRS})

set(${vtk-module}_SYSTEM_INCLUDE_DIRS
    ${OPENSLIDE_INCLUDE_DIRS})

vtk_module_library(vtkDomainsMicroscopy ${OPENSLIDE_SRCS})

vtk_module_link_libraries(vtkDomainsMicroscopy LINK_PRIVATE ${OPENSLIDE_LIBRARY})
