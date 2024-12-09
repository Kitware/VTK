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
  set_property(SOURCE vtkExprTkFunctionParser.cxx APPEND
    PROPERTY
      COMPILE_FLAGS "-fexceptions")
endif ()

vtk_module_add_module(VTK::CommonMisc
  CLASSES ${classes})

# The final executable linking to CommonMisc must use -sDISABLE_EXCEPTION_CATCHING=0 and -fexceptions as well.
if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  vtk_module_link_options(VTK::CommonMisc
    INTERFACE
      "-sDISABLE_EXCEPTION_CATCHING=0"
      "-fexceptions")
endif ()
vtk_add_test_mangling(VTK::CommonMisc)
