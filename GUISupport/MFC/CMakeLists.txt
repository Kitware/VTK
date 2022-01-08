include("${CMAKE_CURRENT_SOURCE_DIR}/VTKMFCSettings.cmake")

set(classes
  vtkMFCWindow
  )
set(headers
  vtkMFCConfigure.h
  )

vtk_module_add_module(VTK::GUISupportMFC
  CLASSES ${classes}
  HEADERS ${headers})

if(VTK_MFC_EXTRA_LIBS)
  vtk_module_link(VTK::GUISupportMFC PRIVATE ${VTK_MFC_EXTRA_LIBS})
endif()
