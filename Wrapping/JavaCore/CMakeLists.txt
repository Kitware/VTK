
# what does this do!
SET(__inst_files ${__inst_files}
  vtkJavaUtil.h
  )

set(JavaCore_SRCS
  vtkJavaUtil.cxx
  )

vtk_module_library(vtkWrappingJavaCore ${JavaCore_SRCS})
