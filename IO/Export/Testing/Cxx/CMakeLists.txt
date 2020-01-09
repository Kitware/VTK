include("${CMAKE_CURRENT_LIST_DIR}/vtkWkHtmlToImage.cmake")

if (VTK_WKHTMLTOIMAGE_EXECUTABLE)
  set(SVGTests
    TestSVGContextExport.cxx,NO_DATA,NO_VALID
    TestSVGContextShading.cxx,NO_DATA,NO_VALID
    TestSVGExporterContextPolyData.cxx,NO_VALID
    )
endif()


vtk_add_test_cxx(vtkIOExportCxxTests tests
  X3DTest.cxx,NO_DATA,NO_VALID
  TestJSONRenderWindowExporter.cxx,NO_DATA,NO_VALID
  TestOBJExporter.cxx,NO_DATA,NO_VALID
  TestGLTFExporter.cxx,NO_DATA,NO_VALID
  TestSingleVTPExporter.cxx,NO_DATA,NO_VALID
  ${SVGTests}
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
  add_test(NAME VTK::IOExport-RIBExporterShaderCompile
          COMMAND ${RMANTREE}/bin/shader
          ${CMAKE_CURRENT_SOURCE_DIR}/../Data/Input/bozo.sl
          ${CMAKE_CURRENT_SOURCE_DIR}/../Data/Input/dented.sl
          WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
  add_test(NAME VTK::IOExport-RIBExporterPrman
          COMMAND ${RMANTREE}/bin/prman
            ${_vtk_build_TEST_OUTPUT_DIRECTORY}/TestRIBExporter.rib
          WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
  # Image diff rman generated tif with baseline
  set(baseline_dir ${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline)
  ExternalData_add_test(${_vtk_build_TEST_DATA_TARGET}
        NAME VTK::IOExportCxx-ComparePrmanImage
        COMMAND ${RMANTREE}/bin/cmptiff -d 100
        DATA{${baseline_dir}/TestRIBExporter.tif}
        ${_vtk_build_TEST_OUTPUT_DIRECTORY}/TestRIBExporter.tif
  )
  set_tests_properties(VTK::IOExport-RIBExporterPrman
        PROPERTIES
        DEPENDS
        "VTK::IOExport-TestRIBExporter"
        DEPENDS
        "VTK::IOExport-RIBExporterShaderCompile"
        ENVIRONMENT
        "RMANTREE=${RMANTREE}"
  )
  set_tests_properties("VTK::IOExportCxx-ComparePrmanImage"
        PROPERTIES
        DEPENDS "VTK::IOExportCxx-TestRIBExporter"
        REQUIRED_FILES
          "${_vtk_build_TEST_OUTPUT_DIRECTORY}/TestRIBExporter.tif"
  )
endif()

include("${CMAKE_CURRENT_LIST_DIR}/vtkTestingSVG.cmake")
vtk_add_svg_test(${SVGTests})
