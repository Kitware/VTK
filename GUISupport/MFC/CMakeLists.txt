include("${CMAKE_CURRENT_SOURCE_DIR}/VTKMFCSettings.cmake")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkMFCConfigure.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkMFCConfigure.h"
  )

set(MFC_SRCS
  vtkMFCWindow.cpp
  )
set(MFC_HDRS
  vtkMFCWindow.h
  )

vtk_module_add_module(VTK::GUISupportMFC
  SOURCES ${MFC_SRCS}
  HEADERS ${MFC_HDRS}
          "${CMAKE_CURRENT_BINARY_DIR}/vtkMFCConfigure.h")

if(VTK_MFC_EXTRA_LIBS)
  vtk_module_link(VTK::GUISupportMFC PRIVATE ${VTK_MFC_EXTRA_LIBS})
endif()
