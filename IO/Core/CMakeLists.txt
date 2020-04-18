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
  vtkGlobFileNames
  vtkInputStream
  vtkJavaScriptDataWriter
  vtkLZ4DataCompressor
  vtkLZMADataCompressor
  vtkNumberToString
  vtkOutputStream
  vtkSortFileNames
  vtkTextCodec
  vtkTextCodecFactory
  vtkUTF16TextCodec
  vtkUTF8TextCodec
  vtkWriter
  vtkZLibDataCompressor)

set(headers
  vtkUpdateCellsV8toV9.h)

vtk_module_add_module(VTK::IOCore
  CLASSES ${classes}
  HEADERS ${headers})
