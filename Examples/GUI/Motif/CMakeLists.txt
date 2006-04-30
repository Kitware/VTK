IF (UNIX)
  PROJECT (MotifExamples)

  INCLUDE(${CMAKE_ROOT}/Modules/CheckIncludeFileCXX.cmake)
  CHECK_INCLUDE_FILE_CXX("Xm/PushB.h" HAVE_XM_PUSHB_H)

  IF (HAVE_XM_PUSHB_H)
    # WARNING: The Motif library HAS to be placed BEFORE the Xt library
    # brought in by VTK (see Motif FAQ item 335)

    LINK_LIBRARIES(-lXm)

    FIND_PACKAGE(VTK)
    IF(NOT VTK_DIR)
      MESSAGE(FATAL_ERROR "Please set VTK_DIR.")
    ENDIF(NOT VTK_DIR)
    INCLUDE(${VTK_USE_FILE})

    LINK_LIBRARIES( 
      vtkRendering
      vtkGraphics
      vtkImaging
      vtkIO
      vtkFiltering
      vtkCommon
    )

    ADD_EXECUTABLE(Example1 Example1.cxx)
    ADD_EXECUTABLE(Example2 Example2.cxx)
  ELSE (HAVE_XM_PUSHB_H)
    MESSAGE(STATUS "Could not find required Motif header Xm/PushB.h. Not building Motif examples.")
  ENDIF (HAVE_XM_PUSHB_H)
ENDIF (UNIX)
