set(classes
  vtkContourValues
  vtkErrorCode
  vtkExprTkFunctionParser
  vtkFunctionParser
  vtkHeap
  vtkPolygonBuilder
  vtkResourceFileLocator)

# ExprTk uses lots of templates and exceeds object file limits
if (MINGW)
  set_property(SOURCE vtkExprTkFunctionParser.cxx APPEND
    PROPERTY
      COMPILE_FLAGS "-Wa,-mbig-obj")
endif ()

vtk_module_add_module(VTK::CommonMisc
  CLASSES ${classes})
