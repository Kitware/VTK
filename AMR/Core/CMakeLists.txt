set(Module_SRCS
  vtkAMRBaseParticlesReader.cxx
  vtkAMRBaseReader.cxx
  vtkAMRCutPlane.cxx
  vtkAMRDataSetCache.cxx
  vtkAMREnzoParticlesReader.cxx
  vtkAMREnzoReader.cxx
  vtkAMREnzoReaderInternal.cxx
  vtkAMRFlashParticlesReader.cxx
  vtkAMRFlashReader.cxx
  vtkAMRFlashReaderInternal.cxx
  vtkAMRGaussianPulseSource.cxx
  vtkAMRResampleFilter.cxx
  vtkAMRSliceFilter.cxx
  vtkAMRToMultiBlockFilter.cxx
  vtkAMRUtilities.cxx
  vtkAMRVolumeMapper.cxx
  )

set_source_files_properties(
  vtkAMRBaseReader
  vtkAMRBaseParticlesReader
  ABSTRACT
  )

set_source_files_properties(
  vtkAMREnzoReaderInternal
  vtkAMRFlashReaderInternal
  vtkAMRUtilities
  WRAP_EXCLUDE
  )

vtk_module_library(vtkAMRCore ${Module_SRCS})
