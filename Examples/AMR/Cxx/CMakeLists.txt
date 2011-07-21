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

## Add TestAMRBoxGrhodCellExtrusion executable
ADD_EXECUTABLE(TestAMRBoxGhostCellExtrusion TestAMRBoxGhostCellExtrusion.cxx)
TARGET_LINK_LIBRARIES(TestAMRBoxGhostCellExtrusion vtkFiltering)

## Add AMRResamplingToUniformGrid executable
ADD_EXECUTABLE(EnzoDumpVariable EnzoDumpVariable.cxx)
TARGET_LINK_LIBRARIES( EnzoDumpVariable 
             vtkGraphics
             vtkFiltering
             vtkRendering 
             vtkIO 
             vtkAMR 
             vtkParallel
             ${MPI_LIBRARIES} 
             ${VTK_HDF5_LIBRARIES}
             )
             
## Add AMRResamplingToUniformGrid executable
ADD_EXECUTABLE(AMRResamplingToUniformGrid AMRResamplingToUniformGrid.cxx)
TARGET_LINK_LIBRARIES( AMRResamplingToUniformGrid 
             vtkGraphics
             vtkFiltering
             vtkRendering 
             vtkIO 
             vtkAMR 
             vtkParallel
             ${MPI_LIBRARIES} 
             ${VTK_HDF5_LIBRARIES}
             )

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
             ${VTK_HDF5_LIBRARIES}
             )

## Add AMRDataTransferPipeline executable
ADD_EXECUTABLE(flash2vth flash2vth.cxx)
TARGET_LINK_LIBRARIES( flash2vth 
             vtkGraphics
             vtkFiltering
             vtkRendering 
             vtkIO 
             vtkAMR 
             vtkParallel
             ${MPI_LIBRARIES} 
             ${VTK_HDF5_LIBRARIES}
             )
             
## Add AMRDataTransferPipeline executable
ADD_EXECUTABLE(enzo2vth enzo2vth.cxx)
TARGET_LINK_LIBRARIES( enzo2vth 
             vtkGraphics
             vtkFiltering
             vtkRendering 
             vtkIO 
             vtkAMR 
             vtkParallel
             ${MPI_LIBRARIES} 
             ${VTK_HDF5_LIBRARIES}
             )
             
## Add AMRDualExtractor executable
ADD_EXECUTABLE( AMRDualExtractor AMRDualExtractor.cxx )
TARGET_LINK_LIBRARIES( AMRDualExtractor 
             vtkGraphics
             vtkFiltering
             vtkRendering 
             vtkIO 
             vtkAMR 
             vtkParallel
             ${MPI_LIBRARIES} 
             ${VTK_HDF5_LIBRARIES}
             )

## Add AMRHomogenization
ADD_EXECUTABLE(AMRHomogenization AMRHomogenization.cxx)
TARGET_LINK_LIBRARIES( AMRHomogenization
                vtkGraphics
                vtkFiltering
                vtkRendering
                vtkIO
                vtkAMR
                vtkParallel
                ${MPI_LIBRARIES}
                ${VTK_HDF5_LIBRARIES}
                )

## Add AMR Slicer 
ADD_EXECUTABLE(AMRSlicer AMRSlicer.cxx)
TARGET_LINK_LIBRARIES( AMRSlicer
                vtkGraphics
                vtkFiltering
                vtkRendering
                vtkIO
                vtkAMR
                vtkParallel
                ${MPI_LIBRARIES}
                ${VTK_HDF5_LIBRARIES}
                )

## Add AMR Ghost Cell Extruder 
ADD_EXECUTABLE(AMRGhostCellExtruder AMRGhostCellExtruder.cxx)
TARGET_LINK_LIBRARIES( AMRGhostCellExtruder
                vtkGraphics
                vtkFiltering
                vtkRendering
                vtkIO
                vtkAMR
                vtkParallel
                ${MPI_LIBRARIES}
                ${VTK_HDF5_LIBRARIES}
                )
                
## Add Generate3DAMRDataSetWithPulse executable
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
                       
## Add Generate3DAMRDataSetWithPulse executable                        
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

## Add TestAMRProbe executable
ADD_EXECUTABLE( TestAMRProbe TestAMRProbe.cxx )
TARGET_LINK_LIBRARIES( TestAMRProbe 
                       vtkGraphics
                       vtkFiltering
                       vtkRendering
                       vtkIO
                       vtkAMR
                       vtkParallel
                       ${MPI_LIBRARIES} 
                       ${VTK_HDF5_LIBRARIES}
                      )
                                   
## Add TestImageDataToStructuredGridFilter
ADD_EXECUTABLE( TestImageDataToStructuredGridFilter 
                TestImageDataToStructuredGridFilter.cxx )
TARGET_LINK_LIBRARIES(TestImageDataToStructuredGridFilter
                      vtkGraphics
                      vtkIO
                      vtkFiltering 
                      )