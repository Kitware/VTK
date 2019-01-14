set(classes
  vtkAmoebaMinimizer
  vtkFunctionSet
  vtkInitialValueProblemSolver
  vtkMatrix3x3
  vtkMatrix4x4
  vtkPolynomialSolversUnivariate
  vtkQuaternionInterpolator
  vtkRungeKutta2
  vtkRungeKutta4
  vtkRungeKutta45)

set(template_classes
  vtkQuaternion)

set(headers
  vtkTuple.h)

if(CMAKE_COMPILER_IS_GNUCXX)
  set_source_files_properties(vtkPolynomialSolversUnivariate.cxx
    PROPERTIES
      COMPILE_FLAGS -O0)
endif()

vtk_module_add_module(VTK::CommonMath
  CLASSES           ${classes}
  TEMPLATE_CLASSES  ${template_classes}
  HEADERS           ${headers})
