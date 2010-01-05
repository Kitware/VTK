IF (VTK_USE_X)
  PROJECT (MotifExamples)

  FIND_PACKAGE(Motif)

  IF (MOTIF_FOUND)
    INCLUDE(${CMAKE_ROOT}/Modules/CheckIncludeFileCXX.cmake)
    CHECK_INCLUDE_FILE_CXX("Xm/PushB.h" HAVE_XM_PUSHB_H)

    IF (HAVE_XM_PUSHB_H)
      # WARNING: The Motif library HAS to be placed BEFORE the Xt library
      # brought in by VTK (see Motif FAQ item 335)

      IF(NOT VTK_BINARY_DIR)
        FIND_PACKAGE(VTK)
        IF(NOT VTK_DIR)
          MESSAGE(FATAL_ERROR "Please set VTK_DIR.")
        ENDIF(NOT VTK_DIR)
        INCLUDE(${VTK_USE_FILE})
      ENDIF(NOT VTK_BINARY_DIR)

      SET(VTK_DEP_LIBS
        vtkRendering
        vtkGraphics
        vtkImaging
        vtkIO
        vtkFiltering
        vtkCommon
      )

      ADD_EXECUTABLE(Example1 Example1.cxx)
      ADD_EXECUTABLE(Example2 Example2.cxx)
      TARGET_LINK_LIBRARIES(Example1 ${MOTIF_LIBRARIES} ${X11_LIBRARIES} ${X11_Xt_LIB} ${VTK_DEP_LIBS})
      TARGET_LINK_LIBRARIES(Example2 ${MOTIF_LIBRARIES} ${X11_LIBRARIES} ${X11_Xt_LIB} ${VTK_DEP_LIBS})
    ELSE (HAVE_XM_PUSHB_H)
      MESSAGE(STATUS "Could not find required Motif header Xm/PushB.h. Not building Motif examples.")
    ENDIF (HAVE_XM_PUSHB_H)
  ELSE (MOTIF_FOUND)
    MESSAGE(STATUS "Could not find Motif libraries.  Not building Motif examples.")
  ENDIF (MOTIF_FOUND)
ENDIF (VTK_USE_X)
