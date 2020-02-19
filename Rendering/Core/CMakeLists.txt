# The TDx support has not been moved, this header is here to define nothing,
# and so allow classes in other modules to safely include it and keep going.
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkTDxConfigure.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkTDxConfigure.h")

set(classes
  vtkAbstractMapper
  vtkAbstractMapper3D
  vtkAbstractPicker
  vtkAbstractVolumeMapper
  vtkActor
  vtkActor2D
  vtkActor2DCollection
  vtkActorCollection
  vtkAssembly
  vtkAvatar
  vtkBackgroundColorMonitor
  vtkBillboardTextActor3D
  vtkCIEDE2000
  vtkCamera
  vtkCameraActor
  vtkCameraInterpolator
  vtkCellCenterDepthSort
  vtkColorTransferFunction
  vtkCompositeDataDisplayAttributes
  vtkCompositeDataDisplayAttributesLegacy
  vtkCompositePolyDataMapper
  vtkCoordinate
  vtkCuller
  vtkCullerCollection
  vtkDataSetMapper
  vtkDiscretizableColorTransferFunction
  vtkDistanceToCamera
  vtkFXAAOptions
  vtkFlagpoleLabel
  vtkFollower
  vtkFrameBufferObjectBase
  vtkFrustumCoverageCuller
  vtkGPUInfo
  vtkGPUInfoList
  vtkGenericVertexAttributeMapping
  vtkGlyph3DMapper
  vtkGraphMapper
  vtkGraphToGlyphs
  vtkGraphicsFactory
  vtkHardwareSelector
  vtkHardwareWindow
  vtkHierarchicalPolyDataMapper
  vtkImageActor
  vtkImageMapper
  vtkImageMapper3D
  vtkImageProperty
  vtkImageSlice
  vtkImageSliceMapper
  vtkInteractorEventRecorder
  vtkInteractorObserver
  vtkLabeledContourMapper
  vtkLight
  vtkLightActor
  vtkLightCollection
  vtkLightKit
  vtkLogLookupTable
  vtkLookupTableWithEnabling
  vtkMapArrayValues
  vtkMapper
  vtkMapper2D
  vtkMapperCollection
  vtkObserverMediator
  vtkPointGaussianMapper
  vtkPolyDataMapper
  vtkPolyDataMapper2D
  vtkProp
  vtkProp3D
  vtkProp3DCollection
  vtkProp3DFollower
  vtkPropAssembly
  vtkPropCollection
  vtkProperty
  vtkProperty2D
  vtkRenderPass
  vtkRenderState
  vtkRenderTimerLog
  vtkRenderWindow
  vtkRenderWindowCollection
  vtkRenderWindowInteractor
  vtkRenderWindowInteractor3D
  vtkRenderer
  vtkRendererCollection
  vtkRendererDelegate
  vtkRendererSource
  vtkSelectVisiblePoints
  vtkShaderProperty
  vtkSkybox
  vtkStereoCompositor
  vtkTextActor
  vtkTextActor3D
  vtkTexture
  vtkTexturedActor2D
  vtkTransformCoordinateSystems
  vtkTransformInterpolator
  vtkTupleInterpolator
  vtkUniforms
  vtkViewDependentErrorMetric
  vtkViewport
  vtkVisibilitySort
  vtkVolume
  vtkVolumeCollection
  vtkVolumeProperty
  vtkWindowLevelLookupTable
  vtkWindowToImageFilter

  # Needed by vtkAssembly
  vtkAssemblyNode
  vtkAssemblyPath
  vtkAssemblyPaths

  # Needed by vtkRenderer
  vtkAreaPicker
  vtkPicker

  # Needed by vtkAreaPicker
  vtkAbstractPropPicker
  vtkLODProp3D

  # Needed by vtkRenderWindowInteractor
  vtkPropPicker
  vtkPickingManager

  # Needed by vtkPropPicker
  vtkWorldPointPicker

  # Needed for the InteractorStyleSwitch...
  vtkCellPicker
  vtkPointPicker
  vtkRenderedAreaPicker
  vtkScenePicker

  # Needed by vtkRenderWindowInteractor, for the default interactor.
  vtkInteractorStyle
  vtkInteractorStyle3D
  vtkInteractorStyleSwitchBase

  # vtkTDx stuff dragged in by vtkInteractorStyle
  vtkTDxInteractorStyle
  vtkTDxInteractorStyleCamera
  vtkTDxInteractorStyleSettings

  vtkStringToImage
  vtkTextMapper
  vtkTextProperty
  vtkTextPropertyCollection
  vtkTextRenderer

  # Some of the new rendering code - may move around in the future.
  vtkAbstractInteractionDevice
  vtkAbstractRenderDevice
  vtkRenderWidget)

# needed as we do not have vtkRenderingOpenGLConfigure.h here
set_source_files_properties(
  vtkRenderWindow.cxx  # dummy out painterdevice adapter
  vtkHardwareSelector.cxx # dummy out painter device adapter
  PROPERTIES
    COMPILE_DEFINITIONS VTK_OPENGL2)

set(headers
  vtkGPUInfoListArray.h
  vtkNoise200x200.h
  vtkPythagoreanQuadruples.h
  vtkRayCastStructures.h
  vtkRenderingCoreEnums.h
  "${CMAKE_CURRENT_BINARY_DIR}/vtkTDxConfigure.h"
  vtkTDxMotionEventInfo.h)

vtk_module_add_module(VTK::RenderingCore
  CLASSES ${classes}
  HEADERS ${headers}
  )

if (VTK_DEFAULT_RENDER_WINDOW_OFFSCREEN)
  vtk_module_definitions(VTK::RenderingCore
    PRIVATE
      VTK_DEFAULT_RENDER_WINDOW_OFFSCREEN)
endif()
