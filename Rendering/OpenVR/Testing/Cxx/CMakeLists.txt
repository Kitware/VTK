# The runtime tests requires an OpenVR runtime and a HMD to pass
# Do not turn on controllers to avoid any issue with the image comparison
# Tested with :
# - Valve Index on Windows
# - Windows Mixed Reality within SteamVR
option(VTK_OPENVR_TESTING_ENABLE_RUNTIME_TESTS "Enable OpenVR tests that require an OpenVR runtime" OFF)
mark_as_advanced(VTK_OPENVR_TESTING_ENABLE_RUNTIME_TESTS)

# The interactive tests require an OpenVR runtime and a HMD and will just run interactively.
# To make it pass, just exit the VR environment using the menu inside VR.
option(VTK_OPENVR_TESTING_ENABLE_INTERACTIVE_TESTS "Enable OpenVR tests that are interactive" OFF)
mark_as_advanced(VTK_OPENVR_TESTING_ENABLE_INTERACTIVE_TESTS)

set(OpenVR_tests
  TestAvatar.cxx
  TestOpenVRInstantiation.cxx,NO_VALID)

if (VTK_OPENVR_TESTING_ENABLE_RUNTIME_TESTS)
  list (APPEND OpenVR_tests
    TestOpenVRInitialization.cxx,NO_VALID
    TestOpenVRRendering.cxx
    TestOpenVRRenderingVolume.cxx)
endif ()

if (VTK_OPENVR_TESTING_ENABLE_INTERACTIVE_TESTS)
  list (APPEND OpenVR_tests
    TestOpenVRInteractiveBox.cxx,NO_VALID
    TestOpenVRInteractiveDragon.cxx,NO_VALID)
endif ()

vtk_add_test_cxx(vtkRenderingOpenVRCxxTests tests ${OpenVR_tests})
vtk_test_cxx_executable(vtkRenderingOpenVRCxxTests tests)
