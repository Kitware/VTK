SET(KIT VolumeRendering)
SET(UKIT VOLUMERENDERING)

SET(KIT_TCL_LIBS vtkRenderingTCL vtkIOTCL)
SET(KIT_PYTHON_LIBS vtkRenderingPythonD vtkIOPythonD)
SET(KIT_JAVA_LIBS vtkRenderingJava vtkIOJava)
SET(KIT_INTERFACE_LIBRARIES vtkRendering vtkIO)
SET(KIT_LIBS ${OPENGL_gl_LIBRARY})
SET(Kit_EXTRA_SRCS)
SET(Kit_EXTRA_CMDS)
SET(Kit_TCL_EXTRA_SRCS)
SET(Kit_PYTHON_EXTRA_SRCS)
SET(Kit_JAVA_EXTRA_SRCS)
SET(KIT_TCL_DEPS)
SET(KIT_PYTHON_DEPS)
SET(KIT_JAVA_DEPS)

SET ( Kit_SRCS
vtkDirectionEncoder.cxx
vtkEncodedGradientEstimator.cxx
vtkEncodedGradientShader.cxx
vtkFiniteDifferenceGradientEstimator.cxx
vtkFixedPointRayCastImage.cxx
vtkFixedPointVolumeRayCastCompositeGOHelper.cxx
vtkFixedPointVolumeRayCastCompositeGOShadeHelper.cxx
vtkFixedPointVolumeRayCastCompositeHelper.cxx
vtkFixedPointVolumeRayCastCompositeShadeHelper.cxx
vtkFixedPointVolumeRayCastHelper.cxx
vtkFixedPointVolumeRayCastMIPHelper.cxx
vtkFixedPointVolumeRayCastMapper.cxx
vtkVolumeRayCastSpaceLeapingImageFilter.cxx
vtkGPUVolumeRayCastMapper.cxx
vtkHAVSVolumeMapper.cxx
vtkProjectedAAHexahedraMapper.cxx
vtkProjectedTetrahedraMapper.cxx
vtkRayCastImageDisplayHelper.cxx
vtkRecursiveSphereDirectionEncoder.cxx
vtkSmartVolumeMapper.cxx
vtkSphericalDirectionEncoder.cxx
vtkVolumeMapper.cxx
vtkVolumeOutlineSource.cxx
vtkVolumePicker.cxx
vtkVolumeProMapper.cxx
vtkVolumeRayCastCompositeFunction.cxx
vtkVolumeRayCastFunction.cxx
vtkVolumeRayCastIsosurfaceFunction.cxx
vtkVolumeRayCastMIPFunction.cxx
vtkVolumeRayCastMapper.cxx
vtkVolumeRenderingFactory.cxx
vtkVolumeTextureMapper.cxx
vtkVolumeTextureMapper2D.cxx
vtkVolumeTextureMapper3D.cxx
vtkUnstructuredGridBunykRayCastFunction.cxx
vtkUnstructuredGridHomogeneousRayIntegrator.cxx
vtkUnstructuredGridLinearRayIntegrator.cxx
vtkUnstructuredGridPartialPreIntegration.cxx
vtkUnstructuredGridPreIntegration.cxx
vtkUnstructuredGridVolumeMapper.cxx
vtkUnstructuredGridVolumeRayCastFunction.cxx
vtkUnstructuredGridVolumeRayCastIterator.cxx
vtkUnstructuredGridVolumeRayIntegrator.cxx
vtkUnstructuredGridVolumeRayCastMapper.cxx
vtkUnstructuredGridVolumeZSweepMapper.cxx
)

SET( KitOpenGL_SRCS
  vtkOpenGLGPUVolumeRayCastMapper.cxx
  vtkOpenGLHAVSVolumeMapper.cxx   
  vtkOpenGLProjectedAAHexahedraMapper.cxx
  vtkOpenGLProjectedTetrahedraMapper.cxx
  vtkOpenGLRayCastImageDisplayHelper.cxx
  vtkOpenGLVolumeTextureMapper2D.cxx
  vtkOpenGLVolumeTextureMapper3D.cxx
)

# -----------------------------------------------------------------------------
# Set of GPU shader source files
# -----------------------------------------------------------------------------
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

# -----------------------------------------------------------------------------
# Create custom commands to encode each glsl file into a C string literal
# in a header file
# -----------------------------------------------------------------------------

set(shader_h_files)
foreach(file ${shader_files})
  GET_FILENAME_COMPONENT(file_we ${file} NAME_WE)
  set(src ${VTK_SOURCE_DIR}/VolumeRendering/${file})
  set(res ${VTK_BINARY_DIR}/VolumeRendering/${file_we}.cxx)
  set(resh ${VTK_BINARY_DIR}/VolumeRendering/${file_we}.h)
  add_custom_command(
    OUTPUT ${res} ${resh}
    DEPENDS ${src} vtkEncodeString
    COMMAND ${VTK_ENCODESTRING_EXE}
    ARGS ${res} ${src} ${file_we} --build-header VTK_VOLUMERENDERING_EXPORT vtkSystemIncludes.h
    )
  set(Kit_EXTRA_SRCS ${Kit_EXTRA_SRCS} ${res})
  set(shader_h_files ${shader_h_files} ${resh})
