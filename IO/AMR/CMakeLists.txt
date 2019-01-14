set(classes
  vtkAMRBaseParticlesReader
  vtkAMRBaseReader
  vtkAMRDataSetCache
  vtkAMREnzoParticlesReader
  vtkAMREnzoReader
  vtkAMRFlashParticlesReader
  vtkAMRFlashReader
  vtkAMReXParticlesReader)

set(sources
  vtkAMREnzoReaderInternal.cxx
  vtkAMRFlashReaderInternal.cxx)

set(private_headers
  vtkAMREnzoReaderInternal.h
  vtkAMRFlashReaderInternal.h)

vtk_module_add_module(VTK::IOAMR
  CLASSES ${classes}
  SOURCES ${sources}
  PRIVATE_HEADERS ${private_headers})
