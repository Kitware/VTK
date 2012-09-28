set(Module_SRCS
  vtkAmoebaMinimizer.cxx
  vtkFastNumericConversion.cxx
  vtkFunctionSet.cxx
  vtkInitialValueProblemSolver.cxx
  vtkMatrix3x3.cxx
  vtkMatrix4x4.cxx
  vtkPolynomialSolversUnivariate.cxx
  vtkQuaternionInterpolator.cxx
  vtkRungeKutta2.cxx
  vtkRungeKutta4.cxx
  vtkRungeKutta45.cxx

  # Header only classes
  vtkTuple.h
  )

set_source_files_properties(
  vtkTuple
  WRAP_EXCLUDE
  )

set_source_files_properties(
  vtkTuple.h
  PROPERTIES WRAP_SPECIAL 1
  )

if(CMAKE_COMPILER_IS_GNUCXX)
  set_source_files_properties(vtkPolynomialSolversUnivariate.cxx
    PROPERTIES COMPILE_FLAGS -O0)
endif()

set_source_files_properties(
  vtkFunctionSet
  vtkInitialValueProblemSolver
  ABSTRACT
  )

vtk_module_library(vtkCommonMath ${Module_SRCS})
