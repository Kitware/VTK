if(VTK_GHOSTSCRIPT_EXECUTABLE)
  set(PDFTests
    TestPDFContextExport.cxx,NO_DATA
    TestPDFContextShading.cxx,NO_DATA
    TestPDFExporterContextPolyData.cxx
  )

  vtk_add_test_cxx(vtkIOExportPDFCxxTests tests
    ${PDFTests}
    )
  vtk_test_cxx_executable(vtkIOExportPDFCxxTests tests
    RENDERING_FACTORY
    )

  include(vtkTestingGL2PS)
  vtk_add_pdf_test_cxx(${PDFTests})
endif()
