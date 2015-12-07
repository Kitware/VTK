project(VTKPNG)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# source files for png
set(PNG_SRCS
pngget.c    pngrio.c    pngwrite.c
png.c       pngmem.c    pngrtran.c  pngtrans.c  pngwtran.c
pngerror.c  pngpread.c  pngrutil.c  pngvcrd.c   pngwutil.c
pnggccrd.c  pngread.c   pngset.c   pngwio.c
)

if(WIN32)
  if(BUILD_SHARED_LIBS)
    add_definitions(-DPNG_BUILD_DLL)
    set(PNG_NO_MODULEDEF 1)
  else()
    set(PNG_STATIC 1)
  endif()
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/vtkpngConfig.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/vtkpngConfig.h)

vtk_add_library(vtkpng ${PNG_SRCS})

target_link_libraries(vtkpng ${vtkzlib_LIBRARIES})

# link against the math library
if(UNIX)
  target_link_libraries(vtkpng -lm)
endif()

if(NOT VTK_INSTALL_NO_DEVELOPMENT)
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/vtkpngConfig.h
    ${CMAKE_CURRENT_SOURCE_DIR}/png.h
    ${CMAKE_CURRENT_SOURCE_DIR}/pngconf.h
    ${CMAKE_CURRENT_SOURCE_DIR}/vtk_png_mangle.h
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR}/vtkpng
    COMPONENT Development)
endif()
