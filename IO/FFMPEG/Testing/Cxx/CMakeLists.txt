vtk_add_test_cxx(vtkIOFFMPEGCxxTests tests
  TestFFMPEGWriter.cxx,NO_VALID
  )

if (VTK_FFMPEG_AVCODEC_SEND_PACKET)
  vtk_add_test_cxx(vtkIOFFMPEGCxxTests tests
    TestFFMPEGVideoSource.cxx
    )
endif()

vtk_test_cxx_executable(vtkIOFFMPEGCxxTests tests)
