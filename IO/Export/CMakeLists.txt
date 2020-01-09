set(classes
  vtkExporter
  vtkGLTFExporter
  vtkIVExporter
  vtkJSONDataSetWriter
  vtkJSONRenderWindowExporter
  vtkJSONSceneExporter
  vtkOBJExporter
  vtkOOGLExporter
  vtkPOVExporter
  vtkRIBExporter
  vtkRIBLight
  vtkRIBProperty
  vtkSVGContextDevice2D
  vtkSVGExporter
  vtkSingleVTPExporter
  vtkVRMLExporter
  vtkX3D
  vtkX3DExporter
  vtkX3DExporterFIWriter
  vtkX3DExporterWriter
  vtkX3DExporterXMLWriter)

vtk_module_add_module(VTK::IOExport
  CLASSES ${classes})
