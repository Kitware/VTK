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
    "SHELL:--release ${VTK_JAVA_RELEASE_VERSION}")
target_link_libraries(vtkjava_tests
  PRIVATE
    VTK::vtkjava)
if (TARGET vtkjava_jar)
  # Apparently the Java compilation doesn't set up dependencies properly,
  # Thus we need to depend on vtkjava_jar which symbolized that the vtkjava
  # jar file has been created. This is needed since compiling every java file
  # requires to have the jars in the classpath of its dependencies.
  add_dependencies(vtkjava_tests vtkjava_jar WrapJava)
else ()
  # If 'vtkjava_jar' doesn't exist, we assume that dependencies are properly
  # handled via the 'vtkjava' target for the vtk.jar file
  add_dependencies(vtkjava_tests vtkjava)
endif ()

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
  set_property(TEST vtkJavaTests-Regression
    PROPERTY
      ENVIRONMENT "VTK_TESTING=1;VTK_TESTING_IMAGE_COMPARE_METHOD=TIGHT_VALID")
endif ()

foreach (java_simple_test IN LISTS java_simple_test_names)
  add_test(
    NAME    "vtkJavaTests-${java_simple_test}"
    COMMAND "${Java_JAVA_EXECUTABLE}"
            -classpath "${vtk_test_classpath}"
            "vtk.test.${java_simple_test}"
            -T "${CMAKE_BINARY_DIR}/Testing/Temporary")
endforeach ()
