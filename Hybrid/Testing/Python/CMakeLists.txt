ADD_TEST(HeaderTesting-Hybrid ${PYTHON_EXECUTABLE}
         ${VTK_SOURCE_DIR}/Common/Testing/Python/HeaderTesting.py
	 "${VTK_SOURCE_DIR}/Hybrid"
         vtk3DS.h
         vtkVRML.h
         )
