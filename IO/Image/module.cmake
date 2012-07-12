if(NOT "${CMAKE_C_COMPILER_ID}" MATCHES "^XL$")
  set(vtkIOImage_vtkoggtheora vtkoggtheora)
endif()
vtk_module(vtkIOImage
  GROUPS
    StandAlone
  DEPENDS
    vtkCommonDataModel
    vtkCommonSystem
    vtkCommonMath
    vtkCommonMisc
    vtkCommonTransforms
    vtkIOGeometry
    vtkjpeg
    vtkpng
    vtktiff
    vtkMetaIO
    ${vtkIOImage_vtkoggtheora}
    vtkDICOMParser
  TEST_DEPENDS
    vtkTestingCore
    vtkImagingSources
    vtkImagingMath
  )
