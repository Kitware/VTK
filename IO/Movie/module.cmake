if(NOT CMAKE_C_COMPILER_ID STREQUAL "XL")
  if(NOT ${CMAKE_CROSS_COMPILING})
    set(vtkIOMovie_vtkoggtheora vtkoggtheora)
  endif()
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
  KIT
    vtkIO
  )
