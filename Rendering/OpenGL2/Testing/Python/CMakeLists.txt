vtk_add_test_python(
  TestArrayRenderer.py,NO_RT
  TestUserShader2.py
  TestTopologyResolution.py
  )

if (VTK_USE_COCOA)
  vtk_add_test_python(
    NO_DATA NO_VALID NO_OUTPUT
    TestCocoaAutoreleasePool.py
    )
endif ()
