set(classes
  vtkMultiBlockVolumeMapper
  vtkOpenGLGPUVolumeRayCastMapper
  vtkOpenGLProjectedTetrahedraMapper
  vtkOpenGLRayCastImageDisplayHelper
  vtkOpenGLVolumeGradientOpacityTable
  vtkOpenGLVolumeLookupTable
  vtkOpenGLVolumeMaskGradientOpacityTransferFunction2D
  vtkOpenGLVolumeMaskTransferFunction2D
  vtkOpenGLVolumeOpacityTable
  vtkOpenGLVolumeRGBTable
  vtkOpenGLVolumeTransferFunction2D
  vtkSmartVolumeMapper
  vtkVolumeInputHelper
  vtkVolumeTexture)

set(shader_files
  shaders/raycasterfs.glsl
  shaders/raycastervs.glsl
  shaders/vtkglProjectedTetrahedraVS.glsl
  shaders/vtkglProjectedTetrahedraFS.glsl)

set(template_classes
  vtkOpenGLVolumeLookupTables
)

set(sources)
set(headers)
foreach (shader_file IN LISTS shader_files)
  vtk_encode_string(
    INPUT         "${shader_file}"
    EXPORT_SYMBOL "VTKRENDERINGVOLUMEOPENGL2_EXPORT"
    EXPORT_HEADER "vtkRenderingVolumeOpenGL2Module.h"
    HEADER_OUTPUT header
    SOURCE_OUTPUT source)
  list(APPEND sources ${source})
  list(APPEND header ${header})
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
  TEMPLATE_CLASSES  ${template_classes}
  SOURCES ${sources} ${vtk_object_factory_source}
  PRIVATE_HEADERS ${headers} ${vtk_object_factory_header})
