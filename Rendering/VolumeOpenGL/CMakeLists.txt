include(vtkObjectFactory)

set(Module_SRCS
  vtkOpenGLHAVSVolumeMapper.cxx
  vtkOpenGLProjectedAAHexahedraMapper.cxx
  vtkOpenGLProjectedTetrahedraMapper.cxx
  vtkOpenGLRayCastImageDisplayHelper.cxx
  vtkOpenGLVolumeTextureMapper2D.cxx
  vtkOpenGLVolumeTextureMapper3D.cxx
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
  get_filename_component(file_we ${file} NAME_WE)
  set(src  ${CMAKE_CURRENT_SOURCE_DIR}/${file})
  set(res  ${CMAKE_CURRENT_BINARY_DIR}/${file_we}.cxx)
  set(resh ${CMAKE_CURRENT_BINARY_DIR}/${file_we}.h)
  list(APPEND shader_h_files ${resh})
  add_custom_command(
    OUTPUT ${res} ${resh}
    DEPENDS ${src} vtkEncodeString
    COMMAND vtkEncodeString
    ARGS ${res} ${src} ${file_we}
      --build-header VTKRENDERINGVOLUMEOPENGL_EXPORT vtkRenderingVolumeOpenGLModule.h
    )
  list(APPEND Module_SRCS ${res})
  set_source_files_properties(${file_we} WRAP_EXCLUDE)
endforeach()

# Nwo for the object factory.
set(opengl_overrides
  HAVSVolumeMapper
  ProjectedAAHexahedraMapper
  ProjectedTetrahedraMapper
  RayCastImageDisplayHelper
  VolumeTextureMapper2D
  VolumeTextureMapper3D
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
set_source_files_properties(${vtk-module}ObjectFactory.cxx WRAP_EXCLUDE)

vtk_module_library(vtkRenderingVolumeOpenGL ${Module_SRCS})
