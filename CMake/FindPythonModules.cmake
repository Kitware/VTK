
# Check whether a Python module is available by name, and if it is,
# define a variable in the internal cache.
macro(_find_python_module_internal module_name)
  # Check for presence of the module.  Even though we don't use all the
  # variable names set here, assigning them suppresses their output in CMake.
  execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "import ${module_name}"
    RESULT_VARIABLE IMPORT_${module_name}_EXITCODE
    OUTPUT_VARIABLE IMPORT_${module_name}_OUTPUT
    ERROR_VARIABLE IMPORT_${module_name}_ERROR
    )
  if(${IMPORT_${module_name}_EXITCODE} EQUAL 0)
    set(PYTHON_MODULE_${module_name}_FOUND TRUE
      CACHE BOOL "Whether or not this Python module is present")
  else()
    set(PYTHON_MODULE_${module_name}_FOUND FALSE
      CACHE BOOL "Whether or not this Python module is present")
  endif()
endmacro()

# Macro to simplify checking if a Python module is available
macro(find_python_module module_name result)
  if(NOT DEFINED PYTHON_MODULE_${module_name}_FOUND)
    _find_python_module_internal(${module_name})
  endif()
  set(${result} ${PYTHON_MODULE_${module_name}_FOUND})
endmacro()
