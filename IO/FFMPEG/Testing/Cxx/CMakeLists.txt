vtk_add_test_cxx(vtkIOFFMPEGCxxTests tests
  TestFFMPEGWriter.cxx,NO_VALID
  )

if (VTK_FFMPEG_AVCODEC_SEND_PACKET)
  vtk_add_test_cxx(vtkIOFFMPEGCxxTests tests
    TestFFMPEGVideoSource.cxx
    )
if (WIN32)
  vtk_add_test_cxx(vtkIOFFMPEGCxxTests tests
    TestFFMPEGVideoSourceWithAudio.cxx
    )
  set(extra_test_libs Xaudio2.lib)
endif()
endif()

vtk_test_cxx_executable(vtkIOFFMPEGCxxTests tests)
if (extra_test_libs)
set_property(TARGET vtkIOFFMPEGCxxTests APPEND PROPERTY LINK_LIBRARIES ${extra_test_libs})
endif()
