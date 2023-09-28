set(classes
  vtkDGRenderResponder
  vtkOpenGLCellGridMapper
  vtkRenderingCellGrid
)

set(sources)
set(private_headers)

set(shader_files
  glsl/vtkCellGridShaderBases.glsl
  glsl/vtkCellGridShaderCommonDefs.glsl
  glsl/vtkCellGridShaderFragment.glsl
  glsl/vtkCellGridShaderUtil.glsl
  glsl/vtkCellGridShaderVertex.glsl
)
unset(shader_h_files)
foreach(file IN LISTS shader_files)
  vtk_encode_string(
    INPUT         "${file}"
    EXPORT_SYMBOL "VTKRENDERINGCELLGRID_EXPORT"
    EXPORT_HEADER "vtkRenderingCellGridModule.h"
    HEADER_OUTPUT header
    SOURCE_OUTPUT source
  )
  list(APPEND sources "${source}")
  list(APPEND private_headers "${header}")
endforeach()

# setup factory overrides
set(opengl_overrides
  CellGridMapper
)

foreach (opengl_override IN LISTS opengl_overrides)
  vtk_object_factory_declare(
    BASE "vtk${opengl_override}"
    OVERRIDE "vtkOpenGL${opengl_override}")
endforeach ()

vtk_object_factory_configure(
  SOURCE_FILE vtk_object_factory_source
  HEADER_FILE vtk_object_factory_header
  EXPORT_MACRO "VTKRENDERINGCELLGRID_EXPORT")

vtk_module_add_module(VTK::RenderingCellGrid
  CLASSES ${classes}
  SOURCES ${sources} ${vtk_object_factory_source}
  HEADERS ${headers}
  NOWRAP_HEADERS ${nowrap_headers}
  PRIVATE_HEADERS ${private_headers} ${vtk_object_factory_header}
)
vtk_add_test_mangling(VTK::RenderingCellGrid)
