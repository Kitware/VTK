set(classes
  vtkContourValues
  vtkErrorCode
  vtkExprTkFunctionParser
  vtkFunctionParser
  vtkHeap
  vtkPolygonBuilder
  vtkResourceFileLocator)

# ExprTk uses lots of templates and exceeds object file limits
if (MINGW AND NOT ANDROID) # MINGW is set when cross-compiling for ANDROID, see issue https://gitlab.kitware.com/cmake/cmake/-/issues/22647
  set_property(SOURCE vtkExprTkFunctionParser.cxx APPEND
    PROPERTY
      COMPILE_FLAGS "-Wa,-mbig-obj")
endif ()

# ExprTk internally relies on exceptions. This compile flag enables exception support in emscripten compiler.
if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  if (NOT VTK_WEBASSEMBLY_EXCEPTIONS)
    message(STATUS "Selectively enabling exceptions for vtkExprTkFunctionParser.cxx. "
      "Please consider turning VTK_WEBASSEMBLY_EXCEPTIONS ON for project-wide exception support.")
    set_property(SOURCE vtkExprTkFunctionParser.cxx APPEND
      PROPERTY
        COMPILE_FLAGS "-fwasm-exceptions")
  endif ()
endif ()

vtk_module_add_module(VTK::CommonMisc
  CLASSES ${classes})

# The final executable linking to CommonMisc must use -fwasm-exceptions as well.
if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  if (NOT VTK_WEBASSEMBLY_EXCEPTIONS)
    _vtk_module_real_target(common_misc_target VTK::CommonMisc)
    set_property(TARGET "${common_misc_target}" APPEND
      PROPERTY
        LINK_FLAGS "-fwasm-exceptions")
  endif ()
endif ()
vtk_add_test_mangling(VTK::CommonMisc)
