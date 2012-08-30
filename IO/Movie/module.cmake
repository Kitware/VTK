if(NOT "${CMAKE_C_COMPILER_ID}" MATCHES "^XL$")
  set(vtkIOMovie_vtkoggtheora vtkoggtheora)
endif()
vtk_module(vtkIOMovie
  GROUPS
    StandAlone
  DEPENDS
    vtkCommonDataModel
    vtkCommonExecutionModel
    vtkCommonSystem
    vtkIOCore
    ${vtkIOMovie_vtkoggtheora}
  TEST_DEPENDS
    vtkTestingCore
    vtkImagingCore
    vtkImagingSources
  )
