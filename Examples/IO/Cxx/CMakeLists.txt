cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)
if(POLICY CMP0025)
  cmake_policy(SET CMP0025 NEW) # CMake 3.0
endif()
if(POLICY CMP0053)
  cmake_policy(SET CMP0053 NEW) # CMake 3.1
endif()

PROJECT (IO)

find_package(VTK COMPONENTS
  vtkCommonCore
  vtkIOLegacy
  vtkIOXML
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
