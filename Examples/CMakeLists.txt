# VTK examples now require CMake 2.0 or higher.
CMAKE_MINIMUM_REQUIRED(VERSION 2.0)
IF(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 1.9)
ELSE(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 1.9)
  MESSAGE(FATAL_ERROR "VTKExamples requires CMake 2.0 or higher.")
ENDIF(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 1.9)

# Choose behavior based on whether we are building inside the VTK tree.
IF(VTK_BINARY_DIR)
  # We are building in the VTK tree.  Add tests for some examples.
  # Test the build system examples.
  IF(BUILD_TESTING)
    ADD_TEST(Example-vtkLocal ${CMAKE_CTEST_COMMAND}
      --build-and-test
      ${VTK_SOURCE_DIR}/Examples/Build/vtkLocal
      ${VTK_BINARY_DIR}/Examples/Build/vtkLocal
      --build-two-config
      --build-generator ${CMAKE_GENERATOR}
      --build-makeprogram ${MAKEPROGRAM}
      --build-project VTKLOCAL
      --build-run-dir "${EXECUTABLE_OUTPUT_PATH}/${CMAKE_CFG_INTDIR}"
      --build-options -DVTK_DIR:PATH=${VTK_BINARY_DIR}
      --test-command
      "${VTK_BINARY_DIR}/Examples/Build/vtkLocal/bin/vtkLocalTest"
      )
    ADD_TEST(Example-vtkMy ${CMAKE_CTEST_COMMAND}
      --build-and-test
      ${VTK_SOURCE_DIR}/Examples/Build/vtkMy
      ${VTK_BINARY_DIR}/Examples/Build/vtkMy
      --build-two-config
      --build-generator ${CMAKE_GENERATOR}
      --build-makeprogram ${MAKEPROGRAM}
      --build-project VTKMY
      --build-run-dir "${EXECUTABLE_OUTPUT_PATH}/${CMAKE_CFG_INTDIR}"
      --build-options -DVTK_DIR:PATH=${VTK_BINARY_DIR}
      --test-command
      "${VTK_BINARY_DIR}/Examples/Build/vtkMy/bin/vtkmyEx1"
      )
  ENDIF(BUILD_TESTING)

  # Build the examples as a separate project using a custom target.
  # Make sure it uses the same build configuration as VTK.
  # we must use the LCD IntDir instead of ConfigurationName which is
  # only supported in VC7 and above.
  IF(CMAKE_CONFIGURATION_TYPES)
    SET(VTKExamples_CONFIG_TYPE -C "$(IntDir)")
  ELSE(CMAKE_CONFIGURATION_TYPES)
    SET(VTKExamples_CONFIG_TYPE)
  ENDIF(CMAKE_CONFIGURATION_TYPES)
  ADD_CUSTOM_COMMAND(
    OUTPUT ${VTK_BINARY_DIR}/VTKExamples
    COMMAND ${CMAKE_CTEST_COMMAND}
    ARGS ${VTKExamples_CONFIG_TYPE}
         --build-and-test
         ${VTK_SOURCE_DIR}/Examples
         ${VTK_BINARY_DIR}/Examples/All
         --build-noclean
         --build-two-config
         --build-project VTKExamples
         --build-generator ${CMAKE_GENERATOR}
         --build-makeprogram ${MAKEPROGRAM}
         --build-options -DVTK_DIR:PATH=${VTK_BINARY_DIR}
                         -DCMAKE_C_COMPILER:STRING=${CMAKE_C_COMPILER}
                         -DCMAKE_C_FLAGS:STRING=${CMAKE_C_FLAGS}
                         -DCMAKE_C_FLAGS_DEBUG:STRING=${CMAKE_C_FLAGS_DEBUG}
                         -DCMAKE_C_FLAGS_RELEASE:STRING=${CMAKE_C_FLAGS_RELEASE}
                         -DCMAKE_C_FLAGS_MINSIZEREL:STRING=${CMAKE_C_FLAGS_MINSIZEREL}
                         -DCMAKE_C_FLAGS_RELWITHDEBINFO:STRING=${CMAKE_C_FLAGS_RELWITHDEBINFO}
                         -DCMAKE_CXX_COMPILER:STRING=${CMAKE_CXX_COMPILER}
                         -DCMAKE_CXX_FLAGS:STRING=${CMAKE_CXX_FLAGS}
                         -DCMAKE_CXX_FLAGS_DEBUG:STRING=${CMAKE_CXX_FLAGS_DEBUG}
                         -DCMAKE_CXX_FLAGS_RELEASE:STRING=${CMAKE_CXX_FLAGS_RELEASE}
                         -DCMAKE_CXX_FLAGS_MINSIZEREL:STRING=${CMAKE_CXX_FLAGS_MINSIZEREL}
                         -DCMAKE_CXX_FLAGS_RELWITHDEBINFO:STRING=${CMAKE_CXX_FLAGS_RELWITHDEBINFO}
                         -DEXECUTABLE_OUTPUT_PATH:PATH=${EXECUTABLE_OUTPUT_PATH}
                         -DLIBRARY_OUTPUT_PATH:PATH=${LIBRARY_OUTPUT_PATH}
         )
  ADD_CUSTOM_TARGET(VTKExamplesTarget ALL DEPENDS ${VTK_BINARY_DIR}/VTKExamples)

  # Make sure the target builds after the rest of VTK.
  ADD_DEPENDENCIES(VTKExamplesTarget
    vtkCommon
    vtkFiltering
    vtkImaging
    vtkGraphics
    vtkGenericFiltering
    vtkIO
    )
  IF(VTK_USE_RENDERING)
    ADD_DEPENDENCIES(VTKExamplesTarget vtkRendering)
    ADD_DEPENDENCIES(VTKExamplesTarget vtkVolumeRendering)
    ADD_DEPENDENCIES(VTKExamplesTarget vtkHybrid)
    ADD_DEPENDENCIES(VTKExamplesTarget vtkWidgets)
  ENDIF(VTK_USE_RENDERING)
  IF(VTK_USE_PARALLEL)
    ADD_DEPENDENCIES(VTKExamplesTarget vtkParallel)
  ENDIF(VTK_USE_PARALLEL)

