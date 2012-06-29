find_package(CUDA REQUIRED)
find_path(PISTON_INCLUDE
  PistonCodingStyle.xml
  "/Source/PISTON/github/PISTON"
  DOC "PISTON library's source directory")

set(Module_SRCS
  vtkDataSetToPiston.cxx
  vtkPistonAlgorithm.cxx
  vtkPistonDataObject.cxx
  vtkPistonContour.cxx
  vtkPistonMapper.cxx
  vtkPistonToDataSet.cxx
  vtkPistonScalarsColors.cxx
  vtkPistonSlice.cxx
  vtkPistonSort.cxx
  vtkPistonThreshold.cxx
  )

set( Module_CUDA_SRCS
  vtkPistonConverters.cu
  vtkPistonContour.cu
  vtkPistonMapper.cu
  vtkPistonSlice.cu
  vtkPistonSort.cu
  vtkPistonThreshold.cu
)

if(BUILD_SHARED_LIBS)
  set(GPGPU_BUILD_TYPE SHARED)
endif()
set(CUDA_ATTACH_VS_BUILD_RULE_TO_CUDA_FILE OFF) #otherwise C_SOURCES is empty in VS
vtk_module_impl() #chicken and egg, I need list of include dirs of the module for nvcc's use before vtk_module_library is called
include_directories(${${vtk-module}_DEPENDS_INCLUDE_DIRS} ${PISTON_INCLUDE} ${CUDA_INCLUDE_DIR})
cuda_compile(Module_CUDA_C_SRCS ${Module_CUDA_SRCS} ${GPGPU_BUILD_TYPE})

set_source_files_properties(
  ${Module_CUDA_C_SRCS}
  WRAP_EXCLUDE
  )

list(APPEND Module_SRCS ${Module_CUDA_C_SRCS})

vtk_module_library(vtkAcceleratorsPiston ${Module_SRCS})
target_link_libraries(vtkAcceleratorsPiston ${CUDA_LIBRARIES})
if(APPLE)
  target_link_libraries(vtkAcceleratorsPiston /usr/local/cuda/lib/libtlshook.dylib)
endif()
