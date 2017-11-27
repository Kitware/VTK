include(vtkObjectFactory)

set(Module_SRCS
  vtkOpenGLHAVSVolumeMapper.cxx
  vtkOpenGLProjectedAAHexahedraMapper.cxx
  vtkOpenGLProjectedTetrahedraMapper.cxx
  vtkOpenGLRayCastImageDisplayHelper.cxx
  vtkSmartVolumeMapper.cxx
  )

if (NOT Module_vtkRenderingVolumeOpenGLNew)
  list(APPEND Module_SRCS vtkOpenGLGPUVolumeRayCastMapper.cxx)
endif()

set(shader_files
  vtkGPUVolumeRayCastMapper_AdditiveFS.glsl
  vtkGPUVolumeRayCastMapper_AdditiveCroppingFS.glsl
  vtkGPUVolumeRayCastMapper_AdditiveNoCroppingFS.glsl
  vtkGPUVolumeRayCastMapper_CompositeFS.glsl
  vtkGPUVolumeRayCastMapper_CompositeMaskFS.glsl
  vtkGPUVolumeRayCastMapper_CompositeBinaryMaskFS.glsl
  vtkGPUVolumeRayCastMapper_CompositeCroppingFS.glsl
  vtkGPUVolumeRayCastMapper_CompositeNoCroppingFS.glsl
  vtkGPUVolumeRayCastMapper_HeaderFS.glsl
  vtkGPUVolumeRayCastMapper_MIPCroppingFS.glsl
  vtkGPUVolumeRayCastMapper_MIPFS.glsl
  vtkGPUVolumeRayCastMapper_MIPBinaryMaskFS.glsl
  vtkGPUVolumeRayCastMapper_MIPFourDependentFS.glsl
  vtkGPUVolumeRayCastMapper_MIPFourDependentCroppingFS.glsl
  vtkGPUVolumeRayCastMapper_MIPFourDependentNoCroppingFS.glsl
  vtkGPUVolumeRayCastMapper_MIPNoCroppingFS.glsl
  vtkGPUVolumeRayCastMapper_MinIPCroppingFS.glsl
  vtkGPUVolumeRayCastMapper_MinIPFS.glsl
  vtkGPUVolumeRayCastMapper_MinIPBinaryMaskFS.glsl
  vtkGPUVolumeRayCastMapper_MinIPFourDependentFS.glsl
  vtkGPUVolumeRayCastMapper_MinIPFourDependentCroppingFS.glsl
  vtkGPUVolumeRayCastMapper_MinIPFourDependentNoCroppingFS.glsl
  vtkGPUVolumeRayCastMapper_MinIPNoCroppingFS.glsl
  vtkGPUVolumeRayCastMapper_ParallelProjectionFS.glsl
  vtkGPUVolumeRayCastMapper_PerspectiveProjectionFS.glsl
  vtkGPUVolumeRayCastMapper_ScaleBiasFS.glsl
  vtkGPUVolumeRayCastMapper_OneComponentFS.glsl
  vtkGPUVolumeRayCastMapper_FourComponentsFS.glsl
  vtkGPUVolumeRayCastMapper_NoShadeFS.glsl
  vtkGPUVolumeRayCastMapper_ShadeFS.glsl
  vtkHAVSVolumeMapper_kbufferVP.asm
  vtkHAVSVolumeMapper_k2FP.asm
  vtkHAVSVolumeMapper_k2BeginFP.asm
  vtkHAVSVolumeMapper_k2EndFP.asm
  vtkHAVSVolumeMapper_k6FP.asm
  vtkHAVSVolumeMapper_k6BeginFP.asm
  vtkHAVSVolumeMapper_k6EndFP.asm
  vtkProjectedAAHexahedraMapper_VS.glsl
  vtkProjectedAAHexahedraMapper_GS.glsl
  vtkProjectedAAHexahedraMapper_FS.glsl
  vtkVolumeTextureMapper3D_FourDependentNoShadeFP.asm
  vtkVolumeTextureMapper3D_FourDependentShadeFP.asm
  vtkVolumeTextureMapper3D_OneComponentNoShadeFP.asm
  vtkVolumeTextureMapper3D_OneComponentShadeFP.asm
  vtkVolumeTextureMapper3D_TwoDependentNoShadeFP.asm
  vtkVolumeTextureMapper3D_TwoDependentShadeFP.asm
  )

unset(shader_h_files)
foreach(file ${shader_files})
  vtk_encode_string(
    INPUT         "${file}"
    EXPORT_SYMBOL "VTKRENDERINGVOLUMEOPENGL_EXPORT"
    EXPORT_HEADER "vtkRenderingVolumeOpenGLModule.h"
    HEADER_OUTPUT header
    SOURCE_OUTPUT source)
  list(APPEND Module_SRCS ${source})
  set_source_files_properties(
    ${header}
    ${source}
    PROPERTIES
      WRAP_EXCLUDE 1
      WRAP_EXCLUDE_PYTHON 1
    )
endforeach()

# Nwo for the object factory.
set(opengl_overrides
  HAVSVolumeMapper
  ProjectedAAHexahedraMapper
  ProjectedTetrahedraMapper
  RayCastImageDisplayHelper
  )

if(NOT Module_vtkRenderingVolumeOpenGLNew)
  list(APPEND opengl_overrides GPUVolumeRayCastMapper)
endif()

foreach(_override ${opengl_overrides})
  vtk_add_override(vtk${_override} vtkOpenGL${_override})
endforeach()
vtk_object_factory_configure("${vtk_module_overrides}")

list(APPEND Module_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx)
set_source_files_properties(
  ${vtk-module}ObjectFactory.cxx
  PROPERTIES
    WRAP_EXCLUDE 1
    WRAP_EXCLUDE_PYTHON 1
  )

vtk_module_library(vtkRenderingVolumeOpenGL ${Module_SRCS})
include(vtkOpenGL)
vtk_opengl_link(vtkRenderingVolumeOpenGL)
