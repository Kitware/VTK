# VTK::RenderingOpenVR

The `OpenVR` module aims to support PC-based rendering to virtual reality
headsets via Valve's [`OpenVR` API](https://github.com/ValveSoftware/openvr).

The OpenVR standard has been succeeded by the industry-wide [OpenXR](https://www.khronos.org/openxr/) standard.
See the VTK [`OpenXR` module](../OpenXR/README.md) for modernized support.
The VTK `OpenVR` module is preserved for legacy support.

## Supported Devices

Any device that renders with OpenGL and runs from the OpenVR runtime
is theoretically supported. Devices include:
- HTC Vive (, Pro)
- Valve Index
- Oculus Rift (, S)
- Meta Quest (1,2,3,Pro)
  - [Quest Link](https://www.meta.com/help/quest/articles/headsets-and-accessories/oculus-link/connect-link-with-quest-2/) or [Air Link](https://www.meta.com/help/quest/articles/headsets-and-accessories/oculus-link/connect-with-air-link/) only)
- HP Reverb G2

## Supported Controllers

The VTK `OpenVR` module provides bindings for the following controllers:
- [HP Motion Controller](vtk_openvr_binding_hpmotioncontroller.json)
- [Valve Knuckles](vtk_openvr_binding_knuckles.json)
- [Oculus Touch](vtk_openvr_binding_oculus_touch.json)
- [HTC Vive Controller](vtk_openvr_binding_vive_controller.json)

The VTK `OpenVR` module is considered legacy and not under active development.
Please see the VTK `OpenXR` module for support for additional controllers and
alternate input mechanisms.

## Testing

A minimum [OpenVRCone](https://examples.vtk.org/site/Cxx/GeometricObjects/OpenVRCone/) example is
available for download on the [VTK Examples website](https://examples.vtk.org/site/).

Tests in the [`Testing/Cxx`](Testing/Cxx) directory may also be run to demonstrate VTK `RenderingOpenVR`
capabilities.
