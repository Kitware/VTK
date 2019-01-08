include(../../../../CMake/vtkGhostscript.cmake)

if(VTK_GHOSTSCRIPT_EXECUTABLE)
  set(PDFTests
    TestPDFContextExport.cxx,NO_DATA
    TestPDFContextShading.cxx,NO_DATA
    TestPDFExporterContextPolyData.cxx
    TestPDFTransformedText.cxx,NO_DATA
  )

  vtk_add_test_cxx(vtkIOExportPDFCxxTests tests
    ${PDFTests}
    )
  vtk_test_cxx_executable(vtkIOExportPDFCxxTests tests
    RENDERING_FACTORY
    )

  include(../../../../CMake/vtkTestingPDF.cmake)
  vtk_add_pdf_test_cxx(${PDFTests})
endif()
