ADD_TEST(PrintSelf-TextAnalysis ${TCL_TCLSH}
         ${VTK_SOURCE_DIR}/Common/Testing/Tcl/PrintSelfCheck.tcl  
         ${VTK_SOURCE_DIR}/TextAnalysis)

ADD_TEST(TestSetObjectMacro-TextAnalysis ${TCL_TCLSH}
         ${VTK_SOURCE_DIR}/Common/Testing/Tcl/FindString.tcl
         "${VTK_SOURCE_DIR}/TextAnalysis/vtk\\\\*.h"
         "vtkSetObjectMacro"
         )
