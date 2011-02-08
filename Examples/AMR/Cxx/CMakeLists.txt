PROJECT (AMR)

IF(NOT VTK_BINARY_DIR)
FIND_PACKAGE(VTK REQUIRED)
IF(NOT VTK_USE_RENDERING)
  MESSAGE(FATAL_ERROR "Example ${PROJECT_NAME} requires VTK_USE_RENDERING.")
ENDIF(NOT VTK_USE_RENDERING)
INCLUDE(${VTK_USE_FILE})
ENDIF(NOT VTK_BINARY_DIR)

IF (VTK_USE_MPI)
  INCLUDE_DIRECTORIES("${MPI_INCLUDE_PATH} ${VTK_USE_FILE}")
  IF (MPI_LIBRARY) 
    SET(MPI_LIBRARIES ${MPI_LIBRARY})   
  ENDIF (MPI_LIBRARY)   
  IF (MPI_EXTRA_LIBRARY)   
    SET(MPI_LIBRARIES ${MPI_LIBRARIES} "${MPI_EXTRA_LIBRARY}")   
  ENDIF (MPI_EXTRA_LIBRARY) 
  
  # Needed for mpich 2
  ADD_DEFINITIONS("-DMPICH_IGNORE_CXX_SEEK")

ENDIF (VTK_USE_MPI)


## Add HierarchicalBoxPipeline executable
ADD_EXECUTABLE(HierarchicalBoxPipeline HierarchicalBoxPipeline.cxx)
TARGET_LINK_LIBRARIES(HierarchicalBoxPipeline vtkRendering vtkIO)

## Add TestAMRBoxGrhodCellExtrusion executable
ADD_EXECUTABLE(TestAMRBoxGhostCellExtrusion TestAMRBoxGhostCellExtrusion.cxx)
TARGET_LINK_LIBRARIES(TestAMRBoxGhostCellExtrusion vtkFiltering)

## Add AMRDataTransferPipeline executable
ADD_EXECUTABLE(AMRDataTransferPipeline AMRDataTransferPipeline.cxx)
TARGET_LINK_LIBRARIES( AMRDataTransferPipeline 
             vtkGraphics
             vtkFiltering
             vtkRendering 
             vtkIO 
             vtkAMR 
             vtkParallel
             ${MPI_LIBRARIES} 
             )
