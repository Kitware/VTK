ADD_TEST(PrintSelf-GenericFiltering ${TCL_TCLSH}
         ${VTK_SOURCE_DIR}/Common/Testing/Tcl/PrintSelfCheck.tcl  
         ${VTK_SOURCE_DIR}/GenericFiltering)

ADD_TEST(TestSetObjectMacro-GenericFiltering ${TCL_TCLSH}
         ${VTK_SOURCE_DIR}/Common/Testing/Tcl/FindString.tcl
         "${VTK_SOURCE_DIR}/GenericFiltering/vtk\\\\*.h"
         "vtkSetObjectMacro"
         )
