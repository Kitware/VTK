cmake_minimum_required(VERSION 3.3...3.12 FATAL_ERROR)
project(IO)

find_package(VTK
  OPTIONAL_COMPONENTS
    IOExport
    IOImport
    TestingRendering)
find_package(VTK
  COMPONENTS
    CommonCore
    IOGeometry
    IOLegacy
    IOXML
    InteractionStyle)
if (NOT VTK_FOUND)
  message("Skipping example: ${VTK_NOT_FOUND_MESSAGE}")
  return ()
endif ()

add_executable(DumpXMLFile MACOSX_BUNDLE
  DumpXMLFile.cxx)
target_link_libraries(DumpXMLFile
  PRIVATE
    ${VTK_LIBRARIES})

add_executable(ParticleReader MACOSX_BUNDLE
  ParticleReader.cxx)
target_link_libraries(ParticleReader
  PRIVATE
    ${VTK_LIBRARIES})

vtk_module_autoinit(
  TARGETS DumpXMLFile
          ParticleReader
  MODULES ${VTK_LIBRARIES})

if (TARGET VTK::IOExport AND TARGET VTK::IOImport)
  add_executable(objtovtp
    objtovtp.cxx)
  target_link_libraries(objtovtp
    PRIVATE
      ${VTK_LIBRARIES}
      VTK::IOExport
      VTK::IOImport)

  vtk_module_autoinit(
    TARGETS objtovtp
    MODULES ${VTK_LIBRARIES}
            VTK::IOExport
            VTK::IOImport)
endif ()

if (BUILD_TESTING)
  if (TARGET VTK::TestingRendering)
    ######## Regression Testing ########
    set(vtk-example IOExamples)
    set(TestDumpXMLFile_ARGS "DATA{${VTK_TEST_INPUT_DIR}/cow.vtp}")
    set(TestParticleReader_ARGS
      "${_vtk_build_TEST_DATA_DIRECTORY}/Data/golf.csv"
      ${_vtk_build_TEST_OUTPUT_DIRECTORY}/TestParticleReader.vtp)
    vtk_add_test_cxx(${vtk-example}CxxTests tests
      NO_DATA NO_VALID NO_OUTPUT
      TestParticleReader.cxx
      TestDumpXMLFile.cxx
      )
    vtk_test_cxx_executable(${vtk-example}CxxTests tests
      RENDERING_FACTORY
      )
  endif ()
endif ()
