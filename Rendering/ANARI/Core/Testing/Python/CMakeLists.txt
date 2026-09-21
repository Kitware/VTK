vtk_add_test_python(
  TestAnariSphere.py
)

set_tests_properties(VTK::RenderingAnariCorePython-TestAnariSphere
  PROPERTIES
  ENVIRONMENT "VTK_FACTORY_PREFER=SupportRenderPass=false;VTK_TESTING_IMAGE_COMPARE_METHOD=TIGHT_VALID"
  TIMEOUT 150)
