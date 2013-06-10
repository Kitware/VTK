set(Module_SRCS
  vtkAmoebaMinimizer.cxx
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
  vtkQuaternion.h
  vtkTuple.h
  )

if(NOT VTK_LEGACY_REMOVE)
  list(APPEND Module_SRCS
    vtkFastNumericConversion.cxx
    )
endif()

set_source_files_properties(
  vtkQuaternion
  vtkTuple
  WRAP_EXCLUDE
  )

set_source_files_properties(
  vtkQuaternion.h
  vtkTuple.h
  PROPERTIES WRAP_SPECIAL 1
  )

set(
  ${vtk-module}_HDRS
  vtkQuaternion.txx
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
