vtk_module_export_info()
set(Module_HDRS
  vtkVariantBoostSerialization.h
  )
if(NOT VTK_INSTALL_NO_DEVELOPMENT)
  install(FILES ${Module_HDRS}
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR}
    COMPONENT Development
    )
endif()
