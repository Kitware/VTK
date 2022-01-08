include("${CMAKE_CURRENT_SOURCE_DIR}/VTKMFCSettings.cmake")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkMFCConfigure.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkMFCConfigure.h"
  )

set(classes
  vtkMFCWindow
  )
set(headers
  "${CMAKE_CURRENT_BINARY_DIR}/vtkMFCConfigure.h"
  )

vtk_module_add_module(VTK::GUISupportMFC
  CLASSES ${classes}
  HEADERS ${headers})

if(VTK_MFC_EXTRA_LIBS)
  vtk_module_link(VTK::GUISupportMFC PRIVATE ${VTK_MFC_EXTRA_LIBS})
endif()
