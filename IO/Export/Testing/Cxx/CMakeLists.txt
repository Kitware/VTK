include(vtkGhostscript)
include(vtkWkHtmlToImage)

if (VTK_WKHTMLTOIMAGE_EXECUTABLE)
  set(SVGTests
    TestSVGContextExport.cxx,NO_DATA,NO_VALID
    TestSVGContextShading.cxx,NO_DATA,NO_VALID
    TestSVGExporterContextPolyData.cxx,NO_VALID
    )
endif()

# Only run GL2PS tests if GhostScript is available
if(VTK_GHOSTSCRIPT_EXECUTABLE)
  set(GL2PSTests
    TestGL2PSBillboardTextActor3D.cxx
    TestGL2PSScalarBar.cxx
    TestGL2PSTextActor.cxx
    TestGL2PSTextActor3D.cxx
    TestGL2PSTextMapper.cxx
    TestGL2PSLabeledDataMapper.cxx
    TestStackedPlotGL2PS.cxx
    )

  set(GL2PSTestsPDFBothOpenGL
    TestGL2PSExporterRaster.cxx
    TestGL2PSExporterMultipleRenderers.cxx
    TestGL2PSExporterVolumeRaster.cxx
    )
  set(GL2PSTestsPDF
    ${GL2PSTestsPDFBothOpenGL}
  )
endif()

vtk_add_test_cxx(vtkIOExportCxxTests tests
  X3DTest.cxx,NO_DATA,NO_VALID
  TestOBJExporter.cxx,NO_DATA,NO_VALID
  TestSingleVTPExporter.cxx,NO_DATA,NO_VALID
  ${GL2PSTests} ${GL2PSTestsPDF} ${SVGTests}
  TestRIBExporter.cxx,NO_VALID
  UnitTestRIB.cxx,NO_DATA,NO_VALID
  )
vtk_test_cxx_executable(vtkIOExportCxxTests tests
  RENDERING_FACTORY
  )

# See if Pixar's Renderman is available
find_path(RMANTREE bin/prman HINTS
          /Applications/Pixar/RenderManProServer-19.0
          /opt/pixar/RenderManProServer-19.0
)
mark_as_advanced(RMANTREE)

# Tests to run if Renderman is present
if(RMANTREE)
  # write a Renderman ini file to specify the shaderpath
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/rendermn.ini
          "/shaderpath &:${CMAKE_CURRENT_SOURCE_DIR}/Testing/Cxx")
  add_test(NAME vtkIOExport-RIBExporterShaderCompile
          COMMAND ${RMANTREE}/bin/shader
          ${CMAKE_CURRENT_SOURCE_DIR}/../Data/Input/bozo.sl
          ${CMAKE_CURRENT_SOURCE_DIR}/../Data/Input/dented.sl
          WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
  add_test(NAME vtkIOExport-RIBExporterPrman
          COMMAND ${RMANTREE}/bin/prman
            ${VTK_TEST_OUTPUT_DIR}/TestRIBExporter.rib
          WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
  # Image diff rman generated tif with baseline
  set(baseline_dir ${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline)
  ExternalData_add_test(VTKData
        NAME vtkIOExportCxx-ComparePrmanImage
        COMMAND ${RMANTREE}/bin/cmptiff -d 100
        DATA{${baseline_dir}/TestRIBExporter.tif}
        ${VTK_TEST_OUTPUT_DIR}/TestRIBExporter.tif
  )
  set_tests_properties(vtkIOExport-RIBExporterPrman
        PROPERTIES
        DEPENDS
        "vtkIOExport-TestRIBExporter"
        DEPENDS
        "vtkIOExport-RIBExporterShaderCompile"
        ENVIRONMENT
        "RMANTREE=${RMANTREE}"
  )
  set_tests_properties("vtkIOExportCxx-ComparePrmanImage"
        PROPERTIES
        DEPENDS "vtkIOExportCxx-TestRIBExporter"
        REQUIRED_FILES
          "${VTK_TEST_OUTPUT_DIR}/TestRIBExporter.tif"
  )
endif()

set(TestGL2PSExporterVolumeRasterError 50)

include(vtkTestingGL2PS)
# text alignment and rotation with TextAsPath off is not
# yet implemented for the OpenGL2 backend.
vtk_add_gl2ps_test_cxx(${GL2PSTests})
# test postscript and pdf
vtk_add_gl2ps_test_cxx(pdf ${GL2PSTestsPDFBothOpenGL})

include(vtkTestingSVG)
vtk_add_svg_test(${SVGTests})
