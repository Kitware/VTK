include(../../../../CMake/vtkGhostscript.cmake)

# Only run GL2PS tests if GhostScript is available
if(VTK_GHOSTSCRIPT_EXECUTABLE)
  set(GL2PSTests
    TestGL2PSBillboardTextActor3D.cxx
    TestGL2PSContext.cxx # No image export supported in OGL
    TestGL2PSLabeledDataMapper.cxx
    TestGL2PSScalarBar.cxx
    TestGL2PSTextActor3D.cxx
    TestGL2PSTextActor.cxx
    TestGL2PSTextMapper.cxx
    TestStackedPlotGL2PS.cxx
    )

  set(GL2PSTestsPDFBothOpenGL
    TestGL2PSExporterMultipleRenderers.cxx
    TestGL2PSExporterRaster.cxx
    TestGL2PSExporterVolumeRaster.cxx
    )

  set(GL2PSTestsPDF
    ${GL2PSTestsPDFBothOpenGL}
  )

  vtk_add_test_cxx(vtkIOExportGL2PSCxxTests tests
    ${GL2PSTests}
    ${GL2PSTestsPDF}
    )
  vtk_test_cxx_executable(vtkIOExportGL2PSCxxTests tests RENDERING_FACTORY)

  set(TestGL2PSExporterVolumeRasterError 50)

  include(../../../../CMake/vtkTestingGL2PS.cmake)
  # text alignment and rotation with TextAsPath off is not
  # yet implemented for the OpenGL2 backend.
  vtk_add_gl2ps_test_cxx(${GL2PSTests})
  # test postscript and pdf
  vtk_add_gl2ps_test_cxx(pdf ${GL2PSTestsPDFBothOpenGL})
endif()
