set(classes
  vtkDirectionEncoder
  vtkEncodedGradientEstimator
  vtkEncodedGradientShader
  vtkFiniteDifferenceGradientEstimator
  vtkFixedPointRayCastImage
  vtkFixedPointVolumeRayCastCompositeGOHelper
  vtkFixedPointVolumeRayCastCompositeGOShadeHelper
  vtkFixedPointVolumeRayCastCompositeHelper
  vtkFixedPointVolumeRayCastCompositeShadeHelper
  vtkFixedPointVolumeRayCastHelper
  vtkFixedPointVolumeRayCastMapper
  vtkFixedPointVolumeRayCastMIPHelper
  vtkGPUVolumeRayCastMapper
  vtkMultiVolume
  vtkOSPRayVolumeInterface
  vtkAnariVolumeInterface
  vtkProjectedTetrahedraMapper
  vtkRayCastImageDisplayHelper
  vtkRecursiveSphereDirectionEncoder
  vtkSphericalDirectionEncoder
  vtkUnstructuredGridBunykRayCastFunction
  vtkUnstructuredGridHomogeneousRayIntegrator
  vtkUnstructuredGridLinearRayIntegrator
  vtkUnstructuredGridPartialPreIntegration
  vtkUnstructuredGridPreIntegration
  vtkUnstructuredGridVolumeMapper
  vtkUnstructuredGridVolumeRayCastFunction
  vtkUnstructuredGridVolumeRayCastIterator
  vtkUnstructuredGridVolumeRayCastMapper
  vtkUnstructuredGridVolumeRayIntegrator
  vtkUnstructuredGridVolumeZSweepMapper
  vtkVolumeMapper
  vtkVolumeOutlineSource
  vtkVolumePicker
  vtkVolumeRayCastSpaceLeapingImageFilter)

set(headers
  vtkBlockSortHelper.h)

set(serialization_helper_sources)
if (VTK_WRAP_SERIALIZATION)
  list(APPEND serialization_helper_sources
    vtkMultiVolumeSerDesHelper.cxx)
endif ()

vtk_module_add_module(VTK::RenderingVolume
  CLASSES ${classes}
  HEADERS ${headers}
  SOURCES ${serialization_helper_sources})
vtk_add_test_mangling(VTK::RenderingVolume)
