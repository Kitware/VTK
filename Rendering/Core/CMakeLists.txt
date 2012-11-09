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
  vtkCameraActor.cxx
  vtkCamera.cxx
  vtkCameraInterpolator.cxx
  vtkCellCenterDepthSort.cxx
  vtkChooserPainter.cxx
  vtkClipPlanesPainter.cxx
  vtkCoincidentTopologyResolutionPainter.cxx
  vtkColorTransferFunction.cxx
  vtkCompositePainter.cxx
  vtkCompositePolyDataMapper2.cxx
  vtkCompositePolyDataMapper.cxx
  vtkCoordinate.cxx
  vtkCullerCollection.cxx
  vtkCuller.cxx
  vtkDataSetMapper.cxx
  vtkDefaultPainter.cxx
  vtkDiscretizableColorTransferFunction.cxx
  vtkDisplayListPainter.cxx
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
  vtkLightActor.cxx
  vtkLightCollection.cxx
  vtkLight.cxx
  vtkLightingPainter.cxx
  vtkLightKit.cxx
  vtkLinesPainter.cxx
  vtkLogLookupTable.cxx
  vtkLookupTableWithEnabling.cxx
  vtkMapArrayValues.cxx
  vtkMapper2D.cxx
  vtkMapperCollection.cxx
  vtkMapper.cxx
  vtkMaterialLibrary.cxx
  vtkObserverMediator.cxx
  vtkPainter.cxx
  vtkPainterDeviceAdapter.cxx
  vtkPainterPolyDataMapper.cxx
  vtkPointsPainter.cxx
  vtkPolyDataMapper2D.cxx
  vtkPolyDataMapper.cxx
  vtkPolyDataPainter.cxx
  vtkPolygonsPainter.cxx
  vtkPrimitivePainter.cxx
  vtkProp3DCollection.cxx
  vtkProp3D.cxx
  vtkProp3DFollower.cxx
  vtkPropAssembly.cxx
  vtkPropCollection.cxx
  vtkProp.cxx
  vtkProperty2D.cxx
  vtkProperty.cxx # needs a load of image readers - IOImage, IOXML
  vtkRendererCollection.cxx
  vtkRenderer.cxx
  vtkRendererDelegate.cxx
  vtkRendererSource.cxx
  vtkRenderWindowCollection.cxx
  vtkRenderWindow.cxx
  vtkRenderWindowInteractor.cxx
  vtkRepresentationPainter.cxx
  vtkScalarsToColorsPainter.cxx
  vtkSelectVisiblePoints.cxx
  vtkShader.cxx # vtkXMLShader
  vtkShaderCodeLibrary.cxx
  vtkShaderDeviceAdapter.cxx
  vtkShaderDeviceAdapter2.cxx
  vtkShaderProgram.cxx # vtkXMLShaderProgram
  vtkStandardPolyDataPainter.cxx
  vtkTexture.cxx
  vtkTexturedActor2D.cxx
  vtkTransformCoordinateSystems.cxx
  vtkTransformInterpolator.cxx
  vtkTStripsPainter.cxx
  vtkTupleInterpolator.cxx
  vtkViewDependentErrorMetric.cxx
  vtkViewport.cxx
  vtkVisibilitySort.cxx
  vtkVolumeCollection.cxx
  vtkVolume.cxx
  vtkVolumeProperty.cxx
  vtkXMLMaterial.cxx
  vtkXMLMaterialParser.cxx
  vtkXMLMaterialReader.cxx
  vtkXMLShader.cxx
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
  vtkHardwareSelectionPolyDataPainter.cxx # Needed by vtkPainterPolyDataMapper

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
  )

set(${vtk-module}_HDRS
  vtkGPUInfoListArray.h
  vtkNoise200x200.h
  vtkPythagoreanQuadruples.h
  vtkRayCastStructures.h
  vtkRenderingCoreEnums.h
  vtkTDxMotionEventInfo.h
  )

set_source_files_properties(
  vtkAbstractMapper3D
  vtkAbstractMapper
  vtkAbstractVolumeMapper
  vtkAbstractPicker
  vtkAbstractPropPicker
  vtkCuller
  vtkImageMapper3D
  vtkInteractorObserver
  vtkMapper
  vtkMapper2D
  vtkPainter
  vtkPolyDataPainter
  vtkPrimitivePainter
  vtkProp
  vtkProp3D
  vtkRendererDelegate
  vtkShader
  vtkShaderDeviceAdapter
  vtkShaderDeviceAdapter2
  vtkShaderProgram
  vtkTDxInteractorStyle
  vtkViewport
  vtkVisibilitySort
  vtkStringToImage
  ABSTRACT
  )

# On OS X, vtkRenderWindowJava must be compiled with Objective C++
if(APPLE)
  set_source_files_properties(
    vtkRenderWindow.cxx
    PROPERTIES WRAP_JAVA_OBJC 1
  )
endif()

vtk_module_library(vtkRenderingCore ${Module_SRCS})
