set(Module_SRCS
  vtkAbstractMapper3D.cxx
  vtkAbstractMapper.cxx
  vtkAbstractPicker.cxx
  vtkAbstractVolumeMapper.cxx
  vtkActor2DCollection.cxx
  vtkActor2D.cxx
  vtkActorCollection.cxx
  vtkActor.cxx
  vtkAssembly.cxx
  vtkBackgroundColorMonitor.cxx
  vtkCameraActor.cxx
  vtkCamera.cxx
  vtkCameraInterpolator.cxx
  vtkCellCenterDepthSort.cxx
  vtkColorTransferFunction.cxx
  vtkCompositeDataDisplayAttributes.cxx
  vtkCompositePolyDataMapper.cxx
  vtkCoordinate.cxx
  vtkCullerCollection.cxx
  vtkCuller.cxx
  vtkDataSetMapper.cxx
  vtkDiscretizableColorTransferFunction.cxx
  vtkDistanceToCamera.cxx
  vtkFollower.cxx
  vtkFrustumCoverageCuller.cxx
  vtkGenericRenderWindowInteractor.cxx
  vtkGenericVertexAttributeMapping.cxx
  vtkGlyph3DMapper.cxx
  vtkGPUInfo.cxx
  vtkGPUInfoList.cxx
  vtkGraphicsFactory.cxx
  vtkGraphMapper.cxx
  vtkGraphToGlyphs.cxx
  vtkHardwareSelector.cxx
  vtkHierarchicalPolyDataMapper.cxx
  vtkImageActor.cxx
  vtkImageMapper3D.cxx
  vtkImageMapper.cxx
  vtkImageProperty.cxx
  vtkImageSlice.cxx
  vtkImageSliceMapper.cxx
  vtkInteractorEventRecorder.cxx
  vtkInteractorObserver.cxx
  vtkLabeledContourMapper.cxx
  vtkLightActor.cxx
  vtkLightCollection.cxx
  vtkLight.cxx
  vtkLightKit.cxx
  vtkLogLookupTable.cxx
  vtkLookupTableWithEnabling.cxx
  vtkMapArrayValues.cxx
  vtkMapper2D.cxx
  vtkMapperCollection.cxx
  vtkMapper.cxx
  vtkObserverMediator.cxx
  vtkPolyDataMapper2D.cxx
  vtkPolyDataMapper.cxx
  vtkProp3DCollection.cxx
  vtkProp3D.cxx
  vtkProp3DFollower.cxx
  vtkPropAssembly.cxx
  vtkPropCollection.cxx
  vtkProp.cxx
  vtkProperty2D.cxx
  vtkProperty.cxx
  vtkRendererCollection.cxx
  vtkRenderer.cxx
  vtkRendererDelegate.cxx
  vtkRendererSource.cxx
  vtkRenderWindowCollection.cxx
  vtkRenderWindow.cxx
  vtkRenderWindowInteractor.cxx
  vtkSelectVisiblePoints.cxx
  vtkShaderDeviceAdapter2.cxx
  vtkTextActor.cxx
  vtkTextActor3D.cxx
  vtkTexture.cxx
  vtkTexturedActor2D.cxx
  vtkTransformCoordinateSystems.cxx
  vtkTransformInterpolator.cxx
  vtkTupleInterpolator.cxx
  vtkViewDependentErrorMetric.cxx
  vtkViewport.cxx
  vtkVisibilitySort.cxx
  vtkVolumeCollection.cxx
  vtkVolume.cxx
  vtkVolumeProperty.cxx
  vtkWindowLevelLookupTable.cxx
  vtkWindowToImageFilter.cxx

  vtkAssemblyNode.cxx # Needed by vtkAssembly
  vtkAssemblyPath.cxx # Needed by vtkAssembly
  vtkAssemblyPaths.cxx # Needed by vtkAssembly

  vtkAreaPicker.cxx # Needed by vtkRenderer
  vtkPicker.cxx # Needed by vtkRenderer
  vtkAbstractPropPicker.cxx # Needed by vtkAreaPicker
  vtkPropPicker.cxx # Needed by vtkRenderWindowInteractor
  vtkPickingManager.cxx # Needed by vtkRenderWindowInteractor
  vtkLODProp3D.cxx # Needed by vtkAreaPicker
  vtkWorldPointPicker.cxx # Needed by vtkPropPicker

  # Needed for the InteractorStyleSwitch...
  vtkCellPicker.cxx
  vtkPointPicker.cxx
  vtkRenderedAreaPicker.cxx
  vtkScenePicker.cxx

  # Needed by vtkRenderWindowInteractor, for the default interactor.
  vtkInteractorStyle.cxx
  vtkInteractorStyleSwitchBase.cxx
  # vtkTDx stuff dragged in by vtkInteractorStyle
  vtkTDxInteractorStyle.cxx
  vtkTDxInteractorStyleCamera.cxx
  vtkTDxInteractorStyleSettings.cxx

  vtkStringToImage.cxx
  vtkTextMapper.cxx
  vtkTextProperty.cxx
  vtkTextPropertyCollection.cxx
  vtkTextRenderer.cxx

  # Some of the new rendering code - may move around in the future.
  vtkAbstractInteractionDevice.cxx
  vtkAbstractRenderDevice.cxx
  vtkRenderWidget.cxx
  )

if(VTK_RENDERING_BACKEND STREQUAL "OpenGL2")
  set_source_files_properties(
    vtkRenderWindow.cxx  # dummy out painterdevice adapter
    vtkHardwareSelector.cxx # dummy out painter device adapter
    PROPERTIES COMPILE_FLAGS -DVTK_OPENGL2)
  list(APPEND Module_SRCS
    vtkPointGaussianMapper.cxx
    )
  set_source_files_properties(
    vtkPointGaussianMapper
    ABSTRACT
    )
else() # Add painter device adapter when not OpenGL2
  list(APPEND Module_SRCS
    vtkPainterDeviceAdapter.cxx
    )
endif()

set(${vtk-module}_HDRS
  vtkGPUInfoListArray.h
  vtkNoise200x200.h
  vtkPythagoreanQuadruples.h
  vtkRayCastStructures.h
  vtkRenderingCoreEnums.h
  vtkTDxMotionEventInfo.h
  )

set_source_files_properties(
  vtkAbstractInteractionDevice
  vtkAbstractMapper3D
  vtkAbstractMapper
  vtkAbstractRenderDevice
  vtkAbstractVolumeMapper
  vtkAbstractPicker
  vtkAbstractPropPicker
  vtkCuller
  vtkGPUInfoList.cxx
  vtkHardwareSelector
  vtkImageMapper3D
  vtkInteractorObserver
  vtkMapper
  vtkMapper2D
  vtkProp
  vtkProp3D
  vtkRendererDelegate
  vtkShader
  vtkShaderDeviceAdapter
  vtkShaderDeviceAdapter2
  vtkShaderProgram
  vtkTextRenderer
  vtkTDxInteractorStyle
  vtkViewport
  vtkVisibilitySort
  vtkStringToImage
  ABSTRACT
  )

set_source_files_properties(
  vtkAbstractInteractionDevice
  vtkAbstractRenderDevice
  vtkRenderWidget
  WRAP_EXCLUDE
  )

# On OS X, vtkRenderWindowJava must be compiled with Objective C++
if(APPLE)
  set_source_files_properties(
    vtkRenderWindow.cxx
    PROPERTIES WRAP_JAVA_OBJC 1
  )
endif()

vtk_module_library(vtkRenderingCore ${Module_SRCS})
