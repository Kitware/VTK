cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)

PROJECT (Medical)

if(NOT VTK_BINARY_DIR)
  find_package(VTK COMPONENTS
    vtkCommonCore
    vtkCommonDataModel
    vtkCommonTransforms
    vtkFiltersCore
    vtkFiltersGeneral
    vtkFiltersGeometry
    vtkFiltersModeling
    vtkIOImage
    vtkIOXML
    vtkImagingCore
    vtkImagingStatistics
    vtkRenderingCore
    vtkRenderingOpenGL
    vtkRenderingVolume
    vtkRenderingVolumeOpenGL
    vtkInteractionStyle
  )
  include(${VTK_USE_FILE})
endif()

add_executable(Medical1 MACOSX_BUNDLE Medical1.cxx)
add_executable(Medical2 MACOSX_BUNDLE Medical2.cxx)
add_executable(Medical3 MACOSX_BUNDLE Medical3.cxx)
add_executable(Medical4 MACOSX_BUNDLE Medical4.cxx)

target_link_libraries(Medical1 ${VTK_LIBRARIES})
target_link_libraries(Medical2 ${VTK_LIBRARIES})
target_link_libraries(Medical3 ${VTK_LIBRARIES})
target_link_libraries(Medical4 ${VTK_LIBRARIES})

add_executable(GenerateModelsFromLabels MACOSX_BUNDLE GenerateModelsFromLabels.cxx)
add_executable(GenerateCubesFromLabels MACOSX_BUNDLE  GenerateCubesFromLabels.cxx)
target_link_libraries(GenerateModelsFromLabels ${VTK_LIBRARIES})
target_link_libraries(GenerateCubesFromLabels ${VTK_LIBRARIES})

if(BUILD_TESTING)
  if(vtkTestingRendering_LOADED)
    ######## Regression Testing ########
    set(KIT MedicalExamples)
    set(MyTests TestMedical1.cxx TestMedical2.cxx TestMedical3.cxx TestMedical4.cxx)

    include(vtkTestingObjectFactory)

    add_executable(${KIT}CxxTests ${KIT}CxxTests.cxx ${MyTests})
    target_link_libraries(${KIT}CxxTests ${VTK_LIBRARIES})

      if (VTK_DATA_ROOT)
        add_test(NAME ${KIT}-Medical1 COMMAND ${KIT}CxxTests
          TestMedical1 ${VTK_DATA_ROOT}/Data/headsq/quarter)
        add_test(NAME ${KIT}-Medical2 COMMAND ${KIT}CxxTests
          TestMedical2 ${VTK_DATA_ROOT}/Data/headsq/quarter)
        add_test(NAME ${KIT}-Medical3 COMMAND ${KIT}CxxTests
          TestMedical3 ${VTK_DATA_ROOT}/Data/headsq/quarter)
        add_test(NAME ${KIT}-Medical4 COMMAND ${KIT}CxxTests
          TestMedical4 ${VTK_DATA_ROOT}/Data/headsq/quarter)
      endif()
  endif()
endif()
