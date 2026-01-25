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

# Create a custom target that tests can depend on to ensure the test JAR is
# built
add_custom_target(vtkjava_tests_jar ALL DEPENDS vtkjava_tests)

set(vtk_test_classpath
  "$<TARGET_FILE:vtkjava_tests>"
  "$<TARGET_FILE:VTK::vtkjava>")
string(REPLACE ";" "${classpath_separator}" vtk_test_classpath "${vtk_test_classpath}")

# Set up the PATH directory variables for Windows tests. Must be in native
# format.
if (WIN32)
  file(TO_NATIVE_PATH "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_JNILIBDIR}" vtk_jni_dir)
  file(TO_NATIVE_PATH "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}" vtk_java_bin_dir)
endif ()

set(vtkjava_tests_need_envmod 0)
function (vtkjava_test_env_properties testname)
  if (WIN32)
    set(vtkjava_tests_need_envmod 1 PARENT_SCOPE)
    set_property(TEST "${testname}" APPEND
      PROPERTY
        ENVIRONMENT_MODIFICATION
          "PATH=path_list_prepend:${vtk_java_bin_dir}"
          "PATH=path_list_prepend:${vtk_jni_dir}")
  elseif (APPLE)
    set(vtkjava_tests_need_envmod 1 PARENT_SCOPE)
    set_property(TEST "${testname}" APPEND
      PROPERTY
        ENVIRONMENT_MODIFICATION
          "DYLD_LIBRARY_PATH=path_list_append:${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
  endif ()
endfunction ()

if (TARGET VTK::RenderingOpenGL2)
  ExternalData_add_test(VTKData
    NAME    vtkJavaTests-Regression
    COMMAND "${Java_JAVA_EXECUTABLE}"
            -classpath "${vtk_test_classpath}"
            vtk.test.Regression
            -D "${CMAKE_BINARY_DIR}/ExternalData"
            -V "DATA{../Data/Baseline/Cone.png}"
            -T "${CMAKE_BINARY_DIR}/Testing/Temporary")
  set_property(TEST vtkJavaTests-Regression APPEND
    PROPERTY
      ENVIRONMENT
        "VTK_TESTING=1"
        "VTK_TESTING_IMAGE_COMPARE_METHOD=TIGHT_VALID")
  set_property(TEST vtkJavaTests-Regression APPEND
    PROPERTY
      REQUIRED_FILES "$<TARGET_FILE:vtkjava_tests>")
  vtkjava_test_env_properties(vtkJavaTests-Regression)
endif ()

foreach (java_simple_test IN LISTS java_simple_test_names)
  add_test(
    NAME    "vtkJavaTests-${java_simple_test}"
    COMMAND "${Java_JAVA_EXECUTABLE}"
            -classpath "${vtk_test_classpath}"
            "vtk.test.${java_simple_test}"
            -T "${CMAKE_BINARY_DIR}/Testing/Temporary")
  set_property(TEST "vtkJavaTests-${java_simple_test}" APPEND
    PROPERTY
      REQUIRED_FILES "$<TARGET_FILE:vtkjava_tests>")
  vtkjava_test_env_properties("vtkJavaTests-${java_simple_test}")
endforeach ()

if (vtkjava_tests_need_envmod AND CMAKE_VERSION VERSION_LESS "3.22")
  message(WARNING
    "CMake 3.22+ is required to reliably set up the environment for Java "
    "tests on macOS and Windows. Tests may fail due to missing libraries.")
endif ()
