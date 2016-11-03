SET(Module_SRCS
  vtkAbstractParticleWriter.cxx
  vtkArrayReader.cxx
  vtkArrayWriter.cxx
  vtkASCIITextCodec.cxx
  vtkBase64InputStream.cxx
  vtkBase64OutputStream.cxx
  vtkBase64Utilities.cxx
  vtkDataCompressor.cxx
  vtkDelimitedTextWriter.cxx
  vtkGlobFileNames.cxx
  vtkInputStream.cxx
  vtkJavaScriptDataWriter.cxx
  vtkLZ4DataCompressor.cxx
  vtkOutputStream.cxx
  vtkSortFileNames.cxx
  vtkTextCodec.cxx
  vtkTextCodecFactory.cxx
  vtkUTF16TextCodec.cxx
  vtkUTF8TextCodec.cxx
  vtkAbstractPolyDataReader.cxx
  vtkWriter.cxx
  vtkZLibDataCompressor.cxx
  vtkArrayDataReader.cxx
  vtkArrayDataWriter.cxx
  )

include_directories(${VTK_SOURCE_DIR}/ThirdParty/utf8/source)

set_source_files_properties(
  vtkAbstractParticleWriter
  vtkDataCompressor
  vtkTextCodec
  vtkWriter
  vtkAbstractPolyDataReader
  ABSTRACT
  )

vtk_module_library(vtkIOCore ${Module_SRCS})
