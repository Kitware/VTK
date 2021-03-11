set(classes
  vtkAMRBaseParticlesReader
  vtkAMRBaseReader
  vtkAMRDataSetCache
  vtkAMREnzoParticlesReader
  vtkAMREnzoReader
  vtkAMReXGridReader
  vtkAMReXParticlesReader
  vtkAMRFlashParticlesReader
  vtkAMRFlashReader
  vtkAMRVelodyneReader)

set(private_classes
  vtkAMREnzoReaderInternal
  vtkAMReXGridReaderInternal
  vtkAMRFlashReaderInternal
  vtkAMRVelodyneReaderInternal)

vtk_module_add_module(VTK::IOAMR
  CLASSES ${classes}
  SOURCES ${sources}
  PRIVATE_CLASSES ${private_classes})
