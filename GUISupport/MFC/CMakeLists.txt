configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/VTKMFCSettings.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/VTKMFCSettings.cmake"
  COPYONLY
  )

include("${CMAKE_CURRENT_BINARY_DIR}/VTKMFCSettings.cmake")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkMFCConfigure.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkMFCConfigure.h"
  )

include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(MFC_SRCS
  vtkMFCWindow.cpp
  )

vtk_module_library(${vtk-module} ${VTK_MFC_LIB_TYPE} ${MFC_SRCS})

if(VTK_MFC_EXTRA_LIBS)
  target_link_libraries(${vtk-module} LINK_PRIVATE ${VTK_MFC_EXTRA_LIBS})
endif(VTK_MFC_EXTRA_LIBS)

if(WIN32)
  if(NOT VTK_INSTALL_NO_DEVELOPMENT)
    file(GLOB vtkMFCHeaderFiles "*.h")
    set(vtkMFCHeaderFiles ${vtkMFCHeaderFiles}
      "${CMAKE_CURRENT_BINARY_DIR}/vtkMFCConfigure.h")
    install(FILES ${vtkMFCHeaderFiles}
      DESTINATION ${VTK_INSTALL_INCLUDE_DIR}
      COMPONENT Development
      )
  endif()
endif()
