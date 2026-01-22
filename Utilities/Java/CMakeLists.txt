# TODO: Update FindJNI to use imported targets.
find_package(JNI REQUIRED)

set(classes
  vtkJavaUtil)

set(headers
  vtkJavaAwt.h)

vtk_module_add_module(VTK::Java
  CLASSES ${classes}
  HEADERS ${headers}
)

foreach(jni_include_dir IN LISTS JNI_INCLUDE_DIRS)
  # We cannot pass a list to vtk_module_include inside a BUILD_INTERFACE
  vtk_module_include(VTK::Java SYSTEM PUBLIC
    "$<BUILD_INTERFACE:${jni_include_dir}>"
  )
endforeach()

vtk_module_link(VTK::Java
  PUBLIC
    ${JAVA_JVM_LIBRARY}
)

# On Windows and macOS, we need to link against jawt for JAWT_GetAWT.
# On Linux, avoid linking against AWT libraries - linking against
# JAVA_AWT_LIBRARY causes early loading of libawt_xawt.so which fails with
# "undefined symbol: JNU_ThrowNoSuchFieldError" on some JDK versions because
# the JVM's internal symbols aren't yet available. JAWT is loaded dynamically
# at runtime on Linux.
if (APPLE OR NOT UNIX)
  if (JAVA_AWT_LIBRARY)
    vtk_module_link(VTK::Java
      PUBLIC
        ${JAVA_AWT_LIBRARY}
    )
  endif ()
endif ()

if (NOT VTK_ABI_NAMESPACE_NAME STREQUAL "<DEFAULT>" AND NOT DEFINED ENV{CI})
  message(WARNING "Utilties::Java ABI does not not support the VTK_ABI_NAMESPACE_NAME "
                  "and the symbols will not be mangled.")
endif ()
