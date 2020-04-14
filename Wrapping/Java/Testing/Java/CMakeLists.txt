find_package(Java REQUIRED COMPONENTS Runtime Development)

set(java_simple_test_names
  ConcurrencyGC
  JavaDelete
  ManualGC
  JavaGCAndDelete)

set(java_test_names
  Regression
  ${java_simple_test_names})

set(java_test_files)
foreach (java_test IN LISTS java_test_names)
  list(APPEND java_test_files
    "${CMAKE_CURRENT_SOURCE_DIR}/vtk/test/${java_test}.java")
endforeach ()

set(classpath_separator ":")
if (WIN32)
  set(classpath_separator "\\;")
endif ()
set(vtk_test_classpath
  "$<TARGET_FILE:VTK::vtkjava>")
string(REPLACE ";" "${classpath_separator}" vtk_test_classpath "${vtk_test_classpath}")

add_library(vtkjava_tests STATIC
  ${java_test_files})
target_compile_options(vtkjava_tests
  PRIVATE
    "SHELL:-classpath \"${vtk_test_classpath}\""
    "SHELL:-source ${VTK_JAVA_SOURCE_VERSION}"
    "SHELL:-target ${VTK_JAVA_TARGET_VERSION}")
target_link_libraries(vtkjava_tests
  PRIVATE
    VTK::vtkjava)

set(vtk_test_classpath
  "$<TARGET_FILE:vtkjava_tests>"
  "$<TARGET_FILE:VTK::vtkjava>")
string(REPLACE ";" "${classpath_separator}" vtk_test_classpath "${vtk_test_classpath}")

if (TARGET VTK::RenderingOpenGL2)
  ExternalData_add_test(VTKData
    NAME    vtkJavaTests-Regression
    COMMAND "${Java_JAVA_EXECUTABLE}"
            -classpath "${vtk_test_classpath}"
            vtk.test.Regression
            -D "${CMAKE_BINARY_DIR}/ExternalData"
            -V "DATA{../Data/Baseline/Cone.png}"
            -T "${CMAKE_BINARY_DIR}/Testing/Temporary")
endif ()

foreach (java_simple_test IN LISTS java_simple_test_names)
  add_test(
    NAME    "vtkJavaTests-${java_simple_test}"
    COMMAND "${Java_JAVA_EXECUTABLE}"
            -classpath "${vtk_test_classpath}"
            "vtk.test.${java_simple_test}"
            -T "${CMAKE_BINARY_DIR}/Testing/Temporary")
endforeach ()
