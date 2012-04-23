set(Module_SRCS
  vtkBMPReader.cxx
  vtkBMPWriter.cxx
  vtkDEMReader.cxx
  vtkDICOMImageReader.cxx
  #vtkFFMPEGWriter.cxx
  vtkGaussianCubeReader.cxx
  vtkGESignaReader.cxx
  vtkImageExport.cxx
  vtkImageImport.cxx
  vtkImageImportExecutive.cxx
  vtkImageReader2Collection.cxx
  vtkImageReader2.cxx
  vtkImageReader2Factory.cxx
  vtkImageReader.cxx
  vtkImageWriter.cxx
  vtkJPEGReader.cxx
  vtkJPEGWriter.cxx
  vtkMedicalImageProperties.cxx
  vtkMedicalImageReader2.cxx
  vtkMetaImageReader.cxx
  vtkMetaImageWriter.cxx
  #vtkMPEG2Writer.cxx # Do we want to continue supporting?
  #vtkOggTheoraWriter.cxx
  vtkPNGReader.cxx
  vtkPNGWriter.cxx
  vtkPNMReader.cxx
  vtkPNMWriter.cxx
  vtkPostScriptWriter.cxx
  vtkSLCReader.cxx
  vtkTIFFReader.cxx
  vtkTIFFWriter.cxx
  vtkVolume16Reader.cxx
  vtkVolumeReader.cxx
  )

if(WIN32)
  # Should we force this on Windows?
  list(APPEND Module_SRCS vtkAVIWriter.cxx)
endif()

set_source_files_properties(
  vtkVolumeReader
  ABSTRACT
  )

vtk_module_library(vtkIOImage ${Module_SRCS})

if(WIN32)
  target_link_libraries(vtkIOImage vfw32)
endif()
