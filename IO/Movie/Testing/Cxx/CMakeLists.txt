set(TEST_SRC)

if(WIN32)
  if (VTK_USE_VIDEO_FOR_WINDOWS)
    list(APPEND TEST_SRC TestAVIWriter.cxx)
  endif()

  if (VTK_USE_MICROSOFT_MEDIA_FOUNDATION)
    list(APPEND TEST_SRC TestMP4Writer.cxx)
  endif()
endif()

vtk_add_test_cxx(vtkIOMovieCxxTests tests
  NO_VALID
  ${TEST_SRC}
  )

vtk_test_cxx_executable(vtkIOMovieCxxTests tests)
