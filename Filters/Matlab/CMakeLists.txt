find_package(MatlabMex REQUIRED)
include(UseMatlabMex)

set(Module_SRCS
  vtkMatlabEngineFilter.cxx
  vtkMatlabEngineInterface.cxx
  vtkMatlabMexAdapter.cxx
  )

vtk_module_library(vtkFiltersMatlab ${Module_SRCS})
target_link_libraries(vtkFiltersMatlab LINK_PRIVATE ${MEX_LIBRARIES} ${MATLAB_ENGINE_LIBRARIES})
