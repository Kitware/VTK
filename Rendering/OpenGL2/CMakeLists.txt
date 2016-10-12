if(TARGET vtkRenderingOpenGL)
  message(FATAL_ERROR "vtkRenderingOpenGL2 cannot be built with vtkRenderingOpenGL, please disable one of them.")
endif()

include(vtkOpenGL)
include(vtkObjectFactory)


# The TDx support has not been moved, this header is here to define nothing,
# and so allow classes in other modules to safely include it and keep going.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/vtkTDxConfigure.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/vtkTDxConfigure.h)

set(Module_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx
  vtkCameraPass.cxx
  vtkClearRGBPass.cxx
  vtkClearZPass.cxx
  vtkCompositePolyDataMapper2.cxx
  vtkDefaultPass.cxx
  vtkDepthOfFieldPass.cxx
  vtkDepthImageProcessingPass.cxx
  vtkDepthPeelingPass.cxx
  vtkEDLShading.cxx
  vtkFrameBufferObject.cxx
  vtkGaussianBlurPass.cxx
  vtkGenericOpenGLRenderWindow.cxx
  vtkHiddenLineRemovalPass.cxx
  vtkImageProcessingPass.cxx
  vtkLightingMapPass.cxx
  vtkLightsPass.cxx
  vtkOpaquePass.cxx
  vtkOpenGLActor.cxx
  vtkOpenGLBillboardTextActor3D.cxx
  vtkOpenGLBufferObject.cxx
  vtkOpenGLCamera.cxx
  vtkOpenGLFXAAFilter.cxx
  vtkOpenGLGL2PSHelper.cxx
  vtkOpenGLGlyph3DHelper.cxx
  vtkOpenGLGlyph3DMapper.cxx
  vtkOpenGLHardwareSelector.cxx
  vtkOpenGLHelper.cxx
  vtkOpenGLImageAlgorithmHelper.cxx
  vtkOpenGLImageMapper.cxx
  vtkOpenGLImageSliceMapper.cxx
  vtkOpenGLIndexBufferObject.cxx
  vtkOpenGLLabeledContourMapper.cxx
  vtkOpenGLLight.cxx
  vtkOpenGLPointGaussianMapper.cxx
  vtkOpenGLPolyDataMapper.cxx
  vtkOpenGLPolyDataMapper2D.cxx
  vtkOpenGLProperty.cxx
  vtkOpenGLRenderPass.cxx
  vtkOpenGLRenderTimer.cxx
  vtkOpenGLRenderUtilities.cxx
  vtkOpenGLRenderWindow.cxx
  vtkOpenGLRenderer.cxx
  vtkOpenGLShaderCache.cxx
  vtkOpenGLSphereMapper.cxx
  vtkOpenGLStickMapper.cxx
  vtkOpenGLTextActor.cxx
  vtkOpenGLTextActor3D.cxx
  vtkOpenGLTextMapper.cxx
  vtkOpenGLTexture.cxx
  vtkOpenGLVertexArrayObject.cxx
  vtkOpenGLVertexBufferObject.cxx
  vtkOverlayPass.cxx
  vtkPointFillPass.cxx
  vtkRenderPassCollection.cxx
  vtkRenderStepsPass.cxx
  vtkSSAAPass.cxx
  vtkSequencePass.cxx
  vtkShader.cxx
  vtkShaderProgram.cxx
  vtkShadowMapBakerPass.cxx
  vtkShadowMapPass.cxx
  vtkSobelGradientMagnitudePass.cxx
  vtkTextureObject.cxx
  vtkTextureUnitManager.cxx
  vtkTransformFeedback.cxx
  vtkTranslucentPass.cxx
  vtkVolumetricPass.cxx
  )

if (NOT DEFINED OPENGL_ES_VERSION)
  set(Module_SRCS ${Module_SRCS}
    vtkDataTransferHelper.cxx
    vtkDualDepthPeelingPass.cxx # Needs GL_MAX
    vtkFrameBufferObject2.cxx
    vtkPixelBufferObject.cxx
    vtkRenderbuffer.cxx
    vtkValuePass.cxx
    vtkValuePassHelper.cxx
    )
