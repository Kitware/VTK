include(vtkObjectFactory)

set(Module_SRCS
  vtkMultiBlockVolumeMapper.cxx
  vtkOpenGLGPUVolumeRayCastMapper.cxx
  vtkOpenGLProjectedTetrahedraMapper.cxx
  vtkOpenGLRayCastImageDisplayHelper.cxx
  vtkSmartVolumeMapper.cxx
  vtkVolumeTexture.cxx
  vtkVolumeInputHelper.cxx
  vtkOpenGLVolumeRGBTable.cxx
  vtkOpenGLVolumeOpacityTable.cxx
  vtkOpenGLVolumeGradientOpacityTable.cxx
  vtkOpenGLTransferFunction2D.cxx
  )

set(shader_files
  shaders/raycasterfs.glsl
  shaders/raycastervs.glsl
  shaders/vtkglProjectedTetrahedraVS.glsl
  shaders/vtkglProjectedTetrahedraFS.glsl
  )

unset(shader_h_files)
foreach(file ${shader_files})
  vtk_encode_string(
    INPUT         "${file}"
    EXPORT_SYMBOL "VTKRENDERINGVOLUMEOPENGL2_EXPORT"
    EXPORT_HEADER "vtkRenderingVolumeOpenGL2Module.h"
    HEADER_OUTPUT header
    SOURCE_OUTPUT source)
  list(APPEND Module_SRCS ${source})
endforeach()

# Now for the object factory.
set(opengl_overrides
  GPUVolumeRayCastMapper
  ProjectedTetrahedraMapper
  RayCastImageDisplayHelper
  )

foreach(_override ${opengl_overrides})
  vtk_add_override(vtk${_override} vtkOpenGL${_override})
endforeach()
vtk_object_factory_configure("${vtk_module_overrides}")

list(APPEND Module_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx)

vtk_module_library(vtkRenderingVolumeOpenGL2 ${Module_SRCS})
include(vtkOpenGL)
vtk_opengl_link(vtkRenderingVolumeOpenGL2)
