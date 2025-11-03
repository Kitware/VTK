# VTK::RenderingANARI

## ANARI - Rendering API compatible with multiple backends

### Introduction

[ANARI](https://github.com/KhronosGroup/ANARI-SDK/tree/next_release) is an open standard 3D rendering API maintained by the Khronos ANARI<sup>:tm:</sup> Working Group. This API is used as a graphics front-end that supports multiple low level back-end rendering engines. The supported backends are listed in the [ANARI-SDK repository](https://github.com/KhronosGroup/ANARI-SDK/blob/v0.14.1/README.md#available-implementations). The front-end is a C99 API with C++ type-safe wrappers, used to build an in-memory hierarchical object tree that expresses the complete scene for a single frame, including 3D surface geometry and volumetric data.

---

### Using ANARI with VTK

ANARI can be enabled via the CMake variable `VTK_MODULE_ENABLE_VTK_RenderingAnari=YES`. VTK requires an installation of the ANARI-SDK library. The path to the ANARI-SDK installation directory has to be given with the following CMake variable `anari_DIR=<path-to-anari-install-directory>`. VTK requires a version of ANARI-SDK with a minimum version of `v0.9.1`. The repository can be found on [GitHub](https://github.com/KhronosGroup/ANARI-SDK/tree/v0.14.1).

Specific build instructions for your chosen platform are provided in [Building ANARI-SDK from Source](https://github.com/KhronosGroup/ANARI-SDK/blob/v0.14.1/README.md#building-the-sdk-from-source).

ANARI's backend can be chosen at runtime by providing the following environment variable: `ANARI_LIBRARY=<backend-name>`.

**Note**: If the library is not installed system-wide, the `LD_LIBRARY_PATH` may also need to be specified with the path to the backend built library.

The following sections show some backends that were tested in VTK with their corresponding version and dependencies.

---

### ANARI-VisRTX

The required version for VisRTX is `v0.11.0` with ANARI `v0.14.1`. The repository with the VisRTX sources can be found on [GitHub](https://github.com/NVIDIA/VisRTX/tree/v0.11.0).

To run an application using ANARI-VisRTX the name of the anari library is `ANARI_LIBRARY=visrtx`.

---

### ANARI-OSPRay

The OSPRay backend repository for ANARI can be found on [GitHub](https://github.com/ospray/anari-ospray). It is highly recommanded to use the internal superbuild for this repository.

With the changes applied in the superbuild CMake file, the build can be achieved by calling the follwing lines:

To run an application using ANARI-OSPRay the name of the anari library is `ANARI_LIBRARY=ospray`.
