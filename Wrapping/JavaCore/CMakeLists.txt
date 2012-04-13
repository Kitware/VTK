find_package(Java REQUIRED)
find_package(JNI REQUIRED)

include_directories(${JAVA_INCLUDE_PATH} ${JAVA_INCLUDE_PATH2})

set(JavaCore_SRCS
  vtkJavaUtil.cxx
  )

vtk_module_library(vtkWrappingJavaCore ${JavaCore_SRCS})
