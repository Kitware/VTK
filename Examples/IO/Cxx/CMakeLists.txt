cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)

PROJECT (IO)

if(NOT VTK_BINARY_DIR)
  find_package(VTK COMPONENTS
    vtkCommonCore
    vtkCommonDataModel
    vtkIOGeometry
    vtkIOXML
    vtkRenderingOpenGL
  )
  include(${VTK_USE_FILE})
endif()

add_executable(DumpXMLFile MACOSX_BUNDLE DumpXMLFile.cxx)
target_link_libraries(DumpXMLFile ${VTK_LIBRARIES})

add_executable(ParticleReader MACOSX_BUNDLE ParticleReader.cxx)
target_link_libraries(ParticleReader ${VTK_LIBRARIES})

if(BUILD_TESTING)
  if(vtkTestingRendering_LOADED)
    ######## Regression Testing ########
    SET(KIT IOExamples)
    SET(MyTests TestDumpXMLFile.cxx TestParticleReader.cxx)

    include(vtkTestingObjectFactory)

    add_executable(${KIT}CxxTests ${KIT}CxxTests.cxx ${MyTests})
    target_link_libraries(${KIT}CxxTests ${VTK_LIBRARIES})

    if (VTK_DATA_ROOT)
      add_test(NAME ${KIT}-ParticleReader COMMAND ${KIT}CxxTests
               TestParticleReader
                 ${VTK_DATA_ROOT}/Data/golf.csv
                 ${VTK_TEST_OUTPUT_DIR}/TestParticleReader.vtp)
      add_test(NAME ${KIT}-DumpXMLFile COMMAND ${KIT}CxxTests
               TestDumpXMLFile
                 ${VTK_DATA_ROOT}/Data/cow.vtp)
    endif()
  endif()
endif()
