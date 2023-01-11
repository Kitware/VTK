# The runtime tests requires an OpenXR runtime and a HMD to pass
# Do not turn on controllers to avoid any issue with the image comparison
# Tested with :
# - Monado on linux
# - Valve Index on Windows
# - Windows Mixed Reality within SteamVR
option(VTK_OPENXR_TESTING_ENABLE_RUNTIME_TESTS "Enable OpenXR tests that require an OpenXR runtime" OFF)
mark_as_advanced(VTK_OPENXR_TESTING_ENABLE_RUNTIME_TESTS)

# The interactive tests require an OpenXR runtime and a HMD and will just run interactively.
# To make it pass, just exit the VR environment using the menu inside VR.
option(VTK_OPENXR_TESTING_ENABLE_INTERACTIVE_TESTS "Enable OpenXR tests that are interactive" OFF) 
mark_as_advanced(VTK_OPENXR_TESTING_ENABLE_INTERACTIVE_TESTS)

set(openxr_tests
  TestOpenXRInstantiation.cxx,NO_VALID)

if (VTK_OPENXR_TESTING_ENABLE_RUNTIME_TESTS)
  list (APPEND openxr_tests
    TestOpenXRInitialization.cxx,NO_VALID
    TestOpenXRRendering.cxx
    TestOpenXRRenderingVolume.cxx)
endif ()

if (VTK_OPENXR_TESTING_ENABLE_INTERACTIVE_TESTS)
  list (APPEND openxr_tests
    TestOpenXRInteractiveBox.cxx,NO_VALID
    TestOpenXRInteractiveDragon.cxx,NO_VALID)
endif ()

vtk_add_test_cxx(vtkRenderingOpenXRCxxTests tests ${openxr_tests})
vtk_test_cxx_executable(vtkRenderingOpenXRCxxTests tests)
