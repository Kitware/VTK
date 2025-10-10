# VTK Wheel SDKs

## Overview

VTK Wheel SDKs are Python wheels that contains a snapshot of VTK CI install directory.
They are available at https://vtk.org/files/wheel-sdks.

VTK install directory contains everything that is needed to build VTK native extensions and wrap them in Python.
When built against the SDK, these native extensions can use the VTK wheel native libraries (DLLs, SOs or DyLibs).

This enables the following features:
- Enable subclassing `vtkObject` based classes and have them work natively with VTK own objects.
- Enable sharing `vtkObject` between different Python wheels without issues.
- Enable exposing VTK in public APIs of Python wheels that contains native (C++) VTK code.
- Remove the need to package VTK in every wheel that uses it.

Each wheel distributed on [VTK GitLab Package Registry](https://gitlab.kitware.com/vtk/vtk/-/packages), have its corresponding wheel SDK, starting with VTK 9.6.0.

## How to use the VTK Wheel SDKs

VTK Wheel SDKs are intended to be consumed "build-system requirements" with [scikit-build-core](https://scikit-build-core.readthedocs.io/en/latest/) Python build backend.

The VTK Wheel SDKs include a [scikit-build-core entry-point](https://scikit-build-core.readthedocs.io/en/latest/configuration/search_paths.html).
This enables projects to automatically find the VTK installation from the wheel SDKs, using a regular `find_package(VTK)`.

For more information about how to use the wheels SDK, please refer to the "Build/VTKPythonExtensions" example.

## Notes

The VTK wheel SDK is also distributed as `tar.xz` archives, this is deprecated in favor of the `.whl`, and may be removed in future versions.
Note that a `.whl` is a ZIP archive, and its content may be used as-is.
The same content as in the `tar.xz` archives can be found in the `vtk_sdk/content` folder of the `.whl`.
