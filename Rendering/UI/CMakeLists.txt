
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
  list(APPEND sources vtkCocoaRenderWindowInteractor.mm)
  list(APPEND headers
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
else () # use generic
  vtk_object_factory_declare(
    BASE vtkRenderWindowInteractor
    OVERRIDE vtkGenericRenderWindowInteractor)
endif ()

if (VTK_USE_SDL2 OR CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  # Add some custom overrides
  vtk_object_factory_declare(
    BASE vtkRenderWindowInteractor
    OVERRIDE vtkSDL2RenderWindowInteractor)
  list(APPEND classes vtkSDL2RenderWindowInteractor)
  set (intial_cpp_code
    "
    // defaults
    #ifdef __EMSCRIPTEN__
    int use_platform = 0\;
    int use_sdl2 = 1\;
    #else
    int use_platform = 1\;
    int use_sdl2 = 0\;
    #endif
    // Query the enviromental for backend to use.
    if(const char* p = std::getenv(\"VTK_WINDOW_BACKEND\"))
    {
      const std::string backend(p)\;
      if(backend == \"PLATFORM\")
      {
        use_sdl2 = 0\;
        use_platform = 1\;
      }
      else if(backend == \"SDL2\")
      {
        #ifndef VTK_ENABLE_SDL2
        vtkLogF(ERROR,\"SDL2 backend requested but the SDL2 backend was not compiled\")\;
        #else
        use_sdl2 = 1\;
        use_platform = 0\;
        #endif
      }
      else
      {
        vtkLogF(ERROR,\"Unknown backend '%p', using default platform native window\",backend.c_str())\;
      }
    }
      // register the factory before modyfing the overrrides so it is found below
    vtkRenderingUIObjectFactory* renderingUIFactory = vtkRenderingUIObjectFactory::New()\;
    if (renderingUIFactory)
    {
      // vtkObjectFactory keeps a reference to the factory,
      vtkObjectFactory::RegisterFactory(renderingUIFactory)\;
      renderingUIFactory->Delete()\;
    }
    vtkObjectFactory* object_factory\;
    vtkCollectionSimpleIterator osit\;
    for (vtkObjectFactory::GetRegisteredFactories()->InitTraversal(osit)\;
        (object_factory = vtkObjectFactory::GetRegisteredFactories()->GetNextObjectFactory(osit))\;)
    {
      if (object_factory->HasOverride(\"vtkRenderWindowInteractor\"))
      {
        if (use_sdl2 && !use_platform)
        {
          object_factory->SetEnableFlag(use_sdl2, \"vtkRenderWindowInteractor\", \"vtkSDL2RenderWindowInteractor\")\;
          // disable ALL other interactor overrides.
          object_factory->SetEnableFlag(0, \"vtkRenderWindowInteractor\", \"vtkGenericRenderWindowInteractor\")\;
          object_factory->SetEnableFlag(0, \"vtkRenderWindowInteractor\", \"vtkCocoaRenderWindowInteractor\")\;
          object_factory->SetEnableFlag(0, \"vtkRenderWindowInteractor\", \"vtkWin32RenderWindowInteractor\")\;
          object_factory->SetEnableFlag(0, \"vtkRenderWindowInteractor\", \"vtkXRenderWindowInteractor\")\;
        }
      }
    }"
  )
endif()

vtk_object_factory_configure(
  SOURCE_FILE vtk_object_factory_source
  HEADER_FILE vtk_object_factory_header
  EXPORT_MACRO "VTKRENDERINGUI_EXPORT"
  EXTRA_INCLUDES "<vtkCollection.h>" "<vtkObjectFactoryCollection.h>" "<vtkLogger.h>" "<cstdlib>"
  INITIAL_CODE ${intial_cpp_code})

vtk_module_add_module(VTK::RenderingUI
  CLASSES ${classes}
  HEADERS ${headers}
  NOWRAP_HEADERS ${nowrap_headers}
  SOURCES ${sources} ${vtk_object_factory_source}
  PRIVATE_HEADERS ${vtk_object_factory_header}
  )

if (VTK_USE_SDL2 OR CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  vtk_module_find_package(PACKAGE SDL2)
  vtk_module_link(VTK::RenderingUI
    PRIVATE
      SDL2::SDL2)
  vtk_module_definitions(VTK::RenderingUI
    PRIVATE VTK_ENABLE_SDL2)
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
