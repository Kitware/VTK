vtk_add_test_cxx(vtkImagingCoreCxxTests tests
  FastSplatter.cxx
  ImageAccumulate.cxx,NO_VALID
  ImageAccumulateLarge.cxx,NO_VALID,NO_DATA,NO_OUTPUT 32
  ImageAutoRange.cxx
  ImageBSplineCoefficients.cxx
  ImageChangeInformation.cxx,NO_VALID,NO_DATA
  ImageDifference.cxx,NO_VALID
  ImageGenericInterpolateSlidingWindow3D.cxx
  ImageHistogram.cxx
  ImageHistogramStatistics.cxx,NO_VALID
  ImageInterpolateSlidingWindow2D.cxx
  ImageInterpolateSlidingWindow3D.cxx
  ImageInterpolator.cxx,NO_VALID,NO_DATA
  ImagePassInformation.cxx,NO_VALID,NO_DATA
  ImageResize.cxx
  ImageResize3D.cxx
  ImageResizeCropping.cxx
  ImageReslice.cxx
  ImageResliceDirection.cxx
  ImageResliceOriented.cxx
  ImageWeightedSum.cxx,NO_VALID
  ImportExport.cxx,NO_VALID
  TestBSplineWarp.cxx
  TestImageProbeFilter.cxx
  TestImageStencilDataMethods.cxx,NO_VALID
  TestImageStencilIterator.cxx,NO_VALID
  TestImageSSIM.cxx,NO_VALID
  TestStencilWithLasso.cxx
  TestStencilWithPolyDataContour.cxx
  TestStencilWithPolyDataSurface.cxx
  TestUpdateExtentReset.cxx,NO_VALID
  )
list(APPEND tests
  TestImageStencilData.cxx
  )

if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  set(_vtk_test_cxx_wasm_enabled_in_browser ON)
  set(_proxy_to_pthread_flag "0")
  if (VTK_WEBASSEMBLY_THREADS)
    set(_proxy_to_pthread_flag "1")
  endif ()
  set(_vtk_test_emscripten_extra_linker_args
    # O2 or higher make wasm-opt terribly slow.
    # test executables take a minute to link.
    "-O1"
    # don't leave runtime alive after exit() is called.
    "-sEXIT_RUNTIME=1"
    # this flag runs main() on a pthread i.e, a web worker.
    # the primary reason we desire this is to make the unit test access
    # data files through synchronous XHR which is only posssible from web-worker.
    "-sPROXY_TO_PTHREAD=${_proxy_to_pthread_flag}"
    "-sALLOW_MEMORY_GROWTH=1"
    "-sOFFSCREENCANVAS_SUPPORT=${_proxy_to_pthread_flag}")
  set(_vtk_test_cxx_pre_args
    "$<TARGET_FILE:Python3::Interpreter>"
    "${CMAKE_SOURCE_DIR}/Testing/WebAssembly/runner.py"
    "--engine=${VTK_TESTING_WASM_ENGINE}"
    "--engine-args=${VTK_TESTING_WASM_ENGINE_ARGUMENTS}"
    "--exit")
else ()
  set(_vtk_test_cxx_pre_args)
endif ()
# The stencil test is special
ExternalData_add_test(${_vtk_build_TEST_DATA_TARGET}
  NAME VTK::ImagingCoreCxx-AddStencilData
  COMMAND "${_vtk_test_cxx_pre_args}"
    "$<TARGET_FILE:vtkImagingCoreCxxTests>" TestImageStencilData 1
    -D ${_vtk_build_TEST_OUTPUT_DATA_DIRECTORY}
    -T ${_vtk_build_TEST_OUTPUT_DIRECTORY}
    -V DATA{../Data/Baseline/TestAddStencilData.png,:})
set_property(TEST VTK::ImagingCoreCxx-AddStencilData APPEND
  PROPERTY
    ENVIRONMENT "VTK_TESTING=1;VTK_TESTING_IMAGE_COMPARE_METHOD=TIGHT_VALID")
ExternalData_add_test(${_vtk_build_TEST_DATA_TARGET}
  NAME VTK::ImagingCoreCxx-SubtractStencilData
  COMMAND "${_vtk_test_cxx_pre_args}"
    "$<TARGET_FILE:vtkImagingCoreCxxTests>" TestImageStencilData 2
    -D ${_vtk_build_TEST_OUTPUT_DATA_DIRECTORY}
    -T ${_vtk_build_TEST_OUTPUT_DIRECTORY}
    -V DATA{../Data/Baseline/TestSubtractStencilData.png,:})
set_property(TEST VTK::ImagingCoreCxx-SubtractStencilData APPEND
  PROPERTY
    ENVIRONMENT "VTK_TESTING=1;VTK_TESTING_IMAGE_COMPARE_METHOD=TIGHT_VALID")
ExternalData_add_test(${_vtk_build_TEST_DATA_TARGET}
  NAME VTK::ImagingCoreCxx-ClipStencilData
  COMMAND "${_vtk_test_cxx_pre_args}"
    "$<TARGET_FILE:vtkImagingCoreCxxTests>" TestImageStencilData 3
    -D ${_vtk_build_TEST_OUTPUT_DATA_DIRECTORY}
    -T ${_vtk_build_TEST_OUTPUT_DIRECTORY}
    -V DATA{../Data/Baseline/TestClipStencilData.png,:})
set_property(TEST VTK::ImagingCoreCxx-ClipStencilData APPEND
  PROPERTY
    ENVIRONMENT "VTK_TESTING=1;VTK_TESTING_IMAGE_COMPARE_METHOD=TIGHT_VALID")

vtk_test_cxx_executable(vtkImagingCoreCxxTests tests
  DISABLE_FLOATING_POINT_EXCEPTIONS
  RENDERING_FACTORY
  )
