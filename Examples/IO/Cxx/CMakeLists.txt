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

if(vtkTestingRendering_SOURCE_DIR)
######## Regression Testing ########
  set(vtk-example IOExamples)
  if (VTK_DATA_ROOT)
    set(TestDumpXMLFile_ARGS ${VTK_DATA_ROOT}/Data/cow.vtp)
    set(TestParticleReader_ARGS
      ${VTK_DATA_ROOT}/Data/golf.csv
      ${VTK_TEST_OUTPUT_DIR}/TestParticleReader.vtp)
    vtk_add_test_cxx(NO_DATA NO_VALID NO_OUTPUT
      TestParticleReader.cxx
      TestDumpXMLFile.cxx
      )
  endif()
  vtk_test_cxx_executable(${vtk-example}CxxTests RENDERING_FACTORY)
endif()
