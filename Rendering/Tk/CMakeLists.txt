vtk_module_export_info()

# The cmake code to find the libs is in vtkWrapTcl
set(VTK_WRAP_TCL_FIND_LIBS 1)
include(vtkWrapTcl)

if(TK_FOUND AND VTK_USE_TK)

  set(HAVE_LIMITS_H ${CMAKE_HAVE_LIMITS_H})
  set(HAVE_UNISTD_H ${CMAKE_HAVE_UNISTD_H})
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/vtkTkInternals.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/vtkTkInternals.h)
  if(NOT VTK_INSTALL_NO_DEVELOPMENT)
    install(FILES
      ${CMAKE_CURRENT_BINARY_DIR}/vtkTkInternals.h
      DESTINATION ${VTK_INSTALL_INCLUDE_DIR}
      COMPONENT Development
      )
  endif()

  # Need Tk internal headers for Tk initialization.
  set(try_file "tkInt.h")
  set(try_paths)
  if(WIN32)
    set(try_file "tkWinPort.h")
  endif()
  if (APPLE)
    set(try_file "tkMacOSXDefault.h")
    GET_FILENAME_COMPONENT(TK_INCLUDE_PATH_PARENT "${TK_INCLUDE_PATH}" PATH)
    set(try_paths "${TK_INCLUDE_PATH_PARENT}/PrivateHeaders")
  endif()
  if(try_file)
    VTK_GET_TCL_TK_VERSION ("TCL_TK_MAJOR_VERSION" "TCL_TK_MINOR_VERSION")
    set(TCL_TK_VERSIOND "${TCL_TK_MAJOR_VERSION}.${TCL_TK_MINOR_VERSION}")
    set(try_paths ${try_paths}
         "${VTK_SOURCE_DIR}/ThirdParty/TclTk/internals/tk${TCL_TK_VERSIOND}"
         "${TK_INCLUDE_PATH}/../win"
         "${TK_INCLUDE_PATH}/../macosx")
    find_path(
       TK_INTERNAL_PATH
       ${try_file}
       PATHS ${try_paths}
       DOC "The path to the Tk internal headers (${try_file}).")
    mark_as_advanced(TK_INTERNAL_PATH)
  endif()

  include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${TK_INCLUDE_PATH}
    ${TK_INTERNAL_PATH}
    ${TK_XLIB_PATH}
    ${TCL_INCLUDE_PATH}
    ${VTK_SOURCE_DIR}/Wrapping/Tcl
    ${VTK_BINARY_DIR}/Wrapping/Tcl)

  if(VTK_USE_X)
    include_directories(${VTK_BINARY_DIR}/Utilities/KWSys)
  endif()

  if (VTK_WRAP_TCL)
    set(Module_TCL_SRCS
      vtkTclTkWidgetsInit.cxx
      vtkTkRenderWidget.cxx
      vtkTkImageViewerWidget.cxx)

    if(VTK_USE_COCOA)
      set(Module_TCL_SRCS ${Module_TCL_SRCS} vtkCocoaTkUtilities.mm)

      # Set specified Objective-C++ flags, if any.
      if(VTK_REQUIRED_OBJCXX_FLAGS)
        set_source_files_properties(vtkCocoaTkUtilities.mm
          PROPERTIES COMPILE_FLAGS "${VTK_REQUIRED_OBJCXX_FLAGS}")
      endif(VTK_REQUIRED_OBJCXX_FLAGS)
    endif()

    set(Module_TCL_LIBS ${VTK_TK_LIBRARIES})
    # Use special interactor for X and Tk.
    if(VTK_USE_X)
      find_package(X11 REQUIRED)
      if(NOT X11_Xt_FOUND)
        message(FATAL_ERROR "X11_Xt_LIB could not be found. Required for VTK X lib.")
      endif()
      set(Module_TCL_SRCS ${Module_TCL_SRCS} vtkXRenderWindowTclInteractor.cxx)
      set(Module_TCL_LIBS vtksys vtkRenderingOpenGL ${TK_LIBRARY} ${X11_LIBRARIES})
    endif()

    if(VTK_USE_CARBON)
      set(Module_TCL_LIBS "-framework Carbon")
    endif()

    vtk_add_library(vtkRenderingTkTCL ${Module_TCL_SRCS})

    target_link_libraries(vtkRenderingTkTCL
          vtkRenderingCore vtkCommonDataModel vtkCommonCoreTCL
          vtkInteractionImage
          ${Module_TCL_LIBS})

    set_property(GLOBAL APPEND PROPERTY VTK_TCL_WRAPPED vtkRenderingTk)

  endif()

  if (VTK_WRAP_PYTHON)
    set(Module_PYTHON_SRCS
      vtkPythonTkWidgetsInit.cxx
      vtkTkRenderWidgetPython.cxx
      vtkTkImageViewerWidgetPython.cxx
      )

    if(VTK_USE_COCOA)
      set(Module_PYTHON_SRCS ${Module_PYTHON_SRCS}
          vtkCocoaTkUtilities.mm)

      # Set specified Objective-C++ flags, if any.
      if(VTK_REQUIRED_OBJCXX_FLAGS)
        set_source_files_properties(vtkCocoaTkUtilities.mm
          PROPERTIES COMPILE_FLAGS "${VTK_REQUIRED_OBJCXX_FLAGS}")
      endif(VTK_REQUIRED_OBJCXX_FLAGS)
    endif()

    add_library(vtkRenderingPythonTkWidgets SHARED ${Module_PYTHON_SRCS})
    vtk_target_install(vtkRenderingPythonTkWidgets)

    set_property(TARGET vtkRenderingPythonTkWidgets PROPERTY OUTPUT_NAME
         vtkRenderingPythonTkWidgets-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION})

    set(Module_PYTHON_LIBS ${VTK_TK_LIBRARIES})
    if(VTK_USE_X)
      set(Module_PYTHON_LIBS vtksys vtkRenderingOpenGL ${Module_PYTHON_LIBS}
          ${X11_LIBRARIES})
    endif()

    if(VTK_USE_CARBON)
      set(Module_PYTHON_LIBS ${Module_PYTHON_LIBS} "-framework Carbon")
    endif()

    target_link_libraries(vtkRenderingPythonTkWidgets
                          vtkRenderingCore vtkCommonDataModel
                          vtkInteractionImage
                          ${Module_PYTHON_LIBS})
  endif()
endif()