else()
  if (${OPENGL_ES_VERSION} MATCHES 3.0)
    set(Module_SRCS ${Module_SRCS}
      vtkDualDepthPeelingPass.cxx # Needs GL_MAX
      vtkFrameBufferObject2.cxx
      vtkPixelBufferObject.cxx
      vtkRenderbuffer.cxx
      vtkValuePass.cxx
      vtkValuePassHelper.cxx
      )
  endif()
endif()

set(${vtk-module}_HDRS
  vtkOpenGL.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkTDxConfigure.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkOpenGLError.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkRenderingOpenGLConfigure.h
  )

set_source_files_properties(
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx
  vtkFrameBufferObject
  vtkOpenGLBufferObject
  vtkOpenGLGL2PSHelper
  vtkOpenGLGlyph3DHelper
  vtkOpenGLGlyph3DMapper
  vtkOpenGLHelper
  vtkOpenGLIndexBufferObject
  vtkOpenGLRenderTimer
  vtkOpenGLShaderCache
  vtkOpenGLVertexArrayObject
  vtkOpenGLVertexBufferObject
  vtkPixelBufferObject
  vtkRenderbuffer
  vtkTextureObject
  vtkTextureUnitManager
  vtkTransformFeedback
  vtkValuePassHelper
  WRAP_EXCLUDE)

set_source_files_properties(
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx
  vtkOpenGLGL2PSHelper
  vtkOpenGLGlyph3DHelper
  vtkOpenGLHelper
  vtkOpenGLRenderTimer
  vtkValuePassHelper
  PROPERTIES WRAP_EXCLUDE_PYTHON 1)

set_source_files_properties(
  vtkOpenGLGL2PSHelper
  vtkOpenGLRenderWindow
  ABSTRACT
  )

set(shader_files
  glsl/vtkDepthOfFieldPassFS.glsl
  glsl/vtkDepthPeelingPassFinalFS.glsl
  glsl/vtkDepthPeelingPassIntermediateFS.glsl
  glsl/vtkEDLBilateralFilterFS.glsl
  glsl/vtkEDLComposeFS.glsl
  glsl/vtkEDLShadeFS.glsl
  glsl/vtkFXAAFilterFS.glsl
  glsl/vtkGaussianBlurPassFS.glsl
  glsl/vtkGaussianBlurPassVS.glsl
  glsl/vtkGlyph3DVS.glsl
  glsl/vtkPointGaussianVS.glsl
  glsl/vtkPointFillPassFS.glsl
  glsl/vtkPolyData2DFS.glsl
  glsl/vtkPolyData2DVS.glsl
  glsl/vtkPolyDataFS.glsl
  glsl/vtkPolyDataVS.glsl
  glsl/vtkPolyDataWideLineGS.glsl
  glsl/vtkSSAAPassFS.glsl
  glsl/vtkSobelGradientMagnitudePass1FS.glsl
  glsl/vtkSobelGradientMagnitudePass2FS.glsl
  glsl/vtkSphereMapperVS.glsl
  glsl/vtkStickMapperVS.glsl
  glsl/vtkTextureObjectFS.glsl
  glsl/vtkTextureObjectVS.glsl
  )
unset(shader_h_files)
foreach(file ${shader_files})
  get_filename_component(file_we ${file} NAME_WE)
  set(src  ${CMAKE_CURRENT_SOURCE_DIR}/${file})
  set(res  ${CMAKE_CURRENT_BINARY_DIR}/${file_we}.cxx)
  set(resh ${CMAKE_CURRENT_BINARY_DIR}/${file_we}.h)
  list(APPEND shader_h_files ${resh})
  add_custom_command(
    OUTPUT ${res} ${resh}
    DEPENDS ${src} vtkEncodeString
    COMMAND vtkEncodeString
    ARGS ${res} ${src} ${file_we}
      --build-header VTKRENDERINGOPENGL2_EXPORT vtkRenderingOpenGL2Module.h
    )
  list(APPEND Module_SRCS ${res})
  set_source_files_properties(${file_we}
    PROPERTIES
      WRAP_EXCLUDE 1
      WRAP_EXCLUDE_PYTHON 1
    )
endforeach()

