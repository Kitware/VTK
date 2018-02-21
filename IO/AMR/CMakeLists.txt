set(Module_SRCS
  vtkAMRBaseParticlesReader.cxx
  vtkAMRBaseReader.cxx
  vtkAMRDataSetCache.cxx
  vtkAMREnzoParticlesReader.cxx
  vtkAMREnzoReader.cxx
  vtkAMREnzoReaderInternal.cxx
  vtkAMReXParticlesReader.cxx
  vtkAMRFlashParticlesReader.cxx
  vtkAMRFlashReader.cxx
  vtkAMRFlashReaderInternal.cxx
  )

vtk_module_library(vtkIOAMR ${Module_SRCS})
