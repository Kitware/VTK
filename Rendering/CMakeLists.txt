SOURCE_FILES ( Rendering_SRCS
vtkAbstractMapper3D
vtkAbstractPicker
vtkAbstractPropPicker
vtkActor
vtkActorCollection
vtkAssembly
vtkCamera
vtkCellPicker
vtkCuller
vtkCullerCollection
vtkDataSetMapper
vtkDirectionEncoder
vtkEncodedGradientEstimator
vtkEncodedGradientShader
vtkExporter
vtkFiniteDifferenceGradientEstimator
vtkFollower
vtkFrustumCoverageCuller
vtkGraphicsFactory
vtkHull
vtkImageActor
vtkImageMapper
vtkImager
vtkImagerCollection
vtkImageWindow
vtkImagingFactory
vtkImporter
vtkInteractorStyle
vtkInteractorStyleFlight
vtkInteractorStyleJoystickActor
vtkInteractorStyleJoystickCamera
vtkInteractorStyleSwitch
vtkInteractorStyleTrackball
vtkInteractorStyleTrackballActor
vtkInteractorStyleTrackballCamera
vtkInteractorStyleUnicam
vtkInteractorStyleUser
vtkLODActor
vtkLODProp3D
vtkLight
vtkLightCollection
vtkMapper
vtkMapperCollection
vtkPicker
vtkPointPicker
vtkPolyDataMapper
vtkPolyDataMapper2D
vtkProjectedPolyDataRayBounder
vtkProp3D
vtkProp3DCollection
vtkPropPicker
vtkProperty
vtkRayBounder
vtkRayCaster
vtkRecursiveSphereDirectionEncoder
vtkRenderWindow
vtkRenderWindowCollection
vtkRenderWindowInteractor
vtkRenderer
vtkRendererCollection
vtkRendererSource
vtkSelectVisiblePoints
vtkTextMapper
vtkTexture
vtkViewRays
vtkVolume
vtkVolumeCollection
vtkVolumeMapper
vtkVolumeProperty
vtkVolumeRayCastCompositeFunction
vtkVolumeRayCastFunction
vtkVolumeRayCastIsosurfaceFunction
vtkVolumeRayCastMIPFunction
vtkVolumeRayCastMapper
vtkVolumeTextureMapper
vtkVolumeTextureMapper2D
vtkWorldPointPicker
)

ABSTRACT_FILES(
vtkAbstractMapper3D
vtkAbstractPicker 
vtkAbstractPropPicker 
vtkCuller
vtkDirectionEncoder
vtkEncodedGradientEstimator 
vtkExporter
vtkGraphicsFactory
vtkImporter
vtkMapper
vtkProp3D
vtkRayBounder
vtkVolumeMapper 
vtkVolumeRayCastFunction 
vtkVolumeReader 
vtkVolumeTextureMapper 
)

#vtkMesaImageActor.h
#vtkMesaImageMapper.h
#vtkMesaImageWindow.h
#vtkMesaImager.h
#vtkXImageMapper.h
#vtkXImageWindow.h
#vtkOpenGLImageWindow

SOURCE_FILES ( Rendering_SRCS
vtkOpenGLActor
vtkOpenGLCamera
vtkOpenGLImageActor
vtkOpenGLImageMapper
vtkOpenGLImager
vtkOpenGLLight
vtkOpenGLPolyDataMapper
vtkOpenGLPolyDataMapper2D
vtkOpenGLProjectedPolyDataRayBounder
vtkOpenGLProperty
vtkOpenGLRenderer
vtkOpenGLTexture
vtkOpenGLVolumeTextureMapper2D
)

IF (WIN32)
  SOURCE_FILES(Rendering_SRCS 
     vtkWin32OpenGLRenderWindow 
     vtkWin32RenderWindowInteractor
     vtkWin32ImageMapper
     vtkWin32ImageWindow
     vtkWin32OpenGLImageWindow
     vtkWin32OpenGLTextMapper
     vtkWin32TextMapper)
  LINK_LIBRARIES (opengl32)
ENDIF (WIN32)

ADD_LIBRARY(vtkRendering Rendering_SRCS)
#WRAP_EXCLUDE_FILES(vtkImagingFactory)

IF (VTK_WRAP_TCL)
#  SOURCE_FILES(RenderingTCL_SRCS 
#    vtkTkImageViewerWidget vtkTkImageWindowWidget)
  VTK_WRAP_TCL(vtkRenderingTCL RenderingTCL_SRCS Rendering_SRCS)
  ADD_LIBRARY(vtkRenderingTCL RenderingTCL_SRCS)
  TARGET_LINK_LIBRARIES (vtkRenderingTCL vtkIOTCL vtkGraphicsTCL 
                         vtkImagingTCL vtkFilteringTCL vtkCommonTCL)
ENDIF (VTK_WRAP_TCL)

LINK_LIBRARIES (
vtkCommon 
vtkFiltering 
vtkImaging
vtkGraphics
vtkIO
vtkRendering
)


