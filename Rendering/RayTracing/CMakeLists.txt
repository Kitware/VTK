set(classes
  vtkOSPRayActorNode
  vtkOSPRayAMRVolumeMapperNode
  vtkOSPRayCameraNode
  vtkOSPRayCompositePolyDataMapperNode
  vtkOSPRayCompositePolyDataMapper2Node
  vtkOSPRayLightNode
  vtkOSPRayMaterialHelpers
  vtkOSPRayMaterialLibrary
  vtkOSPRayMoleculeMapperNode
  vtkOSPRayPass
  vtkOSPRayPolyDataMapperNode
  vtkOSPRayPointGaussianMapperNode
  vtkOSPRayRendererNode
  vtkOSPRayUnstructuredVolumeMapperNode
  vtkOSPRayViewNodeFactory
  vtkOSPRayVolumeMapper
  vtkOSPRayVolumeMapperNode
  vtkOSPRayVolumeNode
  vtkOSPRayWindowNode)

set(nowrap_classes
  RTWrapper/RTWrapper)

set(nowrap_headers
  RTWrapper/RTWrapper.h
  RTWrapper/Types.h
  RTWrapper/Backend.h
  vtkOSPRayCache.h)

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

option(VTK_ENABLE_VISRTX "Enable VisRTX RayTracing backend" OFF)
if (VTK_ENABLE_VISRTX)
  list(APPEND sources RTWrapper/VisRTX/VisRTXBackend.cxx)
endif ()

vtk_module_add_module(VTK::RenderingRayTracing
  HEADER_DIRECTORIES
  CLASSES ${classes}
  SOURCES ${vtk_object_factory_source} ${sources}
  NOWRAP_CLASSES  ${nowrap_classes}
  NOWRAP_HEADERS  ${nowrap_headers}
  PRIVATE_HEADERS ${vtk_object_factory_header})

if (VTK_ENABLE_OSPRAY)
  vtk_module_find_package(PRIVATE_IF_SHARED
    PACKAGE ospray
    VERSION 2.1)

  vtk_module_link(VTK::RenderingRayTracing
    NO_KIT_EXPORT_IF_SHARED
    PRIVATE
      ospray::ospray)
  if (VTKOSPRAY_ENABLE_DENOISER)
    vtk_module_find_package(PRIVATE_IF_SHARED
      PACKAGE OpenImageDenoise
      FORWARD_VERSION_REQ MINOR)
    vtk_module_definitions(VTK::RenderingRayTracing
      PRIVATE
        VTKOSPRAY_ENABLE_DENOISER)
    vtk_module_link(VTK::RenderingRayTracing
      NO_KIT_EXPORT_IF_SHARED
      PRIVATE
        OpenImageDenoise)
  endif ()

  # TODO: FindOSPRay should do this.
  # OSPRay_Core uses MMTime which is in it's own special library.
  if (WIN32)
    vtk_module_link(VTK::RenderingRayTracing
      PRIVATE
        Winmm)
  endif ()
  vtk_module_definitions(VTK::RenderingRayTracing
    PRIVATE
      VTK_ENABLE_OSPRAY
      OSPRAY_VERSION_MINOR=${ospray_VERSION_MINOR})
endif ()

# VisRTX
if (VTK_ENABLE_VISRTX)
  vtk_module_find_package(
    PACKAGE VisRTX
    CONFIG_MODE)
  if (DEFINED VisRTX_VERSION AND
      VisRTX_VERSION VERSION_GREATER_EQUAL "0.2.0")
    message(FATAL_ERROR
      "VisRTX 0.2.0 and above are intended to be used via the ANARI interface. "
      "See https://gitlab.kitware.com/vtk/vtk/-/merge_requests/10324 for progress.")
  endif ()
  vtk_module_link(VTK::RenderingRayTracing
    PUBLIC
      VisRTX_DynLoad)
  vtk_module_definitions(VTK::RenderingRayTracing
    PRIVATE
      VTK_ENABLE_VISRTX)
endif ()
vtk_add_test_mangling(VTK::RenderingRayTracing)
