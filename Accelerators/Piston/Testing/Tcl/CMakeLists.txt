add_test(NAME ${vtk-module}Tcl-TestSetObjectMacro
         COMMAND ${TCL_TCLSH}
         ${VTK_SOURCE_DIR}/Common/Core/Testing/Tcl/FindString.tcl
         "${${vtk-module}_SOURCE_DIR}/vtk\\*.h"
         "vtkSetObjectMacro"
         ""
         )

add_test(NAME ${vtk-module}Tcl-PrintSelf
         COMMAND ${TCL_TCLSH}
         ${VTK_SOURCE_DIR}/Common/Core/Testing/Tcl/PrintSelfCheck.tcl
         ${${vtk-module}_SOURCE_DIR})
