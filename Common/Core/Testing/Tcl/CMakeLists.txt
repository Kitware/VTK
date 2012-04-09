add_test(TestSetObjectMacro-Common ${TCL_TCLSH}
         ${vtkCommonCore_SOURCE_DIR}/Testing/Tcl/FindString.tcl
         "${VTK_SOURCE_DIR}/Common/Core/vtk\\\\*.h"
         "vtkSetObjectMacro"
         ${VTK_SOURCE_DIR}/Common/Core/vtkSetGet.h
         )
add_test(PrintSelf-CommonCore ${TCL_TCLSH}
         ${${vtk-module}_SOURCE_DIR}/Testing/Tcl/PrintSelfCheck.tcl
	 ${VTK_SOURCE_DIR}/Common/Core)

set(tests
  TestArrays
  TestEmptyInput
  TestSetGet
  TestString
  TestIgnoreBTX
  otherInterp
  otherPrint
  )

foreach(test ${tests})
  add_test(NAME ${vtk-module}-${test}TCL
    COMMAND ${VTK_TCL_EXE}
      ${CMAKE_CURRENT_SOURCE_DIR}/${test}.tcl
      -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
endforeach()
