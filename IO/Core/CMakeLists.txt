set(classes
  vtkAbstractParticleWriter
  vtkAbstractPolyDataReader
  vtkArrayDataReader
  vtkArrayDataWriter
  vtkArrayReader
  vtkArrayWriter
  vtkASCIITextCodec
  vtkBase64InputStream
  vtkBase64OutputStream
  vtkBase64Utilities
  vtkDataCompressor
  vtkDelimitedTextWriter
  vtkFileResourceStream
  vtkGlobFileNames
  vtkInputStream
  vtkJavaScriptDataWriter
  vtkLZ4DataCompressor
  vtkLZMADataCompressor
  vtkMemoryResourceStream
  vtkOutputStream
  vtkResourceParser
  vtkResourceStream
  vtkSortFileNames
  vtkTextCodec
  vtkTextCodecFactory
  vtkURI
  vtkURILoader
  vtkUTF16TextCodec
  vtkUTF8TextCodec
  vtkWriter
  vtkZLibDataCompressor)

set(headers
  vtkUpdateCellsV8toV9.h)

vtk_module_add_module(VTK::IOCore
  CLASSES ${classes}
  HEADERS ${headers})
vtk_add_test_mangling(VTK::IOCore)

set_source_files_properties(vtkResourceParser.cxx
  PROPERTIES WRAP_EXCLUDE ON)
