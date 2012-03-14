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
  
  ADD_DEFINITIONS("-DMPICH_IGNORE_CXX_SEEK -DASSERT_ON")

ENDIF (VTK_USE_MPI)


## Add HierarchicalBoxPipeline executable
ADD_EXECUTABLE(HierarchicalBoxPipeline HierarchicalBoxPipeline.cxx)
TARGET_LINK_LIBRARIES(HierarchicalBoxPipeline vtkRendering vtkIO)
                      
             
## Add AMR examples 
IF (VTK_USE_PARALLEL AND VTK_USE_MPI )
  ADD_EXECUTABLE( Generate3DAMRDataSetWithPulse Generate3DAMRDataSetWithPulse.cxx)
  TARGET_LINK_LIBRARIES( Generate3DAMRDataSetWithPulse
                         vtkGraphics
                         vtkFiltering
                         vtkRendering
                         vtkIO
                         vtkAMR
                         vtkParallel
                         ${MPI_LIBRARIES} 
                         ${VTK_HDF5_LIBRARIES}
                         )
                                                
  ADD_EXECUTABLE( Generate2DAMRDataSetWithPulse Generate2DAMRDataSetWithPulse.cxx)
  TARGET_LINK_LIBRARIES( Generate2DAMRDataSetWithPulse
                         vtkGraphics
                         vtkFiltering
                         vtkRendering
                         vtkIO
                         vtkAMR
                         vtkParallel
                         ${MPI_LIBRARIES} 
                         ${VTK_HDF5_LIBRARIES}
                         )
ENDIF (VTK_USE_PARALLEL AND VTK_USE_MPI)