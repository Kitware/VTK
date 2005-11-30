# VTK examples now require CMake 2.0 or higher.
CMAKE_MINIMUM_REQUIRED(VERSION 2.0)
IF(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 1.9)
ELSE(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 1.9)
  MESSAGE(FATAL_ERROR "VTKExamples requires CMake 2.0 or higher.")
ENDIF(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 1.9)

# Choose behavior based on whether we are building inside the VTK tree.
IF(VTK_BINARY_DIR)
  # We are building in the VTK tree.  Add tests for some examples.
  # Test the build system examples.  These tests can be run only
  # when the configuration type is known when running the test.
  IF(BUILD_TESTING AND VTK_TEST_CONFIG_TYPE_KNOWN)
    # If multiple configuration types are available we must add the test
    # in a way that will use the configuration given for testing.  When
    # CTest 2.2 and higher is used to drive the tests it evaluates the
    # ADD_TEST commands stored in the test files using standard cmake
    # language evaluation.  The variable CTEST_CONFIGURATION_TYPE expands
    # to the configuration specified for testing (Debug, Release, etc.).
    # This variable must be stored in the test string and evaluated at
    # testing time, so the dollar must be stored in a way that will not be
    # evaluated immediately.  In CMake 2.2 and higher this can be done
    # with an escaped dollar but we must support older CMake versions
    # syntactically.
    IF(CMAKE_CONFIGURATION_TYPES)
      # We need to pass the configuration type on the test command line.
      SET(DOLLAR "$")
      SET(VTK_EXAMPLE_TEST_CONFIG_TYPE -C "${DOLLAR}{CTEST_CONFIGURATION_TYPE}")
      SET(VTK_EXAMPLE_TEST_RUN_DIR "${EXECUTABLE_OUTPUT_PATH}/${DOLLAR}{CTEST_CONFIGURATION_TYPE}")
    ELSE(CMAKE_CONFIGURATION_TYPES)
      # There is only one configuration type so it need not be passed.
      SET(VTK_EXAMPLE_TEST_CONFIG_TYPE)
      SET(VTK_EXAMPLE_TEST_RUN_DIR "${EXECUTABLE_OUTPUT_PATH}")
    ENDIF(CMAKE_CONFIGURATION_TYPES)

    # Add a test to build the vtkLocal example.
    ADD_TEST(Example-vtkLocal ${CMAKE_CTEST_COMMAND}
      ${VTK_EXAMPLE_TEST_CONFIG_TYPE}
      --build-and-test
      ${VTK_SOURCE_DIR}/Examples/Build/vtkLocal
      ${VTK_BINARY_DIR}/Examples/Build/vtkLocal
      --build-two-config
      --build-generator ${CMAKE_GENERATOR}
      --build-makeprogram ${MAKEPROGRAM}
      --build-project VTKLOCAL
      --build-run-dir ${VTK_EXAMPLE_TEST_RUN_DIR}
      --build-options -DVTK_DIR:PATH=${VTK_BINARY_DIR}
      --test-command "${VTK_BINARY_DIR}/Examples/Build/vtkLocal/bin/vtkLocalTest"
      )

    # Add a test to build the vtkMy example.
    ADD_TEST(Example-vtkMy ${CMAKE_CTEST_COMMAND}
      ${VTK_EXAMPLE_TEST_CONFIG_TYPE}
      --build-and-test
      ${VTK_SOURCE_DIR}/Examples/Build/vtkMy
      ${VTK_BINARY_DIR}/Examples/Build/vtkMy
      --build-two-config
      --build-generator ${CMAKE_GENERATOR}
      --build-makeprogram ${MAKEPROGRAM}
      --build-project VTKMY
      --build-run-dir ${VTK_EXAMPLE_TEST_RUN_DIR}
      --build-options -DVTK_DIR:PATH=${VTK_BINARY_DIR}
      --test-command "${VTK_BINARY_DIR}/Examples/Build/vtkMy/bin/vtkmyEx1"
      )
  ENDIF(BUILD_TESTING AND VTK_TEST_CONFIG_TYPE_KNOWN)

  # Build the examples as a separate project using a custom target.
  # Make sure it uses the same build configuration as VTK.
  IF(CMAKE_CONFIGURATION_TYPES)
    SET(VTKExamples_CONFIG_TYPE -C "${CMAKE_CFG_INTDIR}")
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
  IF(VTK_USE_QVTK)
    ADD_DEPENDENCIES(VTKExamplesTarget QVTK)
  ENDIF(VTK_USE_QVTK)
  IF(VTK_USE_MFC)
    ADD_DEPENDENCIES(VTKExamplesTarget vtkMFC)
  ENDIF(VTK_USE_MFC)

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
