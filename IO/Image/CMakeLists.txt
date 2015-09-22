set(Module_SRCS
  vtkBMPReader.cxx
  vtkBMPWriter.cxx
  vtkDEMReader.cxx
  vtkDICOMImageReader.cxx
  vtkGESignaReader.cxx
  vtkImageExport.cxx
  vtkImageImport.cxx
  vtkImageImportExecutive.cxx
  vtkImageReader.cxx
  vtkImageReader2.cxx
  vtkImageReader2Collection.cxx
  vtkImageReader2Factory.cxx
  vtkImageWriter.cxx
  vtkJPEGReader.cxx
  vtkJPEGWriter.cxx
  vtkJSONImageWriter.cxx
  vtkMedicalImageProperties.cxx
  vtkMedicalImageReader2.cxx
  vtkMetaImageReader.cxx
  vtkMetaImageWriter.cxx
  vtkNIFTIImageHeader.cxx
  vtkNIFTIImageReader.cxx
  vtkNIFTIImageWriter.cxx
  vtkNrrdReader.cxx
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
  vtkMRCReader.cxx
  )

set_source_files_properties(
  vtkVolumeReader
  ABSTRACT
  )

set_source_files_properties(
  vtkNIFTIPrivate.h
  PROPERTIES
    WRAP_EXCLUDE 1
    WRAP_EXCLUDE_PYTHON 1
  )

vtk_module_library(vtkIOImage ${Module_SRCS})
