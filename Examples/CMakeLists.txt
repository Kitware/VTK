PROJECT(VTKExamples)

# Tell the examples where to find VTK.
IF(VTK_BINARY_DIR)
  # Building inside the VTK tree.  Directly specify the location.
  SET(VTK_DIR ${VTK_BINARY_DIR})

  # Prevent the UseVTK.cmake file from duplicating build settings in
  # this tree.
  SET(VTK_USE_FILE_INCLUDED 1)
ENDIF(VTK_BINARY_DIR)

# Load the VTK configuration.
FIND_PACKAGE(VTK)
IF(NOT VTK_DIR)
  MESSAGE(FATAL_ERROR "Please set VTK_DIR.")
ENDIF(NOT VTK_DIR)
INCLUDE(${VTK_USE_FILE})

# Build most examples.
SUBDIRS(
  #AMR/Cxx
  DataManipulation/Cxx
  Medical/Cxx
  Modelling/Cxx
  #MultiBlock/Cxx
  Tutorial/Step1/Cxx
  Tutorial/Step2/Cxx
  Tutorial/Step3/Cxx
  Tutorial/Step4/Cxx
  Tutorial/Step5/Cxx
  Tutorial/Step6/Cxx
  )

# Build optional examples.
IF (VTK_USE_RENDERING)
  SUBDIRS(Rendering/Cxx)
ENDIF (VTK_USE_RENDERING)

IF (VTK_USE_PARALLEL)
  SUBDIRS(ParallelProcessing)
ENDIF (VTK_USE_PARALLEL)

IF(VTK_USE_QVTK)
  SUBDIRS(GUI/Qt)
ENDIF(VTK_USE_QVTK)

IF(VTK_USE_MFC)
  SUBDIRS(GUI/Win32/vtkMFC)
ENDIF(VTK_USE_MFC)

# Test the build system examples.
IF(VTK_BINARY_DIR AND BUILD_TESTING)
  IF(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 1.8)
    ADD_TEST(Example-vtkLocal ${CMAKE_CTEST_COMMAND}
      --build-and-test
      ${VTK_SOURCE_DIR}/Examples/Build/vtkLocal
      ${VTK_BINARY_DIR}/Examples/Build/vtkLocal
      --build-two-config
      --build-generator ${CMAKE_GENERATOR}
      --build-makeprogram ${MAKEPROGRAM}
      --build-project VTKLOCAL
      --build-options -DVTK_DIR:PATH=${VTK_BINARY_DIR}
      --test-command
      ${VTK_BINARY_DIR}/Examples/Build/vtkLocal/bin/vtkLocalTest
      )
    ADD_TEST(Example-vtkMy ${CMAKE_CTEST_COMMAND}
      --build-and-test
      ${VTK_SOURCE_DIR}/Examples/Build/vtkMy
      ${VTK_BINARY_DIR}/Examples/Build/vtkMy
      --build-two-config
      --build-generator ${CMAKE_GENERATOR}
      --build-makeprogram ${MAKEPROGRAM}
      --build-project VTKLOCAL
      --build-options -DVTK_DIR:PATH=${VTK_BINARY_DIR}
      --test-command
      ${VTK_BINARY_DIR}/Examples/Build/vtkMy/bin/vtkmyEx1
      )
  ENDIF(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 1.8)
ENDIF(VTK_BINARY_DIR AND BUILD_TESTING)
