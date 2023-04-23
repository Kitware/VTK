set(classes
  vtkDGOpenGLRenderer
  vtkOpenGLCellGridMapper
  vtkOpenGLCellGridRenderRequest
)

set(sources)
set(private_headers)

set(shader_files
  glsl/vtkCellGridFS_DGHex.glsl
  glsl/vtkCellGridFS_DGTet.glsl
  glsl/vtkCellGridGS_DGHex.glsl
  glsl/vtkCellGridGS_DGTet.glsl
  glsl/vtkCellGridVS.glsl
)
unset(shader_h_files)
foreach(file IN LISTS shader_files)
  vtk_encode_string(
    INPUT         "${file}"
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
