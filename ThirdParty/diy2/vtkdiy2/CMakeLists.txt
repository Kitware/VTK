if (NOT VTK_INSTALL_NO_DEVELOPMENT)
  install(DIRECTORY include/vtkdiy2/
    DESTINATION "${VTK_INSTALL_INCLUDE_DIR}/vtkdiy2"
    COMPONENT Development)
endif()
