SET(KIT VolumeRendering)
SET(UKIT VOLUMERENDERING)

SET(KIT_TCL_LIBS vtkRenderingTCL vtkIOTCL)
SET(KIT_PYTHON_LIBS vtkRenderingPythonD vtkIOPythonD)
SET(KIT_JAVA_LIBS vtkRenderingJava vtkIOJava)
SET(KIT_LIBS vtkRendering vtkIO)

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
vtkProjectedTetrahedraMapper.cxx
vtkRayCastImageDisplayHelper.cxx
vtkRecursiveSphereDirectionEncoder.cxx
vtkSphericalDirectionEncoder.cxx
vtkVolumeMapper.cxx
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
  vtkOpenGLRayCastImageDisplayHelper.cxx
  vtkOpenGLVolumeTextureMapper2D.cxx
  vtkOpenGLVolumeTextureMapper3D.cxx
)

#-------------------------------------------------
# Stuff we need for the vtkVolumeTextureMapper3D
#-------------------------------------------------

# CMake 2.0 has a bug in its Makefile generator that prevents more
# than one generated header file in a directory from working properly
# in parallel builds.  Use a hack to work-around the problem.
SET(VTK_VR_NEED_ASM_HACK 0)
IF("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" LESS 2.2)
  IF(CMAKE_GENERATOR MATCHES "^Unix Makefiles$")
    SET(VTK_VR_NEED_ASM_HACK 1)
  ENDIF(CMAKE_GENERATOR MATCHES "^Unix Makefiles$")
ENDIF("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" LESS 2.2)
IF(VTK_VR_NEED_ASM_HACK)
  # Work-around the CMake 2.0 bug by building the generated
  # headers with a try-compile.  Create a small project that just
  # includes the generator and corresponding custom commands.
  FILE(WRITE ${CMAKE_CURRENT_BINARY_DIR}/ASM/CMakeLists.txt
    "PROJECT(VTKVRASM)\n"
    "SET(VTK_SOURCE_DIR \"${VTK_SOURCE_DIR}\")\n"
    "SET(VTK_BINARY_DIR \"${VTK_BINARY_DIR}\")\n"
    "INCLUDE_DIRECTORIES(\"${VTK_BINARY_DIR}\" \"${VTK_SOURCE_DIR}/Common\")\n"
    "INCLUDE(\"${VTK_SOURCE_DIR}/VolumeRendering/vtkVRASM.cmake\")\n"
    )

  # Drive the custom commands with a custom target in the try-compile so
  # that the headers exist at the end of the CMake stage.  There may be
  # some dependency issues but the asm files do not change much.
  TRY_COMPILE(VTK_VR_ASM_COMPILED
    ${CMAKE_CURRENT_BINARY_DIR}/ASM
    ${CMAKE_CURRENT_BINARY_DIR}/ASM
    VTKVRASM vtk_vr_generate_asm_headers
    CMAKE_FLAGS -DVTK_SOURCE_DIR:PATH=${VTK_SOURCE_DIR}
                -DVTK_BINARY_DIR:PATH=${VTK_BINARY_DIR}
    OUTPUT_VARIABLE VTK_VR_ASM_OUTPUT
    )

  # Report the result of generating the headers.
  IF(VTK_VR_ASM_COMPILED)
    MESSAGE(STATUS "VolumeRendering asm header generation succeeded.")
  ELSE(VTK_VR_ASM_COMPILED)
    MESSAGE(FATAL_ERROR "VolumeRendering asm header generation failed: [${VTK_VR_ASM_OUTPUT}]")
  ENDIF(VTK_VR_ASM_COMPILED)
ELSE(VTK_VR_NEED_ASM_HACK)
  # CMake is powerful enough to generate the headers correctly.
  INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/vtkVRASM.cmake)
ENDIF(VTK_VR_NEED_ASM_HACK)

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/vtkVolumeRenderingToolkit.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/vtkVolumeRenderingToolkit.h )
        


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
SET(Kit_SRCS ${Kit_SRCS} ${KitOpenGL_SRCS})

SET(Kit_EXTRA_SRCS)
SET(Kit_EXTRA_CMDS)
SET(Kit_TCL_EXTRA_SRCS)
SET(Kit_PYTHON_EXTRA_SRCS)
SET(Kit_JAVA_EXTRA_SRCS)
SET(KIT_TCL_DEPS)
SET(KIT_PYTHON_DEPS)
SET(KIT_JAVA_DEPS)

#-----------------------------------------------------------------------------
# Include CMake code common to all kits.
INCLUDE(${VTK_SOURCE_DIR}/CMake/KitCommonBlock.cmake)
#-----------------------------------------------------------------------------

# Make sure vtkRendering is built before vtkVolumeRendering builds.
# This will make sure vtkgl.h exists.
ADD_DEPENDENCIES(vtkVolumeRendering vtkRendering)

