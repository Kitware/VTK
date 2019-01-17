find_package(Java REQUIRED COMPONENTS Runtime Development)

set(java_simple_test_names
  ConcurrencyGC
  JavaDelete
  ManualGC
  JavaGCAndDelete)

set(java_test_names
  Regression
  ${java_simple_test_names})

set(vtk_java_class_output
  "${CMAKE_CURRENT_BINARY_DIR}/class")
file(MAKE_DIRECTORY "${vtk_java_class_output}")

set(java_test_files)
set(java_test_classes)
foreach (java_test IN LISTS java_test_names)
  list(APPEND java_test_files
    "${CMAKE_CURRENT_SOURCE_DIR}/vtk/test/${java_test}.java")
  list(APPEND java_test_classes
    "${vtk_java_class_output}/vtk/test/${java_test}.class")
endforeach ()

if (CMAKE_GENERATOR MATCHES "Ninja")
  set(vtk_jar_depends "${vtk_jar_file}")
else ()
  set(vtk_jar_depends "vtkjava")
endif ()

# Compile Java Tests
add_custom_command(
  OUTPUT  ${java_test_classes}
  COMMAND ${Java_JAVAC_EXECUTABLE}
          -classpath "${vtk_jar_file}"
          -source "${VTK_JAVA_SOURCE_VERSION}"
          -target "${VTK_JAVA_TARGET_VERSION}"
          -d "${vtk_java_class_output}"
          ${java_test_files}
  DEPENDS "${vtk_jar_depends}"
          ${java_test_files}
  COMMENT "Compiling Java Tests")
add_custom_target(vtkJavaTests ALL
  DEPENDS
    ${java_test_classes})

set(classpath_separator ":")
if (WIN32)
  set(classpath_separator "\\;")
endif ()

set(vtk_test_classpath
  "${vtk_jar_file}${classpath_separator}${vtk_java_class_output}")

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
