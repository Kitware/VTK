set(GL2PSTests
  TestGL2PSMathTextOutput.cxx
  TestGL2PSMathTextScaling.cxx
  TestMathTextActor.cxx
  TestMathTextActor3D.cxx
)

if(VTK_DATA_ROOT)
  # add tests that require data
  set(MyTests
    TestContextMathTextImage.cxx
    TestRenderString.cxx
    TestStringToPath.cxx
    ${GL2PSTests}
    )
endif()

# Use the testing object factory, to reduce boilerplate code in tests.
include("${vtkTestingRendering_SOURCE_DIR}/vtkTestingObjectFactory.cmake")
vtk_module_test_executable(${vtk-module}CxxTests ${Tests})

set(TestsToRun ${Tests})
remove(TestsToRun CxxTests.cxx)

# Add all the executables
foreach(test ${TestsToRun})
  get_filename_component(TName ${test} NAME_WE)
  if(VTK_DATA_ROOT)
    if(${${TName}Error})
      set(_error_threshold ${${TName}Error})
    else()
      set(_error_threshold 10)
    endif()
    add_test(NAME ${vtk-module}Cxx-${TName}
      COMMAND ${vtk-module}CxxTests ${TName}
        -D ${VTK_DATA_ROOT}
        -T ${VTK_TEST_OUTPUT_DIR}
        -V Baseline/Rendering/${TName}.png
        -E ${_error_threshold})
  else()
    add_test(NAME MathText-${TName}
      COMMAND ${vtk-module}CxxTests ${TName})
  endif()
endforeach()

# The GL2PS tests produce postscript output, which cannot be compared using
# the built-in image regression framework. Instead, calculate and compare
# md5 sums using a CMake script.
# The file(MD5 ...) subcommand is not available in CMake < 2.8.7
if("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" GREATER 2.8 OR
   ("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" EQUAL 2.8 AND
    "${CMAKE_PATCH_VERSION}" GREATER 7))
  set(CMAKE_HAS_FILE_MD5_SUBCOMMAND TRUE)
endif()
if(CMAKE_HAS_FILE_MD5_SUBCOMMAND AND GL2PSTests AND VTK_DATA_ROOT)
  foreach(test ${GL2PSTests})
    get_filename_component(TName ${test} NAME_WE)
    add_test(NAME ${vtk-module}Cxx-${TName}-CheckMD5sum
      COMMAND ${CMAKE_COMMAND}
        -DTESTFILE=${VTK_BINARY_DIR}/Testing/Temporary/${TName}.ps
        -DGLOBEXP=${VTK_DATA_ROOT}/Baseline/Rendering/${TName}*.ps
        -DCLEANTESTFILE=1
        -P ${VTK_SOURCE_DIR}/CMake/CompareFileMD5.cmake
    )
    set_tests_properties("${vtk-module}Cxx-${TName}-CheckMD5sum"
      PROPERTIES
        DEPENDS "${vtk-module}Cxx-${TName}"
        REQUIRED_FILES
          "${VTK_DATA_ROOT}/Baseline/Rendering/${TName}.ps;${VTK_BINARY_DIR}/Testing/Temporary/${TName}.ps"
        FAIL_REGULAR_EXPRESSION "MATCH_FAIL"
    )
  endforeach()
endif()
