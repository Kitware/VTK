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

## Controller Model Rendering

OpenXR does not yet provide any api for rendering controller models in the same way that OpenVR does, though work towards that goal has been in progress for a long time.  Until that materializes, VTK provides a way to render glTF controller models by using the currently active interaction profile to index into a customizable lookup table of rendering assets.

Out of the box, VTK does not come with any controller models for the `VTK::RenderingOpenXR` module to render, but a data archive containing models for a subset of the currently supported interaction profiles is available from [here](https://www.paraview.org/files/data/OpenXRControllerModels-0.1.tgz).  Those models are included by default in ParaView binary downloads, but can be extracted into any build or install tree, and VTK will automatically find and use them.

Though some of the contents are omitted below for expository purposes, the structure of the default model archive is mostly as follows:

```
OpenXRControllerModels-<version>/
OpenXRControllerModels-<version>/openxr_controllermodels.json
OpenXRControllerModels-<version>/valve_index/
OpenXRControllerModels-<version>/valve_index/left/
OpenXRControllerModels-<version>/valve_index/left/scene.gltf
OpenXRControllerModels-<version>/valve_index/right/
OpenXRControllerModels-<version>/valve_index/right/scene.gltf
OpenXRControllerModels-<version>/htc_vive/
OpenXRControllerModels-<version>/htc_vive/scene.gltf
```

Models are also accompanied by a binary file containing geometry, a base color image, and a license file detailing model attribution and the terms under which the model is distributed.

The contents of `openxr_controllermodels.json` looks like this:

```
[
  {
    "interaction_profile": "/interaction_profiles/htc/vive_controller",
    "asset_paths": {
      "left_controller": "htc_vive/scene.gltf",
      "right_controller": "htc_vive/scene.gltf"
    }
  },
  {
    "interaction_profile": "/interaction_profiles/valve/index_controller",
    "asset_paths": {
      "left_controller": "valve_index/left/scene.gltf",
      "right_controller": "valve_index/right/scene.gltf"
    }
  }
]
```

Note it's just a json array of objects. Each object contains an interaction profile, which should match one of those referenced in the binding files found in `VTK/Rendering/OpenxR/` or `ParaView/Plugins/XRInterface/Plugin/`. Each object also contains a dictionary indicating relative path (from the root of the model archive) to glTF models for left and right controllers.

By extracting this archive (or a sufficiently similar one) into your VTK or ParaView build or install tree, the `VTK::RenderingOpenXR` module will be able to render the supplied models in place of the default green pyramids. Note that downloaded ParaView binaries come with this default set of controller models already installed.

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
