set(Module_SRCS
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
  vtkGPUVolumeRayCastMapper.cxx
  vtkOSPRayVolumeInterface.cxx
  vtkProjectedTetrahedraMapper.cxx
  vtkRayCastImageDisplayHelper.cxx
  vtkRecursiveSphereDirectionEncoder.cxx
  vtkSphericalDirectionEncoder.cxx
  vtkUnstructuredGridBunykRayCastFunction.cxx
  vtkUnstructuredGridHomogeneousRayIntegrator.cxx
  vtkUnstructuredGridLinearRayIntegrator.cxx
  vtkUnstructuredGridPartialPreIntegration.cxx
  vtkUnstructuredGridPreIntegration.cxx
  vtkUnstructuredGridVolumeMapper.cxx
  vtkUnstructuredGridVolumeRayCastFunction.cxx
  vtkUnstructuredGridVolumeRayCastIterator.cxx
  vtkUnstructuredGridVolumeRayCastMapper.cxx
  vtkUnstructuredGridVolumeRayIntegrator.cxx
  vtkUnstructuredGridVolumeZSweepMapper.cxx
  vtkVolumeMapper.cxx
  vtkVolumeOutlineSource.cxx
  vtkVolumePicker.cxx
  vtkVolumeRayCastCompositeFunction.cxx
  vtkVolumeRayCastFunction.cxx
  vtkVolumeRayCastIsosurfaceFunction.cxx
  vtkVolumeRayCastMIPFunction.cxx
  vtkVolumeRayCastMapper.cxx
  vtkVolumeRayCastSpaceLeapingImageFilter.cxx
)

set_source_files_properties(
  vtkDirectionEncoder
  vtkEncodedGradientEstimator
  vtkVolumeMapper
  vtkVolumeRayCastFunction
  vtkVolumeTextureMapper
  vtkUnstructuredGridVolumeMapper
  vtkUnstructuredGridVolumeRayCastFunction
  vtkUnstructuredGridVolumeRayCastIterator
  vtkUnstructuredGridVolumeRayIntegrator
  ABSTRACT
  )

if (VTK_LEGACY_REMOVE)
  set_source_files_properties(
    vtkVolumeRayCastCompositeFunction.h
    vtkVolumeRayCastCompositeFunction.cxx
    vtkVolumeRayCastFunction.h
    vtkVolumeRayCastFunction.cxx
    vtkVolumeRayCastIsosurfaceFunction.h
    vtkVolumeRayCastIsosurfaceFunction.cxx
    vtkVolumeRayCastMIPFunction.h
    vtkVolumeRayCastMIPFunction.cxx
    vtkVolumeRayCastMapper.h
    vtkVolumeRayCastMapper.cxx
    vtkVolumeTextureMapper2D.h
    vtkVolumeTextureMapper2D.cxx
    vtkVolumeTextureMapper3D.h
    vtkVolumeTextureMapper3D.cxx
    PROPERTIES
    WRAP_EXCLUDE 1
    WRAP_EXCLUDE_PYTHON 1
    )
endif()

if(VTK_RENDERING_BACKEND STREQUAL "OpenGL2")
  set_source_files_properties(
    vtkGPUVolumeRayCastMapper.cxx APPEND PROPERTIES
    COMPILE_DEFINITIONS VTK_OPENGL2
    )
endif()

# only some volume mappers are supported for the OpenGL2 backend
if(NOT VTK_RENDERING_BACKEND STREQUAL "OpenGL2")
  set(Module_SRCS ${Module_SRCS}
    vtkHAVSVolumeMapper.cxx
    vtkProjectedAAHexahedraMapper.cxx
    vtkVolumeTextureMapper2D.cxx
    vtkVolumeTextureMapper3D.cxx
    vtkVolumeTextureMapper.cxx
    )
endif()


vtk_module_library(vtkRenderingVolume ${Module_SRCS})
