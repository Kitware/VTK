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

set(RTWrapper_sources
    RTWrapper/RTWrapper.cxx)

set(RTWrapper_headers
    RTWrapper/RTWrapper.h)

vtk_object_factory_declare(
  BASE vtkOSPRayVolumeInterface
  OVERRIDE vtkOSPRayVolumeMapper)

vtk_object_factory_configure(
  SOURCE_FILE vtk_object_factory_source
  HEADER_FILE vtk_object_factory_header
  EXPORT_MACRO "VTKRENDERINGOSPRAY_EXPORT")

# OSPRay
option(VTK_ENABLE_OSPRAY "Enable OSPRay rendering backend" ON)
set(OSPRAY_INSTALL_DIR "" CACHE PATH "Install location of OSPRay")
mark_as_advanced(OSPRAY_INSTALL_DIR)
if (VTK_ENABLE_OSPRAY)
  # TODO: Send patches upstream to export targets from ospray.
  find_package(ospray 1.8 REQUIRED HINTS ${OSPRAY_INSTALL_DIR})

  option(VTKOSPRAY_ENABLE_DENOISER "Build OSPRay Renderer using OpenImageDenoise")
  if (VTKOSPRAY_ENABLE_DENOISER)
    find_package(OpenImageDenoise 0.8 REQUIRED)
    add_definitions(-DVTKOSPRAY_ENABLE_DENOISER)
    set(OPENIMAGEDENIOSE OpenImageDenoise)
  endif()
endif()


# VisRTX
option(VTK_ENABLE_VISRTX "Enable VisRTX rendering backend" ON)
set(VISRTX_INSTALL_DIR "" CACHE PATH "Install location of VisRTX")
mark_as_advanced(VISRTX_INSTALL_DIR)
if (VTK_ENABLE_VISRTX)
  find_package(VisRTX CONFIG REQUIRED HINTS ${VISRTX_INSTALL_DIR})
endif()
if (VisRTX_FOUND)
  list(APPEND RTWrapper_sources RTWrapper/VisRTX/VisRTXBackend.cxx)
endif()

if (NOT ospray_FOUND AND NOT VisRTX_FOUND)
  message("Warning: Ray tracing requires OSPRay and/or VisRTX, turn VTK_ENABLE_OSPRAY and/or VTK_ENABLE_VISRTX ON.")
endif()


vtk_module_add_module(VTK::RenderingOSPRay
  CLASSES ${classes}
  SOURCES ${vtk_object_factory_source} ${RTWrapper_sources}
  PRIVATE_HEADERS ${vtk_object_factory_header} ${RTWrapper_headers})


if (ospray_FOUND)
  vtk_module_link(VTK::RenderingOSPRay
    PUBLIC
      ${OSPRAY_LIBRARIES}
    PRIVATE
      ${OPENIMAGEDENIOSE})
  vtk_module_include(VTK::RenderingOSPRay
    PUBLIC
    ${OSPRAY_INCLUDE_DIRS})

  # TODO: FindOSPRay should do this.
  # OSPRay_Core uses MMTime which is in it's own special library.
  if (WIN32)
    vtk_module_link(VTK::RenderingOSPRay
      PRIVATE
        Winmm)
  endif ()
  vtk_module_definitions(VTK::RenderingOSPRay PRIVATE VTK_ENABLE_OSPRAY)
endif()

if (VisRTX_FOUND)
  vtk_module_link(VTK::RenderingOSPRay PUBLIC VisRTX_DynLoad)
  vtk_module_definitions(VTK::RenderingOSPRay PRIVATE VTK_ENABLE_VISRTX)
endif()
