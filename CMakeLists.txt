# source files for png
set(sources
  png.c
  pngerror.c
  pngget.c
  pngmem.c
  pngpread.c
  pngread.c
  pngrio.c
  pngrtran.c
  pngrutil.c
  pngset.c
  pngtrans.c
  pngwio.c
  pngwrite.c
  pngwtran.c
  pngwutil.c)

set(headers
  png.h
  pngconf.h
  pnglibconf.h
  vtk_png_mangle.h
  "${CMAKE_CURRENT_BINARY_DIR}/vtkpngConfig.h")

if (WIN32)
  if (BUILD_SHARED_LIBS)
    set(PNG_NO_MODULEDEF 1)
  else ()
    set(PNG_STATIC 1)
  endif ()
endif ()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkpngConfig.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkpngConfig.h")

vtk_add_library(vtkpng ${sources} ${headers})
target_link_libraries(vtkpng PRIVATE ${vtkzlib_LIBRARIES})
if (NOT VTK_INSTALL_NO_DEVELOPMENT)
  install(FILES
    ${headers}
    DESTINATION "${VTK_INSTALL_INCLUDE_DIR}/vtkpng"
    COMPONENT Development)
endif()
# for vtkpngConfig.h
target_include_directories(vtkpng PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)

if (WIN32)
  set_target_properties(vtkpng
    PROPERTIES
      DEFINE_SYMBOL PNG_BUILD_DLL)
endif ()

# link against the math library
if (UNIX)
  target_link_libraries(vtkpng
    PRIVATE
      m)
endif ()
