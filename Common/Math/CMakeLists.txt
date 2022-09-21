set(classes
  vtkAmoebaMinimizer
  vtkFFT
  vtkFunctionSet
  vtkInitialValueProblemSolver
  vtkMatrix3x3
  vtkMatrix4x4
  vtkPolynomialSolversUnivariate
  vtkQuaternionInterpolator
  vtkRungeKutta2
  vtkRungeKutta4
  vtkRungeKutta45
)

set(template_classes
  vtkQuaternion
)

set(templates
  vtkFFT.txx)

set(headers
  vtkTuple.h
)

set(nowrap_classes
  vtkReservoirSampler
)

if(CMAKE_COMPILER_IS_GNUCXX)
  set_source_files_properties(vtkPolynomialSolversUnivariate.cxx
    PROPERTIES
      COMPILE_FLAGS -O0)
endif()

vtk_module_add_module(VTK::CommonMath
  CLASSES           ${classes}
  TEMPLATES         ${templates}
  TEMPLATE_CLASSES  ${template_classes}
  HEADERS           ${headers}
  NOWRAP_CLASSES    ${nowrap_classes}
)
vtk_add_test_mangling(VTK::CommonMath)
