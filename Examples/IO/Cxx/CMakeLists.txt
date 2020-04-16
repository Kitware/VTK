cmake_minimum_required(VERSION 3.8...3.12 FATAL_ERROR)
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
  include(CTest)
  if (TARGET VTK::TestingRendering)
    ######## Regression Testing ########
    set(ExternalData_BINARY_ROOT ${CMAKE_CURRENT_BINARY_DIR}/ExternalData)

    list(APPEND ExternalData_URL_TEMPLATES
      # Data published by Girder
      "https://data.kitware.com/api/v1/file/hashsum/%(algo)/%(hash)/download"

      # Data published by developers using git-gitlab-push.
      "https://www.vtk.org/files/ExternalData/%(algo)/%(hash)"
    )
    include(ExternalData)

    ExternalData_add_test(IOData
      NAME    "IOExampleCxx-TestDumpXMLFile"
      COMMAND DumpXMLFile
              DATA{${CMAKE_CURRENT_SOURCE_DIR}/Testing/Data/cow.vtp}
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

    ExternalData_add_test(IOData
      NAME    "IOExampleCxx-TestParticleReader"
      COMMAND ParticleReader
              DATA{${CMAKE_CURRENT_SOURCE_DIR}/Testing/Data/golf.csv}
              ${CMAKE_CURRENT_BINARY_DIR}/Testing/Temporary/TestParticleReader.vtp
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

    ExternalData_Add_Target(IOData)
  endif ()
endif ()
