set(classes
  vtkOSPRayActorNode
  vtkOSPRayAMRVolumeMapperNode
  vtkOSPRayCameraNode
  vtkOSPRayCompositePolyDataMapper2Node
  vtkOSPRayLightNode
  vtkOSPRayMaterialHelpers
  vtkOSPRayMaterialLibrary
  vtkOSPRayPass
  vtkOSPRayPolyDataMapperNode
  vtkOSPRayRendererNode
  vtkOSPRayTetrahedraMapperNode
  vtkOSPRayViewNodeFactory
  vtkOSPRayVolumeMapper
  vtkOSPRayVolumeMapperNode
  vtkOSPRayVolumeNode
  vtkOSPRayWindowNode)

set(sources
    RTWrapper/RTWrapper.cxx)

set(headers
    RTWrapper/RTWrapper.h)

vtk_object_factory_declare(
  BASE vtkOSPRayVolumeInterface
  OVERRIDE vtkOSPRayVolumeMapper)

vtk_object_factory_configure(
  SOURCE_FILE vtk_object_factory_source
  HEADER_FILE vtk_object_factory_header
  EXPORT_MACRO "VTKRENDERINGRAYTRACING_EXPORT")

option(VTK_ENABLE_OSPRAY "Enable OSPRay RayTracing backend" ON)
cmake_dependent_option(VTKOSPRAY_ENABLE_DENOISER
  "build OSPRay Renderer using OpenImageDenoise" OFF
  "VTK_ENABLE_OSPRAY" OFF)

cmake_dependent_option(VTK_ENABLE_VISRTX "Enable VisRTX RayTracing backend" OFF
                       "VTK_USE_CUDA" OFF)
if(VTK_ENABLE_VISRTX)
  list(APPEND sources RTWrapper/VisRTX/VisRTXBackend.cxx)
endif()

vtk_module_add_module(VTK::RenderingRayTracing
  CLASSES ${classes}
  SOURCES ${vtk_object_factory_source} ${sources}
  PRIVATE_HEADERS ${vtk_object_factory_header} ${headers})

if(VTK_ENABLE_OSPRAY)
  # TODO: Send patches upstream to export targets from ospray.
  find_package(ospray 1.8 REQUIRED)

  vtk_module_include(VTK::RenderingRayTracing
    PUBLIC
      ${OSPRAY_INCLUDE_DIRS})
  vtk_module_link(VTK::RenderingRayTracing
    PUBLIC
      ${OSPRAY_LIBRARIES})
  if(VTKOSPRAY_ENABLE_DENOISER)
    find_package(OpenImageDenoise 0.8 REQUIRED)
    add_definitions(-DVTKOSPRAY_ENABLE_DENOISER)
    vtk_module_link(VTK::RenderingRayTracing
      PRIVATE
        OpenImageDenoise)
  endif()

  # TODO: FindOSPRay should do this.
  # OSPRay_Core uses MMTime which is in it's own special library.
  if(WIN32)
    vtk_module_link(VTK::RenderingRayTracing
      PRIVATE
        Winmm)
  endif()
  vtk_module_definitions(VTK::RenderingRayTracing
    PRIVATE
      VTK_ENABLE_OSPRAY)
endif()

# VisRTX
if(VTK_ENABLE_VISRTX)
  find_package(VisRTX CONFIG REQUIRED)
  vtk_module_link(VTK::RenderingRayTracing
    PUBLIC
      VisRTX_DynLoad)
  vtk_module_definitions(VTK::RenderingRayTracing
    PRIVATE
      VTK_ENABLE_VISRTX)
endif()