ELSE(VTK_BINARY_DIR)
  # We are building outside the VTK tree.  Build the examples directly.
  PROJECT(VTKExamples)

  # Find and load the VTK configuration.
  FIND_PACKAGE(VTK)
  IF(NOT VTK_DIR)
    MESSAGE(FATAL_ERROR "Please set VTK_DIR.")
  ENDIF(NOT VTK_DIR)
  INCLUDE(${VTK_USE_FILE})

  # Most examples require rendering support.
  IF(VTK_USE_RENDERING)
    SUBDIRS(
      AMR/Cxx
      MultiBlock/Cxx
      DataManipulation/Cxx
      Medical/Cxx
      Modelling/Cxx
      Rendering/Cxx
      Tutorial/Step1/Cxx
      Tutorial/Step2/Cxx
      Tutorial/Step3/Cxx
      Tutorial/Step4/Cxx
      Tutorial/Step5/Cxx
      Tutorial/Step6/Cxx
      )

    IF(VTK_USE_PARALLEL)
      SUBDIRS(ParallelProcessing)
    ENDIF(VTK_USE_PARALLEL)

    IF(VTK_USE_QVTK)
      SUBDIRS(GUI/Qt)
    ENDIF(VTK_USE_QVTK)

    IF(VTK_USE_MFC)
      SUBDIRS(GUI/Win32/vtkMFC)
    ENDIF(VTK_USE_MFC)
  ENDIF(VTK_USE_RENDERING)
ENDIF(VTK_BINARY_DIR)
