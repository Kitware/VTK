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
ADD_TEST(PrintSelf-Common ${TCLSHCOMMAND}
         ${VTK_SOURCE_DIR}/Common/Testing/Tcl/PrintSelfCheck.tcl  
         ${VTK_SOURCE_DIR}/Common)
