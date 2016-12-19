cmake_minimum_required(VERSION 3.3 FATAL_ERROR)


PROJECT (IO)

find_package(VTK COMPONENTS
  vtkCommonCore
  vtkIOLegacy
  vtkIOXML
  vtkIOGeometry
  vtkTestingRendering
  vtksys
)
include(${VTK_USE_FILE})

add_executable(DumpXMLFile MACOSX_BUNDLE DumpXMLFile.cxx)
target_link_libraries(DumpXMLFile ${VTK_LIBRARIES})

add_executable(ParticleReader MACOSX_BUNDLE ParticleReader.cxx)
target_link_libraries(ParticleReader ${VTK_LIBRARIES})

if(BUILD_TESTING)
  if(vtkTestingRendering_LOADED)
    ######## Regression Testing ########
    set(vtk-example IOExamples)
    set(TestDumpXMLFile_ARGS "DATA{${VTK_TEST_INPUT_DIR}/cow.vtp}")
    set(TestParticleReader_ARGS
      "${VTK_TEST_DATA_DIR}/Data/golf.csv"
      ${VTK_TEST_OUTPUT_DIR}/TestParticleReader.vtp)
    vtk_add_test_cxx(${vtk-example}CxxTests tests
      NO_DATA NO_VALID NO_OUTPUT
      TestParticleReader.cxx
      TestDumpXMLFile.cxx
      )
    vtk_test_cxx_executable(${vtk-example}CxxTests tests
      RENDERING_FACTORY
      )
  endif()
endif()