endforeach(file)

# needed by some external projects

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/vtkVolumeRenderingToolkit.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/vtkVolumeRenderingToolkit.h )

IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  INSTALL(FILES
    ${shader_h_files}
    ${CMAKE_CURRENT_BINARY_DIR}/vtkVolumeRenderingToolkit.h
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR_CM24}
    COMPONENT Development)
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)

SET_SOURCE_FILES_PROPERTIES(
vtkDirectionEncoder
vtkEncodedGradientEstimator
vtkFixedPointVolumeRayCastHelper
vtkRayCastImageDisplayHelper
vtkVolumeMapper
vtkVolumeRayCastFunction
vtkVolumeTextureMapper
vtkUnstructuredGridVolumeMapper
vtkUnstructuredGridVolumeRayCastFunction
vtkUnstructuredGridVolumeRayCastIterator
vtkUnstructuredGridVolumeRayIntegrator
ABSTRACT
)

IF (VTK_USE_MANGLED_MESA)
  IF (MANGLED_MESA_LIBRARY)
    SET ( KitOpenGL_SRCS ${KitOpenGL_SRCS}
                 vtkMesaProjectedTetrahedraMapper.cxx
                 vtkMesaRayCastImageDisplayHelper.cxx
                 vtkMesaVolumeTextureMapper2D.cxx
    )
    SET (KIT_LIBS ${KIT_LIBS} ${MANGLED_MESA_LIBRARY})
    IF(MANGLED_OSMESA_LIBRARY)
      SET( KIT_LIBS ${KIT_LIBS} ${MANGLED_OSMESA_LIBRARY} )
    ENDIF(MANGLED_OSMESA_LIBRARY)
  ENDIF (MANGLED_MESA_LIBRARY)
ELSE (VTK_USE_MANGLED_MESA)
  IF(VTK_OPENGL_HAS_OSMESA)
    IF (OSMESA_LIBRARY)
      SET(KIT_LIBS ${KIT_LIBS} ${OSMESA_LIBRARY})
    ENDIF (OSMESA_LIBRARY)
  ENDIF(VTK_OPENGL_HAS_OSMESA)
ENDIF(VTK_USE_MANGLED_MESA)

INCLUDE(${VTK_SOURCE_DIR}/CMake/FindVLI.cmake)

IF (VLI_LIBRARY_FOR_VP1000 AND VLI_INCLUDE_PATH_FOR_VP1000)
  OPTION (VTK_USE_VOLUMEPRO_1000 "Enable support for VolumePro 1000." ON)
ELSE (VLI_LIBRARY_FOR_VP1000 AND VLI_INCLUDE_PATH_FOR_VP1000)
  OPTION (VTK_USE_VOLUMEPRO_1000 "Enable support for VolumePro 1000." OFF)
ENDIF (VLI_LIBRARY_FOR_VP1000 AND VLI_INCLUDE_PATH_FOR_VP1000)

IF (VTK_USE_VOLUMEPRO_1000)
  SET (Kit_SRCS ${Kit_SRCS}
    vtkVolumeProVP1000Mapper.cxx
    )
  SET (KitOpenGL_SRCS ${KitOpenGL_SRCS}
    vtkOpenGLVolumeProVP1000Mapper.cxx
    )
  INCLUDE_DIRECTORIES (${VLI_INCLUDE_PATH_FOR_VP1000})
  SET (KIT_LIBS ${KIT_LIBS} ${VLI_LIBRARY_FOR_VP1000})
ENDIF (VTK_USE_VOLUMEPRO_1000)

MARK_AS_ADVANCED (
  VTK_USE_VOLUMEPRO_1000
  VLI_LIBRARY_FOR_VP1000
  VLI_INCLUDE_PATH_FOR_VP1000
  )

SET(Kit_SRCS ${Kit_SRCS} ${KitOpenGL_SRCS})

#-----------------------------------------------------------------------------
# Include CMake code common to all kits.
INCLUDE(${VTK_CMAKE_DIR}/KitCommonBlock.cmake)
#-----------------------------------------------------------------------------

# Make sure vtkRendering is built before vtkVolumeRendering builds.
# This will make sure vtkgl.h exists.
ADD_DEPENDENCIES(vtkVolumeRendering vtkRendering)

# when crosscompiling vtkEncodeString is imported, don't add a dependency to it
# this should be handled by cmake so it doesn't have to be done here
IF(NOT CMAKE_CROSSCOMPILING)
  # Add target dependency to ensure the vtkEncodeString is built before
  # VolumeRendering is.
  ADD_DEPENDENCIES(vtk${KIT} vtkEncodeString)
ENDIF(NOT CMAKE_CROSSCOMPILING)
