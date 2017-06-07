if (VTK_LEGACY_REMOVE)
  message(STATUS
    "vtkFiltersMatlab can not be enabled as it has been "
    "marked for deprecation and legacy code has been disabled in this "
    "build.")
  return()
else()
  message(WARNING
    "vtkFiltersMatlab is deprecated and will be removed in a future VTK version")
endif()

find_package(MatlabMex REQUIRED)
include(UseMatlabMex)

set(Module_SRCS
  vtkMatlabEngineFilter.cxx
  vtkMatlabEngineInterface.cxx
  vtkMatlabMexAdapter.cxx
  )

vtk_module_library(vtkFiltersMatlab ${Module_SRCS})
target_link_libraries(vtkFiltersMatlab LINK_PRIVATE ${MEX_LIBRARIES} ${MATLAB_ENGINE_LIBRARIES})
