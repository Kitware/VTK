ADD_TEST(otherInterp ${VTK_EXECUTABLE}
         ${VTK_SOURCE_DIR}/Common/Testing/Tcl/otherInterp.tcl 
         -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
ADD_TEST(otherPrint ${VTK_EXECUTABLE}
         ${VTK_SOURCE_DIR}/Common/Testing/Tcl/otherPrint.tcl 
         -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
ADD_TEST(TestEmptyInput ${VTK_EXECUTABLE}
         ${VTK_SOURCE_DIR}/Common/Testing/Tcl/TestEmptyInput.tcl 
         -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
ADD_TEST(TestSetGet ${VTK_EXECUTABLE}
         ${VTK_SOURCE_DIR}/Common/Testing/Tcl/TestSetGet.tcl 
         -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
ADD_TEST(TestArrays ${VTK_EXECUTABLE}
         ${VTK_SOURCE_DIR}/Common/Testing/Tcl/TestArrays.tcl 
         -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
ADD_TEST(otherDataSetAttributes ${VTK_EXECUTABLE}
         ${VTK_SOURCE_DIR}/Common/Testing/Tcl/otherDataSetAttributes.tcl 
         -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
ADD_TEST(TestAmoebaMinimizer ${VTK_EXECUTABLE}
         ${VTK_SOURCE_DIR}/Common/Testing/Tcl/TestAmoebaMinimizer.tcl)
ADD_TEST(PrintSelf-Common ${TCL_TCLSH}
         ${VTK_SOURCE_DIR}/Common/Testing/Tcl/PrintSelfCheck.tcl  
         ${VTK_SOURCE_DIR}/Common)
ADD_TEST(TestSetObjectMacro-Common ${TCL_TCLSH}
         ${VTK_SOURCE_DIR}/Common/Testing/Tcl/FindString.tcl
         "${VTK_SOURCE_DIR}/Common/vtk\\\\*.h"
         "vtkSetObjectMacro"
         ${VTK_SOURCE_DIR}/Common/vtkSetGet.h
         )
