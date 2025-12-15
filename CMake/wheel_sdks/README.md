# vtk-sdk

## Overview

The Visualization Toolkit (VTK) is a robust and open-source software system that
provides advanced features in 3D computer graphics, image processing, modeling,
volume rendering, and scientific visualization. It offers threaded and
distributed-memory parallel processing for scalability and better performance.

This project is intended to distribute the content of the existing VTK wheel
SDKs as first-class `vtk-sdk` wheels.

Each `vtk-sdk` Python wheel is equipped with a scikit-build-core `cmake.prefix`
[entrypoint][scikit-build-core-entrypoint], housing the official VTK SDK sourced
from the corresponding [archive][wheel-sdks-link].

[wheel-sdks-link]: https://vtk.org/files/wheel-sdks/
[scikit-build-core-entrypoint]:
  https://scikit-build-core.readthedocs.io/en/latest/cmakelists.html#finding-other-packages

## License

VTK is distributed under the OSI-approved BSD 3-clause License. See
Copyright.txt for details.
