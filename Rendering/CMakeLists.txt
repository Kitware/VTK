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
vtkImageViewer
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
     vtkWin32OpenGLImageWindow
     vtkWin32OpenGLTextMapper
     vtkWin32TextMapper)
  LINK_LIBRARIES (opengl32)
ENDIF (WIN32)

ADD_LIBRARY(vtkRendering Rendering_SRCS)
#WRAP_EXCLUDE_FILES(vtkImagingFactory)

IF (VTK_WRAP_TCL)
  SOURCE_FILES(RenderingTCL_SRCS 
    vtkTkRenderWidget vtkTkImageViewerWidget vtkTkImageWindowWidget)

  # add in the Tk values if found
  IF (TK_INCLUDE_PATH)
    INCLUDE_DIRECTORIES(${TK_INCLUDE_PATH})
  ENDIF (TK_INCLUDE_PATH)
  IF (TK_LIB_PATH)
    LINK_DIRECTORIES (${TK_LIB_PATH})
  ENDIF (TK_LIB_PATH)
  IF (TK_LIBRARY)
    LINK_LIBRARIES (${TK_LIBRARY})
  ENDIF (TK_LIBRARY)

  # find the tk internals for win32
  IF (WIN32)
    FIND_PATH(TK_INTERNAL_PATH tkWinInt.h 
      ${VTK_SOURCE_DIR}/Rendering/tkInternals)
    IF (TK_INTERNAL_PATH)
      INCLUDE_DIRECTORIES(${TK_INTERNAL_PATH})
    ENDIF (TK_INTERNAL_PATH)  
  ENDIF (WIN32)  
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


