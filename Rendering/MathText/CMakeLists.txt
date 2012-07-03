# Check that mathtext is available:
set(IMPORT_MATHTEXT "")
execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "import matplotlib.mathtext"}
  RESULT_VARIABLE IMPORT_MATHTEXT_EXITCODE)
set(MATHTEXT_FOUND FALSE)
if(${IMPORT_MATHTEXT_EXITCODE} EQUAL 0)
  set(MATHTEXT_FOUND TRUE)
endif()

if(NOT MATHTEXT_FOUND)
  message(FATAL_ERROR "MathText not found! ('${PYTHON_EXECUTABLE} -c \"import matplotlib.mathtext\"' failed.")
endif()

set(Module_SRCS
  vtkMathTextUtilities.cxx
  )

vtk_module_library(${vtk-module} ${Module_SRCS})
