vtk_module_export_info()

find_package(TCL)

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
    include(vtkTclTkMacros)

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
    ${TCL_INCLUDE_PATH})

  if(VTK_USE_X)
    include_directories(${VTK_BINARY_DIR}/Utilities/KWSys)
  endif()

  if (VTK_WRAP_PYTHON)
    configure_file(
      "${CMAKE_CURRENT_SOURCE_DIR}/vtkTkAppInitConfigure.h.in"
      "${CMAKE_CURRENT_BINARY_DIR}/vtkTkAppInitConfigure.h")

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
      endif()
    endif()

    add_library(vtkRenderingPythonTkWidgets SHARED ${Module_PYTHON_SRCS})
    vtk_target_install(vtkRenderingPythonTkWidgets)

    set_property(TARGET vtkRenderingPythonTkWidgets PROPERTY OUTPUT_NAME
         vtkRenderingPythonTkWidgets-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION})

    set(Module_PYTHON_LIBS ${VTK_TK_LIBRARIES})
    if(VTK_USE_X)
      set(Module_PYTHON_LIBS vtksys vtkRenderingOpenGL2 ${Module_PYTHON_LIBS}
          ${X11_LIBRARIES})
    endif()

    vtk_module_link_libraries(vtkRenderingPythonTkWidgets
                          vtkRenderingCore vtkCommonDataModel
                          vtkInteractionImage
                          ${Module_PYTHON_LIBS}
                          ${TCL_LIBRARY}
                          ${TK_LIBRARY})
  endif()
endif()
