# VTK::RenderingOpenXR

The `OpenXR` module aims to support rendering to a variety of mixed reality
devices under the OpenXR industry-wide standard. Detailed information
on the OpenXR specification and compliant OpenXR runtimes may be found
on the [Khronos Group website](https://www.khronos.org/openxr/).

## Supported Devices

The OpenXR standard is implemented by most PC-based OpenXR runtimes and
devices. The VTK `OpenXR` module aims to support most devices that implement
the OpenXR specification and support OpenGL rendering.

The list of possible XR device targets is extensive and constantly expanding.
At the time of writing, theoretically supported devices include but are not
limited to the following:
- Valve Index
- HTC Vive (, Pro)
- Meta Quest (1,2,3,Pro)
  - [Quest Link](https://www.meta.com/help/quest/articles/headsets-and-accessories/oculus-link/connect-link-with-quest-2/) or [Air Link](https://www.meta.com/help/quest/articles/headsets-and-accessories/oculus-link/connect-with-air-link/) only
- HP Reverb G2

Supported input devices and mechanisms include the following:
- HP Mixed Reality Controller [json](https://gitlab.kitware.com/vtk/vtk/-/blob/master/Rendering/OpenXR/vtk_openxr_binding_hp_mixed_reality.json)
- HTC Vive Controller [json](https://gitlab.kitware.com/vtk/vtk/-/blob/master/Rendering/OpenXR/vtk_openxr_binding_htc_vive_controller.json)
- KHR Simple Controller [json](https://gitlab.kitware.com/vtk/vtk/-/blob/master/Rendering/OpenXR/vtk_openxr_binding_khr_simple_controller.json)
- Valve Knuckles [json](https://gitlab.kitware.com/vtk/vtk/-/blob/master/Rendering/OpenXR/vtk_openxr_binding_knuckles.json)
- Microsoft Hand Interaction [json](https://gitlab.kitware.com/vtk/vtk/-/blob/master/Rendering/OpenXR/vtk_openxr_binding_microsoft_hand_interaction.json)

The `OpenXR` module is commonly tested with the Valve Index and HTC Vive
virtual reality headsets.

## Adding New Devices

It may be necessary to tell VTK how to handle inputs from a new OpenXR-compatible device.
Consider contributing a new JSON input binding specification to add support for a new
XR device to the VTK `OpenXR` module.

The OpenXR interaction profile specification is documented here:

https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#semantic-path-interaction-profiles

Input binding JSON files should be added to `OpenXR` and set as default in
`vtk_openxr_actions.json`[json](https://gitlab.kitware.com/vtk/vtk/-/blob/master/Rendering/OpenXR/vtk_openxr_actions.json).

## Building

The `OpenXR` module depends on the [`OpenXR-SDK`](https://github.com/KhronosGroup/OpenXR-SDK) library.
`OpenXR-SDK` can be built with CMake via the steps below:

```sh
> git clone git@github.com:KhronosGroup/OpenXR-SDK.git
> mkdir OpenXR-SDK-build
> cd OpenXR-SDK-build
OpenXR-SDK-build > cmake ../OpenXR-SDK
OpenXR-SDK-build > cmake --build . --config "Release"
```

The `OpenXR` is turned off in VTK by default. Run the following steps to build VTK with OpenXR:

```sh
VTK-build > cmake -DVTK_MODULE_ENABLE_VTK_RenderingOpenXR:STRING=YES -DOpenXR_INCLUDE_DIR:PATH="path/to/OpenXR-SDK/include/openxr" -DOpenXR_LIBRARY:FILEPATH="path/to/OpenXR-SDK-build/src/loader/Release/openxr_loader.lib" path/to/VTK
VTK-build > cmake --build . --config "Release"
```

## Testing

Minimum OpenXR examples are available in the [`Testing/Cxx`](https://gitlab.kitware.com/vtk/vtk/-/blob/master/Rendering/OpenXR/Testing/Cxx) directory for testing.

To run OpenXR tests, first build VTK with testing enabled.

```sh
VTK-build > cmake -DVTK_BUILD_TESTING:BOOL=ON path/to/VTK
VTK-build > cmake --build . --config "Release"
```

Then run the test with CTest.

```sh
VTK-build > ctest -C Release -R <name_of_test>
```

## Additional Notes

See [VTK `OpenXRRemoting` documentation](../OpenXRRemoting/README.md) for information on virtual reality
rendering to DirectX devices such as the Microsoft HoloLens 2.

Some non-OpenGL devices may be compatible with the WebGL and WebXR specifications.
If your XR device does not support OpenGL or OpenXR, we suggest visiting
[VTK.js WebXR documentation](https://kitware.github.io/vtk-js/docs/develop_webxr.html)
for a web-driven solution.