if(VTK_USE_X)
  find_package(X11 REQUIRED)
  if(NOT X11_Xt_FOUND)
    message(FATAL_ERROR "X11_Xt_LIB could not be found. Required for VTK X lib.")
  endif()
  include_directories(${X11_INCLUDE_DIR} ${X11_Xt_INCLUDE_PATH})
endif()

# setup factory overrides
set(opengl_overrides
  Actor
  BillboardTextActor3D
  Camera
  LabeledContourMapper
  HardwareSelector
  ImageMapper
  ImageSliceMapper
  Glyph3DMapper
  Light
  PointGaussianMapper
  PolyDataMapper
  PolyDataMapper2D
  Property
  Renderer
  TextActor
  TextActor3D
  TextMapper
  Texture
  )

# Now to see about the GPU info list.
list(APPEND vtk_module_overrides "vtkGPUInfoList")
list(APPEND Module_SRCS "vtkDummyGPUInfoList.cxx")
set(vtk_module_vtkGPUInfoList_override "vtkDummyGPUInfoList")

foreach(_override ${opengl_overrides})
  vtk_add_override(vtk${_override} vtkOpenGL${_override})
endforeach()

# FIXME: Not sure that this is the best way to handle X...
if(VTK_USE_X)
  find_package(X11 REQUIRED)
  if(NOT X11_Xt_FOUND)
    message(FATAL_ERROR "X11_Xt_LIB could not be found. Required for VTK X lib.")
  endif()
  include_directories(${X11_INCLUDE_DIR} ${X11_Xt_INCLUDE_PATH})
  # Add some custom overrides
  list(APPEND vtk_module_overrides "vtkRenderWindowInteractor")
  set(vtk_module_vtkRenderWindowInteractor_override "vtkXRenderWindowInteractor")
  list(APPEND vtk_module_overrides "vtkRenderWindow")
  set(vtk_module_vtkRenderWindow_override "vtkXOpenGLRenderWindow")
  list(APPEND Module_SRCS
    vtkXRenderWindowInteractor.cxx
    vtkXOpenGLRenderWindow.cxx
    )
elseif(VTK_USE_OSMESA)
  list(APPEND vtk_module_overrides "vtkRenderWindow")
  set(vtk_module_vtkRenderWindow_override "vtkOSOpenGLRenderWindow")
  list(APPEND Module_SRCS vtkOSOpenGLRenderWindow.cxx)
elseif(VTK_USE_OFFSCREEN_EGL)
  list(APPEND vtk_module_overrides "vtkRenderWindow")
  set(vtk_module_vtkRenderWindow_override "vtkEGLRenderWindow")
  list(APPEND Module_SRCS vtkEGLRenderWindow.cxx)
  set_source_files_properties(
    vtkEGLRenderWindow.cxx
    PROPERTIES COMPILE_DEFINITIONS "VTK_EGL_DEVICE_INDEX=${VTK_EGL_DEVICE_INDEX};VTK_USE_OFFSCREEN_EGL")
elseif(WIN32)
  # Add some custom overrides
  list(APPEND vtk_module_overrides "vtkRenderWindowInteractor")
  set(vtk_module_vtkRenderWindowInteractor_override "vtkWin32RenderWindowInteractor")
  list(APPEND vtk_module_overrides "vtkRenderWindow")
  set(vtk_module_vtkRenderWindow_override "vtkWin32OpenGLRenderWindow")
  list(APPEND Module_SRCS
    vtkWin32RenderWindowInteractor.cxx
    vtkWin32OpenGLRenderWindow.cxx
    )
