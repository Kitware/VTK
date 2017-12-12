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

set(
  ${vtk-module}_HDRS
  vtkQuaternion.txx
  )

if(CMAKE_COMPILER_IS_GNUCXX)
  set_source_files_properties(vtkPolynomialSolversUnivariate.cxx
    PROPERTIES COMPILE_FLAGS -O0)
endif()

vtk_module_library(vtkCommonMath ${Module_SRCS})
