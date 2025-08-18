
set(classes
  vtkGenericRenderWindowInteractor
)

set(headers)
set(nowrap_headers)

# Setup factory overrides and add window system specific render windows.
if (VTK_USE_X)
  # Add some custom overrides
  if (NOT VTK_DEFAULT_RENDER_WINDOW_HEADLESS)
    vtk_object_factory_declare(
      BASE vtkRenderWindowInteractor
      OVERRIDE vtkXRenderWindowInteractor)
  endif ()
  list(APPEND classes vtkXRenderWindowInteractor)
elseif (WIN32)
  # Add some custom overrides
  if (NOT VTK_DEFAULT_RENDER_WINDOW_HEADLESS)
    vtk_object_factory_declare(
      BASE vtkRenderWindowInteractor
      OVERRIDE vtkWin32RenderWindowInteractor)
    vtk_object_factory_declare(
      BASE vtkHardwareWindow
      OVERRIDE vtkWin32HardwareWindow)
  endif ()
  list(APPEND classes vtkWin32HardwareWindow vtkWin32RenderWindowInteractor)
elseif (VTK_USE_COCOA)
  # Add some custom overrides
  if (NOT VTK_DEFAULT_RENDER_WINDOW_HEADLESS)
    vtk_object_factory_declare(
      BASE vtkRenderWindowInteractor
      OVERRIDE vtkCocoaRenderWindowInteractor)
  endif ()
  list(APPEND sources
    vtkCocoaAutoreleasePool.mm
    vtkCocoaRenderWindowInteractor.mm)
  list(APPEND headers
    vtkCocoaAutoreleasePool.h
    vtkCocoaRenderWindowInteractor.h)
  list(APPEND nowrap_headers
    vtkCocoaMacOSXSDKCompatibility.h)
  # Set specified Objective-C++ flags, if any.
  if (VTK_REQUIRED_OBJCXX_FLAGS)
    set_source_files_properties(
      vtkCocoaRenderWindowInteractor.mm
      PROPERTIES
        COMPILE_FLAGS "${VTK_REQUIRED_OBJCXX_FLAGS}")
  endif ()
elseif (ANDROID)
  # Add some custom overrides
  vtk_object_factory_declare(
    BASE vtkRenderWindowInteractor
    OVERRIDE vtkAndroidRenderWindowInteractor)
  list(APPEND classes vtkAndroidRenderWindowInteractor)
  include_directories("${CMAKE_ANDROID_NDK}/sources/android/native_app_glue")
elseif (APPLE_IOS)
  # Add some custom overrides
  vtk_object_factory_declare(
    BASE vtkRenderWindowInteractor
    OVERRIDE vtkIOSRenderWindowInteractor)
  list(APPEND sources vtkIOSRenderWindowInteractor.mm)
  list(APPEND headers vtkIOSRenderWindowInteractor.h)
  # Set specified Objective-C++ flags, if any.
  if (VTK_REQUIRED_OBJCXX_FLAGS)
    set_source_files_properties(
      vtkIOSRenderWindowInteractor.mm
      PROPERTIES
        COMPILE_FLAGS "${VTK_REQUIRED_OBJCXX_FLAGS}")
  endif ()
elseif (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  # Add some custom overrides
  vtk_object_factory_declare(
    BASE vtkRenderWindowInteractor
    OVERRIDE vtkWebAssemblyRenderWindowInteractor)
  list(APPEND classes vtkWebAssemblyRenderWindowInteractor)
else () # use generic
  vtk_object_factory_declare(
    BASE vtkRenderWindowInteractor
    OVERRIDE vtkGenericRenderWindowInteractor)
endif ()

vtk_object_factory_configure(
  SOURCE_FILE vtk_object_factory_source
  HEADER_FILE vtk_object_factory_header
  EXPORT_MACRO "VTKRENDERINGUI_EXPORT"
  EXTRA_INCLUDES "<vtkCollection.h>" "<vtkObjectFactoryCollection.h>" "<vtkLogger.h>" "<cstdlib>")

vtk_module_add_module(VTK::RenderingUI
  CLASSES ${classes}
  HEADERS ${headers}
  NOWRAP_HEADERS ${nowrap_headers}
  SOURCES ${sources} ${vtk_object_factory_source}
  PRIVATE_HEADERS ${vtk_object_factory_header}
  )

if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  vtk_module_link_options(VTK::RenderingUI
    INTERFACE
      "$<BUILD_INTERFACE:--js-library=${CMAKE_CURRENT_SOURCE_DIR}/vtkWebAssemblyRenderWindowInteractor.js>"
      "$<INSTALL_INTERFACE:--js-library=$<INSTALL_PREFIX>/${_vtk_build_LIBRARY_DESTINATION}/vtkWebAssemblyRenderWindowInteractor.js>")
  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/vtkWebAssemblyRenderWindowInteractor.js"
    DESTINATION ${_vtk_build_LIBRARY_DESTINATION})
endif ()

if (VTK_USE_X)
  vtk_module_find_package(
    PACKAGE X11)
  vtk_module_link(VTK::RenderingUI
    PUBLIC
      X11::X11)
elseif (VTK_USE_COCOA)
  vtk_module_link(VTK::RenderingUI PUBLIC "-framework Cocoa")
endif ()

if (ANDROID)
  vtk_module_link(VTK::RenderingUI
    PRIVATE
      android)
endif ()

if (APPLE_IOS)
  vtk_module_link(VTK::RenderingUI PUBLIC "-framework UIKit")
endif ()
vtk_add_test_mangling(VTK::RenderingUI)