elseif(VTK_USE_COCOA)
  # Add some custom overrides
  list(APPEND vtk_module_overrides "vtkRenderWindowInteractor")
  set(vtk_module_vtkRenderWindowInteractor_override "vtkCocoaRenderWindowInteractor")
  list(APPEND vtk_module_overrides "vtkRenderWindow")
  set(vtk_module_vtkRenderWindow_override "vtkCocoaRenderWindow")
  list(APPEND Module_SRCS
    vtkCocoaRenderWindowInteractor.mm
    vtkCocoaRenderWindow.mm
    vtkCocoaGLView.mm
    )
  list(APPEND ${vtk-module}_HDRS
    vtkCocoaMacOSXSDKCompatibility.h
    )
  # Set specified Objective-C++ flags, if any.
  if(VTK_REQUIRED_OBJCXX_FLAGS)
    set_source_files_properties(vtkCocoaRenderWindowInteractor.mm
                                vtkCocoaRenderWindow.mm
                                vtkCocoaGLView.mm
                                PROPERTIES COMPILE_FLAGS "${VTK_REQUIRED_OBJCXX_FLAGS}")
  endif()
  set_source_files_properties(
    vtkCocoaGLView
    PROPERTIES
      WRAP_EXCLUDE 1
      WRAP_EXCLUDE_PYTHON 1
    )
elseif(ANDROID)
  # Add some custom overrides
  list(APPEND vtk_module_overrides "vtkRenderWindowInteractor")
  set(vtk_module_vtkRenderWindowInteractor_override "vtkAndroidRenderWindowInteractor")
  list(APPEND vtk_module_overrides "vtkRenderWindow")
  set(vtk_module_vtkRenderWindow_override "vtkEGLRenderWindow")
  list(APPEND Module_SRCS
    vtkAndroidRenderWindowInteractor.cxx
    vtkEGLRenderWindow.cxx
    )
  include_directories(
    "${CMAKE_ANDROID_NDK}/sources/android/native_app_glue"
    )
elseif(APPLE_IOS)
  # Add some custom overrides
  list(APPEND vtk_module_overrides "vtkRenderWindowInteractor")
  set(vtk_module_vtkRenderWindowInteractor_override "vtkIOSRenderWindowInteractor")
  list(APPEND vtk_module_overrides "vtkRenderWindow")
  set(vtk_module_vtkRenderWindow_override "vtkIOSRenderWindow")
  list(APPEND Module_SRCS
    vtkIOSRenderWindowInteractor.mm
    vtkIOSRenderWindow.mm
    )
  # Set specified Objective-C++ flags, if any.
  if(VTK_REQUIRED_OBJCXX_FLAGS)
    set_source_files_properties(vtkIOSRenderWindowInteractor.mm
                                vtkIOSRenderWindow.mm
                                PROPERTIES COMPILE_FLAGS "${VTK_REQUIRED_OBJCXX_FLAGS}")
  endif()
 endif()

vtk_object_factory_configure("${vtk_module_overrides}")

# Detect and clear OpenGL errors. If not then other classes/user code
# may detect VTK's OpenGL errors and vice-versa.
set(VTK_REPORT_OPENGL_ERRORS ON CACHE BOOL "Enable OpenGL error check and report")
cmake_dependent_option(VTK_REPORT_OPENGL_ERRORS_IN_RELEASE_BUILDS
  "Enable OpenGL error check and reporting in non-debug builds." OFF
  VTK_REPORT_OPENGL_ERRORS OFF)
mark_as_advanced(VTK_REPORT_OPENGL_ERRORS VTK_REPORT_OPENGL_ERRORS_IN_RELEASE_BUILDS)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/vtkOpenGLError.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/vtkOpenGLError.h)

# Configure the module specific settings into a module configured header.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/vtkRenderingOpenGLConfigure.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/vtkRenderingOpenGLConfigure.h)
# Set up some of the module specific preprocessor definitions.
set(${vtk-module}_EXPORT_CODE "#include \"vtkRenderingOpenGLConfigure.h\"")

vtk_module_library(${vtk-module} ${Module_SRCS})
vtk_opengl_link(${vtk-module})
if(VTK_USE_X)
  vtk_module_link_libraries(${vtk-module}
    LINK_PUBLIC ${X11_LIBRARIES} ${X11_Xt_LIB})
elseif(VTK_USE_COCOA)
  vtk_module_link_libraries(${vtk-module} LINK_PUBLIC "-framework Cocoa")
endif()

if(ANDROID)
  target_link_libraries(${vtk-module}
    LINK_PUBLIC ${OPENGL_egl_LIBRARY}
    LINK_PRIVATE android
    )
endif()

if (APPLE_IOS)
  vtk_module_link_libraries(${vtk-module} LINK_PUBLIC "-framework UIKit")
endif()
