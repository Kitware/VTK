ADD_TEST(PrintSelf-Geovis ${TCL_TCLSH}
         ${VTK_SOURCE_DIR}/Common/Testing/Tcl/PrintSelfCheck.tcl  
         ${VTK_SOURCE_DIR}/Geovis)

ADD_TEST(TestSetObjectMacro-Geovis ${TCL_TCLSH}
         ${VTK_SOURCE_DIR}/Common/Testing/Tcl/FindString.tcl
         "${VTK_SOURCE_DIR}/Geovis/vtk\\\\*.h"
         "vtkSetObjectMacro"
         )
