cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)
if(POLICY CMP0025)
  cmake_policy(SET CMP0025 NEW) # CMake 3.0
endif()
if(POLICY CMP0053)
  cmake_policy(SET CMP0053 NEW) # CMake 3.1
endif()

PROJECT (AMR)

if(NOT VTK_SOURCE_DIR)
  message(ERROR " Cannot build AMR examples without VTK_SOURCE_DIR")
endif()

find_package(VTK COMPONENTS
  vtkCommonExecutionModel
  vtkFiltersCore
  vtkFiltersGeneral
  vtkFiltersSMP
  vtkImagingCore
)
include(${VTK_USE_FILE})

if (VTK_SMP_IMPLEMENTATION_TYPE STREQUAL "TBB")
  add_executable(ParallelContourTree vtkSMPContourFilterManyPieces vtkSMPMinMaxTree vtkAMRDataInternals2 vtkOverlappingAMR2 vtkUniformGridAMR2 vtkUniformGridAMRDataIterator2 vtkStreamedCompositeDataPipeline vtkStreamedCompositeSources main.cxx)
  target_link_libraries(ParallelContourTree ${VTK_LIBRARIES})
endif()
