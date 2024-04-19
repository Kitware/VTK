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

vtk_module_add_module(VTK::CommonMisc
  CLASSES ${classes})
vtk_add_test_mangling(VTK::CommonMisc)
