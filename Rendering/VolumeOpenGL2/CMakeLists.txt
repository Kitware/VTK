set(classes
  vtkMultiBlockUnstructuredGridVolumeMapper
  vtkMultiBlockVolumeMapper
  vtkOpenGLGPUVolumeRayCastMapper
  vtkOpenGLProjectedTetrahedraMapper
  vtkOpenGLRayCastImageDisplayHelper
  vtkSmartVolumeMapper
  vtkVolumeTexture)

set(nowrap_classes
  vtkOpenGLVolumeGradientOpacityTable
  vtkOpenGLVolumeLookupTable
  vtkOpenGLVolumeMaskGradientOpacityTransferFunction2D
  vtkOpenGLVolumeMaskTransferFunction2D
  vtkOpenGLVolumeOpacityTable
  vtkOpenGLVolumeRGBTable
  vtkOpenGLVolumeTransferFunction2D
  vtkVolumeInputHelper)

set(shader_files
  shaders/raycasterfs.glsl
  shaders/raycastervs.glsl
  shaders/vtkglProjectedTetrahedraFS.glsl
  shaders/vtkglProjectedTetrahedraVS.glsl)

set(nowrap_template_classes
  vtkOpenGLVolumeLookupTables
)

set(sources)
set(private_headers)
foreach (shader_file IN LISTS shader_files)
  vtk_encode_string(
    INPUT         "${shader_file}"
    HEADER_OUTPUT header
    SOURCE_OUTPUT source)
  list(APPEND sources ${source})
  list(APPEND private_headers ${header})
endforeach ()

# Now for the object factory.
set(opengl_overrides
  GPUVolumeRayCastMapper
  ProjectedTetrahedraMapper
  RayCastImageDisplayHelper)

foreach (opengl_override IN LISTS opengl_overrides)
  vtk_object_factory_declare(
    BASE "vtk${opengl_override}"
    OVERRIDE "vtkOpenGL${opengl_override}")
endforeach ()

vtk_object_factory_configure(
  SOURCE_FILE vtk_object_factory_source
  HEADER_FILE vtk_object_factory_header
  EXPORT_MACRO "VTKRENDERINGVOLUMEOPENGL2_EXPORT")

vtk_module_add_module(VTK::RenderingVolumeOpenGL2
  CLASSES ${classes}
  NOWRAP_CLASSES ${nowrap_classes}
  NOWRAP_TEMPLATE_CLASSES  ${nowrap_template_classes}
  SOURCES ${sources} ${vtk_object_factory_source}
  PRIVATE_HEADERS ${private_headers} ${vtk_object_factory_header})
vtk_add_test_mangling(VTK::RenderingVolumeOpenGL2)
